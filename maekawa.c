#include "types.h"
#include "stat.h"
#include "user.h"

#define MSGSIZE 100
#define p 9
#define P1 2
#define P2 3
#define P3 4

volatile int numRelease;

void acquire(){

}

void release(){

}

int main(int argc, char *argv[])
{
	numRelease = 0;

	int P1id[P1], P2id[P2], P3id[P3];
	
	for(int i = 0; i< P1; i++){
		P1id[i] = fork();
		if(P1id[i] == 0){
			while(numRelease < P);
			exit();
		}
	}

	for(int i = 0; i < P2; i++){
		P2id[i] = fork();
		if(P2id[i] == 0){
			acquire();
			printf(1, "%d acquired the lock at time %d\n", getpid(), 1);
			//sleep for 2 sec
			printf(1, "%d released the lock at time %d\n", getpid(), 1);
			release();
			exit();
		}
	}

	for(int i = 0; i < P3; i++){
		P3id[i] = fork();
		if(P3id[i] == 0){
			acquire();
			printf(1, "%d acquired the lock at time %d\n", getpid(), 1);
			printf(1, "%d released the lock at time %d\n", getpid(), 1);
			release();
			exit();
		}
	}

	for(int i = 0; i < P; i++){
		wait();
	}
	exit();
}