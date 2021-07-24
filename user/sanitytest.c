// #include "kernel/param.h"
// #include "kernel/types.h"
// #include "kernel/stat.h"
// #include "user/user.h"
// #include "kernel/fs.h"
// #include "kernel/fcntl.h"
// #include "kernel/syscall.h"
// #include "kernel/memlayout.h"
// #include "kernel/riscv.h"

// void swaping_test()
// {
//     if (fork() == 0)
//     {
//         printf("-------------Task 1 Test-----------\n");
//         char *arr = (char *)sbrk(28 * PGSIZE); // create an array 28 pages long (proc starts with 4 pages) so we have 32 pages allocated
//         printf("Finished increments the program's data space by sbrk \n");
//         for (int i = 0; i < 28; i++)
//         {
//             printf("%d\n", i);
//             arr[(i * PGSIZE)] = i + '0'; //writes data to all pages
//         }
//         printf("Finished writing memory on all pages\n");
//         for (int j = 27; j >= 0; j--)
//         {
//             printf("%d\n", j);
//             arr[j * PGSIZE] = j + '0';
//         }

//         for (int i = 0; i < 28; i++)
//         {
//             if (arr[i * PGSIZE] != i + '0')
//             {
//                 printf("Failed, wrong value was written on page%d, %c\n", i, arr[i * PGSIZE]);
//                 break;
//             }
//         }
//         printf("Passed!\n");

//         printf("-------------Task 1 Test DONE-----------\n");
//         exit(0);
//     }
//     else
//     {
//         printf("-------------Parent starting-------------\n");

//         wait(0);
//     }
// }

// void sbrk_test() //Sbrk test
// { 
//     printf("--------------------sbrk_test Started ----------------------\n");
//     printf("Allocating 29 pages\n");
//     if (fork() == 0)
//     {
//         for (int i = 0; i < 29; i++)
//         {
//             printf("Calling sbrk number %d\n", i);
//             sbrk(PGSIZE);
//         }
//         exit(0);
//     }
//     wait(0);
//     printf("--------------------Finished sbrk_test----------------------\n");
// }

// void alloc_dealloc_test()
// {
//     printf("alloc dealloc test\n");
//     if (!fork())
//     {
//         int *arr = (int *)(sbrk(PGSIZE * 20));
//         for (int i = 0; i < PGSIZE * 20 / sizeof(int); ++i)
//         {
//             arr[i] = 0;
//         }
//         sbrk(-PGSIZE * 20);
//         printf("dealloc complete\n");
//         arr = (int *)(sbrk(PGSIZE * 20));
//         for (int i = 0; i < PGSIZE * 20 / sizeof(int); ++i)
//         {
//             arr[i] = 2;
//         }
//         for (int i = 0; i < PGSIZE * 20 / sizeof(int); ++i)
//         {
//             if (i % PGSIZE == 0)
//             {
//                 printf("arr[%d]=%d\n", i, arr[i]);
//             }
//         }
//         sbrk(-PGSIZE * 20);
//         exit(0);
//     }
//     else
//     {
//         wait(0);
//     }
// }


// void alloc_test()
// {
//     char *alloc = malloc(20 * PGSIZE);
//     for (int i = 0; i < 20; i++)
//     {
//         alloc[i * PGSIZE] = '0' + i;
//         printf("added i : %d\n", i);
//     }
//     for (int i = 0; i < 20; i++)
//         printf("alloc[%d] = %c\n", i * PGSIZE, alloc[i * PGSIZE]);
//     free(alloc);
//     printf("--------- allocTest finished ---------\n");
// }

// void scfifo_test()
// {
//     if (fork() == 0)
//     {
//         int queue[18] = {0 , 0 , 2 ,2 , 4 ,4 ,6 ,6 ,8, 8 ,10 ,10 , 17 ,16 , 15 ,14 ,13 ,12};
//         char *arr = sbrk(18 * PGSIZE);
// //        debug_selectors(1);
//         for (int i = 0; i < 18; i++)
//         {
//             arr[(queue[i]) * PGSIZE] = 'a';
//         }

//         printf("--------- scfifo Test finished --------- \n");
//         printf("should have swaped out the following pages - 0, 1, 2, 3, 4, 5\n");
//         exit(0);
//     }
//     else
//     {
//         wait(0);
//     }
// }

// void ageing_algotest()
// {
//     if (fork() == 0)
//     {
//         int queue[33] = {
//             0, 0, 0 ,0 ,0 ,0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2 ,2 ,2 ,2, 2, 2, 2 
//             // , 3 ,3 ,3 , 3 ,3 ,3 , 3 ,3 ,3  , 4,4,4,4, 4,4,4,4 , 5,5 ,5 ,5 , 5, 5 ,5 , 6 ,6, 6, 6 ,6, 6 , 7 ,7 ,7 ,7 ,7,
//             // 8, 8 , 8 ,8, 9 ,9 , 9 ,10 , 10 , 11 , 12 ,12 ,13, 13, 14, 14 ,15, 15 ,16, 16 , 17 ,17
//             };

//         char *arr = sbrk(18 * PGSIZE);
// //        debug_selectors(1);
//         sleep(15);
//         for (int i = 0; i < 33; i++)
//         {
//             // printf("i : %d, queue[%d] = %d\n", i, i, queue[i]);
//             arr[(queue[i]) * PGSIZE] = 'a';
            
//         }

//         printf("--------- ageing_algotest finished --------- \n");
//         printf("should have swaped out the following pages - 0, 1, 2, 3, 4, 5\n");
//         exit(0);
//     }
//     else
//     {
//         wait(0);
//     }
// }

// int NFUAAgingTesting()
// {
//     printf("--------- NFUAAgingTesting --------- \n");
//     int *memoryPages[20];

//     // allocate 12 page - 16 in ram and 0 in file
//     for (int i = 0; i < 12; i++)
//     {
//         memoryPages[i] = (int *)sbrk(PGSIZE);
//     }

//     // 3 youngest
//     for (int i = 0; i < 12; i++)
//     {
//         if (i != 3)
//             *memoryPages[i] = i;
//     }

//     // 2->3 is the youngest
//     for (int i = 0; i < 12; i++)
//     {
//         if (i != 3 && i != 2)
//             *memoryPages[i] = i;
//     }

//     memoryPages[12] = (int *)sbrk(PGSIZE);

//     *memoryPages[3] = -1;
//     printf("\n page[3]: %p should create seg fault \n", memoryPages[3]);

//     *memoryPages[12] = -1;
//     memoryPages[13] = (int *)sbrk(PGSIZE);

//     *memoryPages[2] = -1;
//     printf("\n page[3]: %p should create seg fault \n", memoryPages[2]);

//     sbrk(-14 * PGSIZE);
//     printf("--------- NFUAAgingTesting fINISHED --------- \n");
//     return 1;
// }

// int main(int argc, char *argv[])
// {
//     // swaping_test();
//     // sbrk_test();
//     //Dolav
//     // priority_test();
//     // test_mem_arr();
// #ifdef SCFIFO
//    scfifo_test();
// #endif
    
// #if defined(NFUA) || defined(LAPA)
// ageing_algotest();
// #endif
//      alloc_dealloc_test(); // Not passing todo: delte

//     exit(0);
//     return 0;
// }

// #include "kernel/param.h"
// #include "kernel/types.h"
// #include "kernel/stat.h"
// #include "user/user.h"
// #include "kernel/fs.h"
// #include "kernel/fcntl.h"
// #include "kernel/syscall.h"
// #include "kernel/memlayout.h"
// #include "kernel/riscv.h"

// void swaping_test()
// {
//     if (fork() == 0)
//     {
//         printf("-------------Task 1 Test-----------\n");
//         char *arr = (char *)sbrk(28 * PGSIZE); // create an array 28 pages long (proc starts with 4 pages) so we have 32 pages allocated
//         printf("Finished increments the program's data space by sbrk \n");
//         for (int i = 0; i < 28; i++)
//         {
//             printf("%d\n", i);
//             arr[(i * PGSIZE)] = i + '0'; //writes data to all pages
//         }
//         printf("Finished writing memory on all pages\n");
//         for (int j = 27; j >= 0; j--)
//         {
//             printf("%d\n", j);
//             arr[j * PGSIZE] = j + '0';
//         }

//         for (int i = 0; i < 28; i++)
//         {
//             if (arr[i * PGSIZE] != i + '0')
//             {
//                 printf("Failed, wrong value was written on page%d, %c\n", i, arr[i * PGSIZE]);
//                 break;
//             }
//         }
//         printf("Passed!\n");

//         printf("-------------Task 1 Test DONE-----------\n");
//         exit(0);
//     }
//     else
//     {
//         printf("-------------Parent starting-------------\n");
//         wait(0);
//     }
// }

// void sbrk_test() //Sbrk test
// {
//     if (fork() == 0)
//     {
//         printf("--------------------sbrk_test Started ----------------------\n");
//         printf("Allocating 28 pages\n");
//         if (fork() == 0)
//         {
//             for (int i = 0; i < 28; i++)
//             {
//                 printf("Calling sbrk number %d\n", i);
//                 sbrk(PGSIZE);
//             }
//             exit(0);
//         }
//         wait(0);
//         printf("--------------------Finished sbrk_test----------------------\n");
//         exit(0);
//     }
//     else
//     {
//         wait(0);
//     }
// }

// void alloc_dealloc_test()
// {
//     printf("--------- alloc dealloc test starts ---------\n");
//     if (!fork())
//     {
//         int *arr = (int *)(sbrk(PGSIZE * 20));
//         for (int i = 0; i < PGSIZE * 20 / sizeof(int); ++i)
//         {
//             arr[i] = 'a';
//         }
//         sbrk(-PGSIZE * 20);
//         printf("dealloc complete\n");
//         arr = (int *)(sbrk(PGSIZE * 20));
//         for (int i = 0; i < PGSIZE * 20 / sizeof(int); ++i)
//         {
//             arr[i] = 'b';
//         }
//         for (int i = 0; i < PGSIZE * 20 / sizeof(int); ++i)
//         {
//             if (i % PGSIZE == 0)
//             {
//                 printf("arr[%d]=%c\n", i, arr[i]);
//             }
//         }
//         sbrk(-PGSIZE * 20);
//         printf("--------------------Finished alloc dealloc test----------------------\n");
//         exit(0);
//     }
//     else
//     {
//         wait(0);
//     }
// }

// void alloc_test()
// {
//     printf("--------- allocTest starts ---------\n");
//     if (fork() == 0)
//     {
//         char *alloc = malloc(20 * PGSIZE);
//         for (int i = 0; i < 20; i++)
//         {
//             alloc[i * PGSIZE] = '0' + i;
//             printf("added i : %d\n", i);
//         }
//         for (int i = 0; i < 20; i++)
//             printf("alloc[%d] = %c\n", i * PGSIZE, alloc[i * PGSIZE]);
//         free(alloc);
//         printf("--------- allocTest finished ---------\n");
//         exit(0);
//     }
//     else
//     {
//         wait(0);
//     }
// }

// void scfifo_test()
// {
//     if (fork() == 0)
//     {
//         int queue[18] = {0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 17, 16, 15, 14, 13, 12};
//         char *arr = sbrk(18 * PGSIZE);
//         // debug_selectors(1);
//         for (int i = 0; i < 18; i++)
//         {
//             arr[(queue[i]) * PGSIZE] = 'a';
//         }

//         printf("--------- scfifo Test finished --------- \n");
//         exit(0);
//     }
//     else
//     {
//         wait(0);
//     }
// }

// void ageing_algotest()
// {
//     printf("--------------------ageing_algotest starts----------------------\n");

//     if (fork() == 0)
//     {
//         int queue[90] = {
//             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2,
//             2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7,
//             7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 10, 10, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17};

//         char *arr = sbrk(18 * PGSIZE);
//         // debug_selectors(1);
//         sleep(15);
//         for (int i = 0; i < 90; i++)
//         {
//             arr[(queue[i]) * PGSIZE] = 'a';
//         }

//         printf("--------- ageing_algotest finished --------- \n");
//         exit(0);
//     }
//     else
//     {
//         wait(0);
//     }
// }

// int main(int argc, char *argv[])
// {
//     swaping_test();
//     sbrk_test();
// #ifdef SCFIFO
//     scfifo_test();
// #endif
// #if defined(NFUA) || defined(LAPA)
//     ageing_algotest();
// #endif

//     alloc_dealloc_test();
//     exit(0);
//     return 0;
// }

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define PGSIZE 4096

/*
    struct page_access_info{
    uint64 page_address;             // Indetfire for pte
    uint access_counter;             //Indicates the number of access to the page; 
    uint64 loaded_at;                // Indicates the time the page was loaded to RAM; 
    int in_use ; 
    };
    Test Cases: 
        1- SCFIFO:
        * should return the first page loaded into ram, that its access bit -0 
            1- all pages with access_bit = 0            => should return p->ram_pages[i]
                                                            ram_pages[i]->loaded_at is minimal, at 1st iteration
            2- all pages with access_bit = 1            => should return p->ram_pages[i]
                                                            ram_pages[i]->loaded_at is minimal, at iteration 17
            3- all pages with access_bit = 1            => should return ram_pages[i] with max loaded_at 
                except from the last one
            4- order of saving to ram
                is corresponding to the array           => all 3 above should act the same
            5- order of saving to ram
                is not corresponding to the array       => all 3 above should act the same
            6- access pages 0-14 for 6 times            => should return page min_loaded_time(page i) 0<=i<15
                than access once page 15
        2- NFUA:
        * should return the page that was accessed last while considering the number of accesses
            1- all pages with 6 accesses                => should return the one was accessed first
            2- access pages 0-14 for 6 times            => should return page 15
                than access page 15
        3- LAPA:
        * should return the page that was least accessed, in case of equality, should return the one was accessed earlier
            1- all pages with 6 accesses                => should return the one was accessed first
            2- access pages 0-14 for 6 times            => should return page min_access_time(page_i) 0<=i<15
                than access once page 15
            
*/

// tries to access a page that is not belong to it
int test1(void){
    char *memo = malloc(PGSIZE*16);
    int i,j;
    int NUM_ITER = 20;

    for(j = 0 ; j < NUM_ITER ; j++){
        for(i = 0 ; i < 16 ; i++){
            memo[i*PGSIZE] = (char)j+i;
            memo[5*PGSIZE] = (char)j+i;
            memo[6*PGSIZE] = (char)j+i;
            memo[7*PGSIZE] = (char)j+i;
        }
    }
    uint all_pass = 1;
    for(i = 0 ; i < 16 ; i++){
        if(i >=5 && i<=7){
            all_pass &= (memo[i*PGSIZE] == (char)15+(NUM_ITER-1));
        }
        else{
            all_pass &= (memo[i*PGSIZE] == (char)i+(NUM_ITER-1));
        }
    }
    free(memo);
    return all_pass;
}

int test_fork(void){
    char *memo = malloc(PGSIZE*16);
    int i,j;
    int NUM_ITER = 5;
    int offset = 5;
    int cpid1,cpid2,cret1,cret2;
    uint all_pass = 1;
    int flag = 1;

    // for(i = 0 ; i < 16 ; i++){
    //     printf("page %d at %p\n",i,&memo[PGSIZE*i]);
    // }
    for(j = 0 ; j < NUM_ITER ; j++){
        for(i = 0 ; i < 16 ; i++){
            memo[i*PGSIZE] = (char)j+i;
            memo[5*PGSIZE] = (char)j+i;
            memo[6*PGSIZE] = (char)j+i;
            memo[7*PGSIZE] = (char)j+i;
        }
    }
    // printf("%d started\n",getpid());
    if((cpid1 = fork()) < 0){
        printf("FAILED - 1st fork\n");
        return 0;
    }
    // printf("%d after 1\n",getpid());

    for(j = 0 ; j < NUM_ITER ; j++){
        for(i = 0 ; i < 16 ; i++){
            memo[i*PGSIZE] = (char)j+i+offset;
            memo[5*PGSIZE] = (char)j+i+offset;
            memo[6*PGSIZE] = (char)j+i+offset;
            memo[7*PGSIZE] = (char)j+i+offset;
        }
    }
    
    if((cpid2 = fork()) < 0){
        printf("FAILED - 2nd fork\n");
        return 0;
    }    
    // printf("%d after 2\n",getpid());

    for(j = 0 ; j < NUM_ITER ; j++){
        for(i = 0 ; i < 16 ; i++){
            // if(getpid() == 4)
            //     printf("i:%d, %p <- %d\n",i,&memo[i*PGSIZE],(char)j+i+(offset*2));
            memo[i*PGSIZE] = (char)j+i+(offset*2);
            memo[5*PGSIZE] = (char)j+i+(offset*2);
            memo[6*PGSIZE] = (char)j+i+(offset*2);
            memo[7*PGSIZE] = (char)j+i+(offset*2);
        }
    }

    for(i = 0 ; i < 16 ; i++){
        if(i >=5 && i<=7){
            all_pass &= (memo[i*PGSIZE] == (char)15+(NUM_ITER-1)+(offset*2))|flag;
            // if(getpid() == 4)
            //     printf("i:%d -- %d ?= %d\n",i,memo[i*PGSIZE],(char)(15+(NUM_ITER-1)+(offset*2)));
        }
        else{
            all_pass &= (memo[i*PGSIZE] == (char)(i+(NUM_ITER-1)+(offset*2)))|flag;
            // if(getpid() == 4)
            //     printf("i:%d -- %d ?= %d\n",i,memo[i*PGSIZE],(char)(i+(NUM_ITER-1)+(offset*2)));

        }
    }

    // for tree processes- 
    //    3
    //  5   4
    //        6
    
    // case 3,5
    if(cpid1 != 0){
        // case 3
        if(cpid2 != 0){
            wait(&cret1);
            wait(&cret2);
        }
        // case 5
        else{
            exit(all_pass);
        }
    }
    // case 6,4
    else{
        // case 4
        if(cpid2 != 0){
            wait(&cret2);
            exit(all_pass && cret2);
        }
        // case 6
        else{
            exit(all_pass);
        }
    }
    free(memo);
    // printf("%d got to return ap:%d cret1:%d cret2:%d\n",getpid(),all_pass,cret1,cret2);
    return all_pass && cret1 && cret2;
}

struct test {
    int (*f)(void);
    char *s;
  } tests[] = {
    {test1,"test1"},
    {test_fork, "test_fork"},
    { 0, 0}, 
  };

int main(void){
    int res;
    for (struct test *t = tests; t->s != 0; t++) {
        printf("----------- test - %s -----------\n", t->s);
        res = t->f();
        if(res){
            printf("----------- %s PASSED -----------\n", t->s);
        }
        else{
            printf("----------- %s FAILED -----------\n", t->s);
        }
    }

    exit(0);
}