#include "types.h"
#include "stat.h"
#include "user.h"

#define MSGSIZE 100
#define MAXPROGS 50
#define SWITCHTIME 100

struct message{
	int pid;	//process id
	int num;	//system call
	char name[16];	//process name
};
// num:
// 0 >> exit
// 1 >> join container 
// 2 >> leave container
// 3 >> ps

// Per-process state
struct proc {
//  enum procstate state;        // Process state
	int pid;                     // Process ID
	char name[16];               // Process name (debugging)
};

volatile char* msgShared;
volatile char* msgShared_temp;
volatile int msgRecvd;

struct proc user_programs[MAXPROGS]; 
int pid_current_proc;
int time_last_switch;
int time_current;

//signal handler
//calling other funcions doesn't work inside signal handler (why? or maybe just printf doesn't work?)
//can't do syscall here
//can't use lock here, unaivailable lock doesn't return to scheduler
void sig_handler(char* msg){
	int i = MSGSIZE;
	msgShared_temp = msgShared;
	while(i--)
		*msgShared_temp++ = *msg++;
	msgRecvd = 1;
	return;
}

void ps1(){
	for(int i = 0; i < MAXPROGS; i++){
		if(user_programs[i].pid != -1){
			printf(1, "pid:%d name:%s\n", user_programs[i].pid, user_programs[i].name);
		}
	}
}

void add_process(int pid, char* pname){
	for(int i = 0; i < MAXPROGS; i++){
		if(user_programs[i].pid != -1){
			user_programs[i].pid = pid;
			for(int j = 0; j < 16; j++)
				user_programs[i].name[j] = pname[j];
			break;
		}
	}	
}

void delete_process(int pid){
	for(int i = 0; i < MAXPROGS; i++){
		if(user_programs[i].pid == pid){
			user_programs[i].pid = -1;
			break;
		}
	}	
}

void switch1(){
	
}

int
main(int argc, char *argv[])
{
	msgShared = (char*)malloc(MSGSIZE);
	struct message *m;
	set_handle(sig_handler);
	msgRecvd = 0;

	for(int i = 0; i < MAXPROGS; i++){
		user_programs[i].pid = -1;
	}

	time_last_switch = uptime();

	while(1){
		if(msgRecvd == 1){
			m = (struct message*)msgShared;
			switch(m->num){
				case 0: exit(); break;
				case 1: add_process(m->pid, m->name); break;
				case 2: delete_process(m->pid); break;
				case 3: ps(); break;
				default: break;
			}
			msgRecvd = 0;
		}

		time_current = uptime();
		if(time_current - time_last_switch > SWITCHTIME){
			switch1();
			time_last_switch = time_current;
		}
	}

	exit();
}
