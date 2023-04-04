#include "defs.h"
#include "sysinfo.h"
#include "proc.h"
uint64 
sys_sysinfo(void)
{
    struct sysinfo info;
    struct proc *p = myproc();
    info.freemem = get_freemem();
    info.nproc = get_procnum();
    uint64 addr;
    argaddr(0, &addr);
    if(copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0) {
        return -1;
    } 
    return 0;

}



