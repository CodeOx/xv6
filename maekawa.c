#include "types.h"
#include "stat.h"
#include "user.h"

#define MSGSIZE 100
#define P 9
#define P1 8
#define P2 1
#define P3 0

struct message{
	int pid;
	int time;	//timestamp
	char type;	//R:request, L:locked, F:failed, I:inquire, Q:relenquish, S:released
};

int *pid;
int *req_set;
int req_size;
int my_id;

volatile int numRelease;
volatile int state;		//0: unlocked, 1: locked
volatile char* msgShared;
volatile char* msgShared_temp;
volatile struct message *m_recv;
volatile struct message *m_send;

//signal handler
void sig_handler(char* msg){
	int i = MSGSIZE;
	msgShared_temp = msgShared;
	while(i--)
		*msgShared_temp++ = *msg++;
	m_recv = (struct message*)msgShared;
	switch(m_recv->type){
		case 'R': if(state == 0)
					state = 1;
				send_multi(my_id,req_set,(char*)m_send,1);
				break;
		default: break;
	}
	return;
}

void request_set(int pno){
	int row_size;
	switch(P){
		case 4: req_size = 3; row_size = 2; break;
		case 9: req_size = 5; row_size = 3; break;
		case 16: req_size = 7; row_size = 4; break;
		case 25: req_size = 9; row_size = 5; break;
		default: req_size = 1; row_size = 1; break;
	}
	req_set = (int*)malloc(req_size*sizeof(int));
	for(int i = 0; i < row_size; i++){
		req_set[i] = pid[((pno/row_size)*row_size) + i];
	}
	int k = row_size;
	for(int i = 0; i < row_size; i++){
		if(pno/row_size != i){
			req_set[k] = pid[(pno%row_size) + (i*row_size)];
			k++;
		}
	}
	return;
}

void acquire1(){
	struct message *m = (struct message*)malloc(MSGSIZE);
	m->pid = my_id;
	m->time = uptime();
	m->type = 'R';
	send_multi(my_id,req_set,(char*)m,req_size);
	free(m);
}

void release1(){

}

int main(int argc, char *argv[])
{
	numRelease = 0;
	pid = (int*)malloc(MSGSIZE);
	int P1id[P1], P2id[P2], P3id[P3];
	
	msgShared = (char*)malloc(MSGSIZE);
	m_send = (struct message*)malloc(MSGSIZE);
	state = 0;

	for(int i = 0; i< P1; i++){
		P1id[i] = fork();
		if(P1id[i] == 0){
			my_id = getpid();
			//set signal handler
			set_handle(sig_handler);
			recv(pid);
			//while(numRelease < P);
			exit();
		}
		pid[i] = P1id[i];
	}

	for(int i = 0; i < P2; i++){
		P2id[i] = fork();
		if(P2id[i] == 0){
			my_id = getpid();
			//set signal handler
			set_handle(sig_handler);
			recv(pid);
			request_set(P1 + i);
			acquire1();
			//printf(1, "%d acquired the lock at time %d\n", getpid(), uptime());
			sleep(200);	//sleep for 2 sec
			//printf(1, "%d released the lock at time %d\n", getpid(), uptime());
			release1();
			printf(1, "s:%d\n", state);			
			exit();
		}
		pid[P1+i] = P2id[i];
	}

	for(int i = 0; i < P3; i++){
		P3id[i] = fork();
		if(P3id[i] == 0){
			my_id = getpid();
			//set signal handler
			set_handle(sig_handler);
			recv(pid);
			request_set(P1 + P2 + i);
			acquire1();
			//printf(1, "%d acquired the lock at time %d\n", getpid(), uptime());
			//printf(1, "%d released the lock at time %d\n", getpid(), uptime());
			release1();
			exit();
		}
		pid[P1+P2+i] = P3id[i];
	}

	for(int i = 0; i < P; i++){
		send(getpid(), pid[i], pid);
	}

	for(int i = 0; i < P; i++){
		wait();
	}

	/*for(int i = 0; i < 3; i++){
		for(int j = 0; j < 3; j++){
			printf(1, "%d ", pid[i*3 + j]);
		}
		printf(1, "\n");
	}*/

	exit();
}