#include "types.h"
#include "defs.h"

#define N_SYSCALLS 29
#define N_MAXSYSCALLS 30

int trace_on = 0;				//used in syscall.c
int syscall_count[N_MAXSYSCALLS];  //used in syscall.c

char*
syscall_name(int num){
  switch(num){
    case 1: return (char*) "sys_fork";
    case 2: return (char*) "sys_exit";
    case 3: return (char*) "sys_wait";
    case 4: return (char*) "sys_pipe";
    case 5: return (char*) "sys_read";
    case 6: return (char*) "sys_kill";
    case 7: return (char*) "sys_exec";
    case 8: return (char*) "sys_fstat";
    case 9: return (char*) "sys_chdir";
    case 10: return (char*) "sys_dup";
    case 11: return (char*) "sys_getpid";
    case 12: return (char*) "sys_sbrk";
    case 13: return (char*) "sys_sleep";
    case 14: return (char*) "sys_uptime";
    case 15: return (char*) "sys_open";
    case 16: return (char*) "sys_write";
    case 17: return (char*) "sys_mknod";
    case 18: return (char*) "sys_unlink";
    case 19: return (char*) "sys_link";
    case 20: return (char*) "sys_mkdir";
    case 21: return (char*) "sys_close";
    case 22: return (char*) "sys_print_count";
	case 23: return (char*) "sys_toggle";
    default: return (char*)"unknown sys call";
  }

}

int
sys_print_count(void)
{
  for(int i = 1; i <= N_SYSCALLS; i++){
    if(syscall_count[i] > 0)
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
  return 0;
}

int 
sys_add(void)
{
	int input1, input2;
	if(argint(0, &input1) < 0)
		return -1;
	if(argint(1, &input2) < 0)
		return -1;
	return input1 + input2;
}