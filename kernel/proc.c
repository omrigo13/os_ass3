#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void
proc_mapstacks(pagetable_t kpgtbl) {
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table at boot time.
void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->kstack = KSTACK((int) (p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void) {
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void) {
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int
allocpid() {
  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;
  p->scfifo_index = 0;
  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
  p->scfifo_index = 0;
}

// Create a user page table for a given process,
// with no user memory, but with trampoline pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe just below TRAMPOLINE, for trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// od -t xC initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;
  
  // allocate one user page and copy init's instructions
  // and data into it.
  uvminit(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n, 1)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  np->scfifo_index = p->scfifo_index;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

#ifndef NONE
  if(swap_participate(np)){
    if(init_data(np) < 0){
      freeproc(np);
      return -1;
    }
    if(init_swap(np) < 0){
      freeproc(np);
      return -1;
    }
    if(!swap_participate(p)){
      char *mem = kalloc();
      for(int offset = 0; offset < (PGSIZE * MAX_PSYC_PAGES); offset += PGSIZE){
        if(writeToSwapFile(np, mem, offset, PGSIZE) < 0){
          freeproc(np);
          release(&np->lock);
          return -1;
        }
      }
      kfree(mem);
    }
  }
  if(swap_participate(p)){
    if(dup_swap(np, p) < 0){
      freeproc(np);
      return -1;
    }
    memmove(np->ramArr, p->ramArr, sizeof(p->ramArr));
    memmove(np->swapArr, p->swapArr, sizeof(p->swapArr));
  }
#endif

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  #ifndef NONE
    if(swap_participate(p))
      clear_data(p);
  #endif

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);
  
  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
  struct proc *np;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(np = proc; np < &proc[NPROC]; np++){
      if(np->parent == p){
        // make sure the child isn't still in exit() or swtch().
        acquire(&np->lock);

        havekids = 1;
        if(np->state == ZOMBIE){
          // Found one.
          pid = np->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                  sizeof(np->xstate)) < 0) {
            release(&np->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(np);
          release(&np->lock);
          release(&wait_lock);
          return pid;
        }
        release(&np->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || p->killed){
      release(&wait_lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);

        #if defined(NFUA) || defined(LAPA)
          update_access_counter(p);
        #endif

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
      release(&p->lock);
    }
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock);  //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}

#ifndef NONE
int 
init_swap(struct proc *p){
  char *mem = kalloc();
  for(int offset = 0; offset < PGSIZE * MAX_PSYC_PAGES; offset += PGSIZE){
    if(writeToSwapFile(p, mem, offset, PGSIZE) < 0)
      return -1;
  }
  kfree(mem);
  return 0;
}

int
init_data(struct proc *p){
  if(p->swapFile == 0 && createSwapFile(p) < 0)
    return -1;
    
  for(int i = 0; i < MAX_PSYC_PAGES; i++){
    p->ramArr[i].addr = 0;
    p->ramArr[i].access_counter = 0;
    p->ramArr[i].used = 0;
    p->swapArr[i].pgdir = i * PGSIZE;
    p->swapArr[i].used = 0;
    p->swapArr[i].addr = 0;
  }
  return 0;
}

void 
clear_data(struct proc *p){
  if(removeSwapFile(p) < 0){
    panic("clear_data: delete swap page failed");
  }
  
  p->swapFile = 0;

  for(int i = 0; i < MAX_PSYC_PAGES; i++){
    p->ramArr[i].access_counter = 0;
    p->ramArr[i].used = 0;
    p->ramArr[i].addr = 0;
    p->swapArr[i].used = 0;
    p->swapArr[i].addr = 0;
  }
}

void 
update_access_counter(struct proc *p){
  int i = 0;
  struct page *ram_page;
  for(ram_page = p->ramArr; ram_page < &p->ramArr[MAX_PSYC_PAGES]; ram_page++){
    pte_t *pte = walk(p->pagetable, ram_page->addr, 0);
    ram_page->access_counter >>= 1;
    if(*pte & PTE_A){
      ram_page->access_counter |= (1 << 31); // add 1 to the msb of the access counter
      *pte &= ~PTE_A;
    }
    i++;
  }
}

int 
free_ram_index(struct proc *p){
  int ram_i = -1;
  for(int i = 0; i < MAX_PSYC_PAGES; i++)
    if(p->ramArr[i].used == 0)
      ram_i = i;  
  return ram_i;
}

int 
free_swap_index(struct proc *p){
  int swap_i = -1;
  for(int i = 0; i < MAX_PSYC_PAGES; i++){
    if(p->swapArr[i].used == 0){
      swap_i = i;
      break;
    }
  }
  return swap_i;
}

int 
get_swap(struct proc *p, int addr){
  int swap_i = -1;
  for(int i = 0; i < MAX_PSYC_PAGES; i++){
    if(p->swapArr[i].addr == addr){
      swap_i = i;
      break;
    }
  }
  return swap_i;
}

int 
ram_to_swap(struct proc *p, int page_index){
    if(page_index < 0 || page_index > MAX_PSYC_PAGES)
      panic("ram_to_swap: bad page index");
    if(p->ramArr[page_index].used == 0)
      panic("ram_to_swap: free page");
    
    pte_t *pte = walk(p->pagetable, p->ramArr[page_index].addr, 0);
    
    if(pte == 0)
      panic("ram_to_swap: pte unallocated");
    if(!(*pte & PTE_V) && !(*pte & PTE_LZ))
      panic("ram_to_swap: invalid page");

    int swap_page_i = free_swap_index(p);
    if(swap_page_i < 0)
      return -1;
    
    if(!(*pte & PTE_LZ)){
      uint64 pa = PTE2PA(*pte);
        if(writeToSwapFile(p, (char *)pa, p->swapArr[swap_page_i].pgdir, PGSIZE) < 0)
          return -1;
      kfree((void *)pa);
    }

    p->swapArr[swap_page_i].addr = p->ramArr[page_index].addr;
    p->swapArr[swap_page_i].used = 1;

    p->ramArr[page_index].used = 0;
    p->ramArr[page_index].access_counter = 0;
    p->ramArr[page_index].addr = 0;
    *pte |= PTE_PG;     //page was swapped out
    *pte &= ~PTE_V;     //page is valid
    sfence_vma();       // refreshing the TLB
    return 0;
}

int 
swap_to_ram(struct proc *p, int swap_index, int ram_index){
  if(ram_index < 0 || ram_index > MAX_PSYC_PAGES)
    panic("swap_to_ram: bad ram index");
  if(swap_index < 0 || swap_index > MAX_PSYC_PAGES)
    panic("swap_to_ram: bad swap index");
  if(p->swapArr[swap_index].used == 0)
    panic("swap_to_ram: free page");
    
  pte_t *pte = walk(p->pagetable, p->swapArr[swap_index].addr, 0);
    
  if(pte == 0)
    panic("swap_to_ram: pte unallocated");
  if(*pte & PTE_V || !(*pte & PTE_PG))
    panic("swap_to_ram: page is valid");
  if(p->ramArr[ram_index].used == 1)
    panic("swap_to_ram: ram page is not free");
    
  if(!(*pte & PTE_LZ)){
    uint64 new_pa = (uint64)kalloc();
    if(readFromSwapFile(p, (char *)new_pa, p->swapArr[swap_index].pgdir, PGSIZE) < 0){
      kfree((void *)new_pa);
      return -1;
    }
    *pte = PTE_FLAGS(*pte) | PA2PTE(new_pa);     // insert the new allocated pa to the pte in the correct part
    *pte |= PTE_V;                               // set the flag stating the page is valid
  }
  
  p->ramArr[ram_index].used = 1;
  p->ramArr[ram_index].addr = p->swapArr[swap_index].addr;
  p->ramArr[ram_index].access_counter = access_counter_value;
  p->swapArr[swap_index].used = 0;
  p->swapArr[swap_index].addr = 0;
  *pte &= ~PTE_PG; // clear the swapped out page flag
  sfence_vma();    // refreshing the TLB
  return 0;
}

void 
swap_to_ram_addr(struct proc *p, uint64 pa, uint64 addr, int ram_index){
  pte_t *pte = walk(p->pagetable, addr, 0);

  if(pte == 0)
    panic("swap_to_ram_addr: pte is not allocated");
  if((*pte & PTE_V || !(*pte & PTE_PG)))
    panic("swap_to_ram_addr: page is valid");
  if(p->ramArr[ram_index].used == 1)
    panic("swap_to_ram_addr: ram page is not free");
  
  if(!(*pte & PTE_LZ)){
    *pte |= PTE_V;
    *pte = PA2PTE(pa) | PTE_FLAGS(*pte);
  }

  p->ramArr[ram_index].used = 1;
  p->ramArr[ram_index].access_counter = access_counter_value;
  p->ramArr[ram_index].addr = addr;
  *pte &= ~PTE_PG;
  sfence_vma();
}

int 
access_counter_by_bits(uint bitmask){
  int counter = 0;
  while(bitmask){
    counter += bitmask & 1;
    bitmask >>= 1;
  }
  return counter;
}

int 
lapa_selector(struct proc *p){
  int min_counter = access_counter_by_bits(p->ramArr[0].access_counter);
  int min_access_counter = p->ramArr[0].access_counter;
  int index_selector = 0;
  for(int i = 1; i < MAX_PSYC_PAGES; i++){
    int counter = access_counter_by_bits(p->ramArr[i].access_counter);
    if(min_counter > counter){
      min_counter = counter;
      min_access_counter = p->ramArr[i].access_counter;
      index_selector = i;
    }
    else if(min_counter == counter && min_access_counter >= p->ramArr[i].access_counter){
      min_access_counter = p->ramArr[i].access_counter;
      index_selector = i;
    }
  }
  return index_selector;
}

int 
scfifo_selector(struct proc *p){
  while(p->scfifo_index < MAX_PSYC_PAGES){
    pte_t *pte = walk(p->pagetable, p->ramArr[p->scfifo_index].addr, 0);
    if(!(*pte & PTE_A)){
      int index_selector = p->scfifo_index;
      p->scfifo_index = ((p->scfifo_index + 1) % MAX_PSYC_PAGES);
      return index_selector;
    }
    else{
      *pte &= ~PTE_A;
    }
    p->scfifo_index = ((p->scfifo_index + 1) % MAX_PSYC_PAGES);
  }
  panic("scfifo_selector: bad scfifo selector");
}

int 
nfua_selector(struct proc *p){
  int min_age = p->ramArr[0].access_counter;
  int index_selector = 0;
  for(int i = 1; i < MAX_PSYC_PAGES; i++){
    if(min_age >= p->ramArr[i].access_counter){
      min_age = p->ramArr[i].access_counter;
      index_selector = i;
      }
  }
  return index_selector;
}
#endif

int 
page_selector(struct proc *p){
  #ifdef NFUA
    return nfua_selector(p);
  #endif

  #ifdef LAPA
    return lapa_selector(p);
  #endif

  #ifdef SCFIFO
    return scfifo_selector(p);
  #endif
  
  return -1;
}

#ifndef NONE
int 
add_page_ram(struct proc *p, uint64 addr){
  if(!swap_participate(p))
    return 0;
    
  int ram_index = free_ram_index(p);
    
  if(ram_index < 0){
    int to_swap = page_selector(p);
    if(ram_to_swap(p, to_swap) < 0)
      return -1;
    ram_index = to_swap;
  }
  p->ramArr[ram_index].addr = addr;
  p->ramArr[ram_index].access_counter = 0;
  p->ramArr[ram_index].used = 1;
  return 0;
}
#endif

int 
delete_page_ram(struct proc *p, uint64 addr){
  int page_flag = 0;
  if(!swap_participate(p))
    return 0;
  
  for(int i = 0; i < MAX_PSYC_PAGES; i++){
    page_flag = (p->ramArr[i].used == 1 && p->ramArr[i].addr == addr);
    if(page_flag){
      p->ramArr[i].addr = 0;
      p->ramArr[i].access_counter = 0;
      p->ramArr[i].used = 0;            
      return 0;
    }
  }
  for(int i = 0; i < MAX_PSYC_PAGES; i++){
    page_flag = (p->swapArr[i].used == 1 && p->swapArr[i].addr == addr);
    if(page_flag){
      p->swapArr[i].used = 0;
      p->swapArr[i].addr = 0;            
      return 0;
    }
  }
  return -1;
}

uint64 
swap_pa(struct proc *p, int index){
  pte_t *pte = walk(p->pagetable, p->swapArr[index].addr, 0);
  uint64 new_pa = 0;
  if(!(*pte & PTE_LZ)){
    new_pa = (uint64)kalloc();
    if(readFromSwapFile(p, (char *)new_pa, p->swapArr[index].pgdir, PGSIZE) < 0)
      panic("swap_pa: read from swap failed");
  }
  p->swapArr[index].addr = 0;
  p->swapArr[index].used = 0;
  
  return new_pa;
}

#ifndef NONE
int 
handle_page_fault(uint64 addr){
  struct proc *p = myproc();
  if (!swap_participate(p))
    panic("handle_page_fault: none swap page fault");
  
  pte_t *pte = walk(p->pagetable, addr, 0);

  if(pte == 0 || (!(*pte & PTE_PG) && !(*pte & PTE_V) && !(*pte & PTE_LZ)))
    return -1;
  
  if(*pte & PTE_V)
    panic("handle_page_fault: valid page");
    
  if(!(*pte & PTE_PG))
    return 0;
    
  addr = PGROUNDDOWN(addr);
  int page_index = get_swap(p, addr);
  if(page_index < 0)
    panic("handle_page_fault: expected page in swap");

  uint64 pa = 0;
  int ram_index = free_ram_index(p);
  if(ram_index < 0){ 
    int page_to_swap = page_selector(p);
    if(free_swap_index(p) < 0){
      pa = swap_pa(p, page_index);
      page_index = -1;
    }
    ram_to_swap(p, page_to_swap); // written the page with to_swap index to the file.
    ram_index = page_to_swap;
  }
  if(page_index < 0)
    swap_to_ram_addr(p, pa, addr, ram_index);        
  else{
    if(swap_to_ram(p, page_index, ram_index) < 0)
      return -1;
  }
  return 0;
}
#endif

int 
handle_lazy_page_fault(uint64 addr){
  struct proc *p = myproc();
  pte_t *pte = walk(p->pagetable, addr, 0);
  if(pte == 0 || (!(*pte & PTE_V) && !(*pte & PTE_LZ)))
    return -1;

  if(!(*pte & PTE_LZ))
    return 0;
    
  if(*pte & PTE_V)
    panic("handle_lazy_page_fault: page is valid");
  
  if(!(*pte & PTE_PG)){
    char *mem = kalloc();
    if(mem == 0)
      return -1;
    memset(mem, 0, PGSIZE);

    *pte |= PTE_V;
    *pte &= ~PTE_LZ;
    *pte = PA2PTE(mem) | PTE_FLAGS(*pte);
    sfence_vma(); // refreshing the TLB
  }
  return 0;
}

int 
swap_participate(struct proc *p){
  #ifdef NONE
    return 0;
  #endif
  #ifndef NONE
  if(strncmp(p->name, "initcode", sizeof(p->name)) == 0)
    return 0;

  else if(strncmp(p->name, "init", sizeof(p->name)) == 0)
    return 0;
  
  else if(strncmp(p->parent->name, "init", sizeof(p->parent->name)) == 0)
    return 0;    
  
  return 1;
  #endif
}