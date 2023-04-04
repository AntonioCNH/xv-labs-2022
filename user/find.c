#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *getname(char *path) {
    static char buf[DIRSIZ+1];
    char *p;
    for(p=path+strlen(path); p >= path && *p != '/'; p--);
    p++;
    if(strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    *(buf + strlen(p)) = '\0';
    // memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
    return buf;
}

void find(char *path, char *target) {
    int fd;
    struct stat st;
    struct dirent de;
    char p[strlen(path) + DIRSIZ];
    char *name;
    if((fd = open(path, 0)) <0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type) {
        case T_DEVICE:
        case T_FILE:
            name = getname(path);
            if((strcmp(name, target)) == 0) {
                printf("%s\n", path);
            }
            break;
        case T_DIR:
            while(read(fd, &de, sizeof(de)) == sizeof(de)) {
                if((de.inum == 0) || (strcmp(de.name, ".") == 0) || (strcmp(de.name, "..") == 0)) {
                    continue;
                }
                strcpy(p, path);
                *(p + strlen(path)) = '/';
                memmove(p + strlen(path) + 1, de.name, DIRSIZ);
                find(p, target);
            }
            break;
    }
    close(fd);

}

int main(int argc, char *argv[]) {
    if(argc < 3) {
        fprintf(2, "find: argc too small\n");
        exit(1);
    }
    char *path = argv[1];
    find(path, argv[2]);

    exit(0);
}