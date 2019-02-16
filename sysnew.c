#include "types.h"
#include "defs.h"

#define N_SYSCALLS 23
#define N_MAXSYSCALLS 30

int trace_on = 0;				//used in syscall.c
int syscall_count[N_MAXSYSCALLS];  //used in syscall.c

char*
syscall_name(int num){
  switch(num){
    case 1: return (char*) "SYS_fork";
    case 2: return (char*) "SYS_exit";
    case 3: return (char*) "SYS_wait";
    case 4: return (char*) "SYS_pipe";
    case 5: return (char*) "SYS_read";
    case 6: return (char*) "SYS_kill";
    case 7: return (char*) "SYS_exec";
    case 8: return (char*) "SYS_fstat";
    case 9: return (char*) "SYS_chdir";
    case 10: return (char*) "SYS_dup";
    case 11: return (char*) "SYS_getpid";
    case 12: return (char*) "SYS_sbrk";
    case 13: return (char*) "SYS_sleep";
    case 14: return (char*) "SYS_uptime";
    case 15: return (char*) "SYS_open";
    case 16: return (char*) "SYS_write";
    case 17: return (char*) "SYS_mknod";
    case 18: return (char*) "SYS_unlink";
    case 19: return (char*) "SYS_link";
    case 20: return (char*) "SYS_mkdir";
    case 21: return (char*) "SYS_close";
    case 22: return (char*) "SYS_print_count";
	case 23: return (char*) "SYS_toggle";
    default: return (char*)"unknown sys call";
  }

}

int
sys_print_count(void)
{
  for(int i = 1; i <= N_SYSCALLS; i++){
  	cprintf("%s %d\n", syscall_name(i), syscall_count[i]);
  }
  return 0;
}

int
sys_toggle(void)
{
  trace_on = 1 - trace_on;
  if(trace_on){
	  for(int i = 0; i < N_MAXSYSCALLS; i++){
	  	syscall_count[i] = 0;
	  }
  }
  cprintf("sys_toggle !!!\n");
  return 0;
}