#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_ps(void)
{
  return ps();
}

int
sys_send(void)
{
  return send();
}

int
sys_recv(void)
{
  return recv();
}

int
sys_send_multi(void)
{
  return send_multi();
}

int
sys_set_handle(void)
{
  return set_handle();
}

int
sys_set_barrier(void)
{
  return set_barrier();
}

int
sys_barrier(void)
{
  return barrier();
}
int 
sys_create_container(void)
{
  int a=fork();
  a++;
  return create_container();
}

int 
sys_join_container(void)
{
  int cid;
  if (argint(0,&cid)<0)
  {
    return -1;
  }
  join_container(cid);
  return 0;
}


int 
sys_leave_container(void)
{
  // int cid;
  // if (argint(0,&cid)<0)
  // {
  //   return -1;
  // }
  leave_container();
  return 0;
}

int 
sys_destroy_container(void)
{
  int cid;
  if (argint(0,&cid)<0)
  {
    return -1;
  }
  destroy_container(cid);
  return 0;
}



//file handling in destroy container
//ps then send signal, make sure that kernel does not di that depending on containerid