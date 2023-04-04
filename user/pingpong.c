#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int p1[2];
    int p2[2];
    if(pipe(p1) < 0) {
        fprintf(2, "pingpong: pipe1 error.\n");
        exit(1);
    }
    if(pipe(p2) < 0) {
        fprintf(2, "pingpong: pipe2 error.\n");
        exit(1);
    }
    int pid = fork();
    if(pid > 0) {
        close(p1[0]);
        close(p2[1]);
        char chp = 'p';
        write(p1[1], &chp, sizeof(char));
        read(p2[0], &chp, sizeof(char));
        printf("%d: received pong\n", getpid());
        exit(0);
    } else if(pid == 0){
        close(p1[1]);
        close(p2[0]);
        char chc = 'c';
        read(p1[0], &chc, sizeof(char));
        printf("%d: received ping\n", getpid());
        write(p2[1], "c", sizeof(char));
        exit(0);
    } else {
        fprintf(2, "pingpong: fork error\n");
        exit(1);
    }
}