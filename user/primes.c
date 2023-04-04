#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM 34

void primes(int *get, int count) {
   // printf("pid = %d, count = %d\n", getpid(), count);
    if(count == 0) {
        exit(0);
    }
    int p[2];
    if(pipe(p) < 0) {
        fprintf(2, "primes: pipe error\n");
        exit(1);
    }
    int pid = fork();
    if(pid > 0) {
        close(p[0]);    //close read 
        write(p[1], get, count * sizeof(int));
        close(p[1]);
        wait(0);
        // printf("exit process: %d\n", getpid());
        exit(0);
    } else if(pid == 0) {
        close(p[1]);    //close write 
        int num = 0;
        int first = 0;
        int i = 0;
        while(read(p[0], &num, sizeof(int)) != 0) {
            if(first == 0) {
                first = num;
                printf("prime %d\n",first);
            }
            if(num % first != 0) {
                get[i] = num;
                i++;
            }
        }
        
        primes(get, i);
       // printf("exit process: %d\n", getpid());
        exit(0);
    }
    printf("primes: fork() error.\n");
}

int main(int argc, char *argv[]) {
    int p[2];
    if(pipe(p) < 0) {
        fprintf(2, "primes: pipe error.\n");
        exit(1);
    }
    int nums[NUM];
    int first = 2;
    printf("prime: %d\n", first);
    int j = 0;
    for(int i = 0; i < NUM; i++) {
        if((i + 2) % first == 0){
            continue;
        } 
        nums[j] = i + 2;
        j++;
    }
    primes(nums, j);
    exit(0);
}