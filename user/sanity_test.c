#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"

void 
paging_framework_test(){
    int flag = 1;
    printf("task1: paging framework\n");
    if(fork() == 0){
        
        char *pages = (char *)sbrk(28 * PGSIZE);            // proc has 32 pages and init with 4 pages, create 28 more pages to have 32 pages in total
        
        printf("all 28 pages created\n");
        
        for (int i = 0; i < 28; i++)
            pages[(i * PGSIZE)] = (i + 65) + '\0';          // write data to the 28 pages we have just created
        
        printf("write to the 28 pages done\n");

        for(int i = 0; i < 28; i++)
            if(pages[i * PGSIZE] != (i + 65)){              // validate all values of data on pages are correct
                printf("page: %d, expected: %c, actual: %c\n", i, (i + 65), pages[i * PGSIZE]);
                flag = 0;
            }

        if(flag)
            printf("paging framework is working\nall values are correct\ntask1: test completed successfully\n");
        
        exit(0);
    }
    else{
        wait(0);
    }
}

void 
sbrk_test(){
    if(fork() == 0){
        printf("sbrk test:\ntry to allocate 28 pages\n");
        if(fork() == 0){
            for (int i = 0; i < 28; i++){
                sbrk(PGSIZE);
            }
            exit(0);
        }
        
        wait(0);
        printf("all 28 pages allocated successfully\n");
        exit(0);
    }
    else{
        wait(0);
    }
}

void 
allocate_then_dealocate_test(){
    printf("allocate then dealocate test:\n");
    if(!fork()){
        int *arr = (int *)(sbrk(PGSIZE * 20));
        
        printf("allocate completed successfully\n");
        
        for(int i = 0; i < PGSIZE * 20 / sizeof(int); ++i) // init data with 0 on all the pages
            arr[i] = 0;
        
        sbrk(-PGSIZE * 20);                                // try to deallocate all the pages
        
        printf("deallocate completed successfully\n");
        
        arr = (int *)(sbrk(PGSIZE * 20));
        
        printf("allocate completed successfully\n");

        for(int i = 0; i < PGSIZE * 20 / sizeof(int); ++i) // init data with 1 on all the pages
            arr[i] = 1;
        
        for(int i = 0; i < PGSIZE * 20 / sizeof(int); ++i)
            if(i % PGSIZE == 0)
                printf("arr[%d]=%d\n", i, arr[i]);
            
        
        sbrk(-PGSIZE * 20);

        printf("deallocate completed successfully\nallocate then dealocate test test completed successfully\n");

        exit(0);
    }
    else{
        wait(0);
    }
}

void 
scfifo_page_replacement_test(){
    printf("scfifo page replacement test: ");
    if(fork() == 0){
        int queue[10] = {0, 0, 2, 2, 4, 4, 9, 8, 7, 6};
        char *arr = sbrk(10 * PGSIZE);
        for (int i = 0; i < 9; i++)
            arr[(queue[i]) * PGSIZE] = (i + 65) + '\0';;

        printf("scfifo page replacement test completed successfully\n");
        exit(0);
    }
    else{
        wait(0);
    }
}

void
ageing_page_replacement_test(){
    printf("ageing page replacement test: ");
    if(fork() == 0){
        int queue[50] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
            3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 10, 11, 12, 12, 11, 10};

        char *arr = sbrk(13 * PGSIZE);
        sleep(10);
        for(int i = 0; i < 50; i++)
            arr[(queue[i]) * PGSIZE] = (i + 65) + '\0';;

        printf("ageing page replacement test completed successfully\n");
        exit(0);
    }
    else{
        wait(0);
    }
}

int main(int argc, char *argv[]){
    paging_framework_test();
    sbrk_test();
    #ifdef SCFIFO
        scfifo_page_replacement_test();
    #endif
    #if defined(NFUA) || defined(LAPA)
        ageing_page_replacement_test();
    #endif
    allocate_then_dealocate_test();
    exit(0);
    return 0;
}