#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

char buf[1024];

char *read_line(int fd) {
    char ch = 'c';
    int i = 0;
    while(read(fd, &ch, sizeof(ch)) != 0) {
        if(ch == '\n') {
            buf[i] = '\0';
            return buf;
        } 
        buf[i] = ch;
        i++;
    }
    return 0x0;
}

int main(int argc, char *argv[]) {
    char *new_argv[MAXARG];
    if(argc < 2) {
        fprintf(2, "xargs: too few arguments\n");
        exit(1);
    }
    int i = 1;
    for(i = 1; i < argc; i++) {
        new_argv[i-1] = argv[i];
    }
    while(1) {
        char *line = read_line(0);
        if(!line) {
            break; 
        } 
        new_argv[i-1] = line;
        int pid = fork();
        if(pid > 0) {
            wait(0);
        } else if(pid == 0) {
            exec(new_argv[0], new_argv);
            exit(0);
        } else {
            fprintf(2, "xargs: fork failed.\n");
            exit(1);
        }
    }
    exit(0);
} 