#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// #define LAB_PGTBL
#ifdef LAB_PGTBL
// returns -1 if requested number of pages is more than MAX_PGACCESS_PAGES
int
sys_pgaccess(void)
{
  uint64 addr, mask;
  argaddr(0, &addr);
  int num;
  argint(1, &num);
  argaddr(2, &mask);
  if (num > MAX_PGACCESS_PAGES){
    return -1;
  }
  uint64 localmask = 0;
  pagetable_t pagetable = myproc()->pagetable;
  for (int i=0;i<num;i++){
    pte_t *pte = walk(pagetable, addr + PGSIZE*i, 0);
    // printf("HI\n");
    if ((*pte) & PTE_A){
      localmask |= (1 << i);
      (*pte) &= (~PTE_A);
    }
  }
  copyout(pagetable, mask, (char*)&localmask,num/8);
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
