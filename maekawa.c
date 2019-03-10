#include "types.h"
#include "stat.h"
#include "user.h"

#define MSGSIZE 100
#define P 9
#define P1 2
#define P2 3
#define P3 4

volatile int numRelease;
int *pid;

void acquire1(){

}

void release1(){

}

int main(int argc, char *argv[])
{
	numRelease = 0;
	pid = (int*)malloc(MSGSIZE);
	int P1id[P1], P2id[P2], P3id[P3];
	
	for(int i = 0; i< P1; i++){
		P1id[i] = fork();
		if(P1id[i] == 0){
			recv(pid);
			//while(numRelease < P);
			exit();
		}
		pid[i] = P1id[i];
	}

	for(int i = 0; i < P2; i++){
		P2id[i] = fork();
		if(P2id[i] == 0){
			recv(pid);
			acquire1();
			printf(1, "%d acquired the lock at time %d\n", getpid(), uptime());
			sleep(200);	//sleep for 2 sec
			printf(1, "%d released the lock at time %d\n", getpid(), uptime());
			release1();
			exit();
		}
		pid[P1+i] = P2id[i];
	}

	for(int i = 0; i < P3; i++){
		P3id[i] = fork();
		if(P3id[i] == 0){
			recv(pid);
			acquire1();
			printf(1, "%d acquired the lock at time %d\n", getpid(), uptime());
			printf(1, "%d released the lock at time %d\n", getpid(), uptime());
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
	exit();
}