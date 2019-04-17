#include "lock.h"

#define MSGSIZE 8

volatile int num;
volatile char* msgShared;
volatile char* msgShared_temp;
struct spinlock lock;

//signal handler
//calling other funcions doesn't work inside signal handler (why? or maybe just printf doesn't work?)
//can't do syscall here
//can't use lock here, unaivailable lock doesn't return to scheduler
void test(char* msg){
	printf(1, "*****inside signal handler\n");
	//acquire(&lock, 1);
	msgShared_temp = msgShared;
	while((*msgShared_temp++ = *msg++) != 0);
	num = 9;
	//release(&lock, 1);
	return;
}

int main(void)
{
	initlock(&lock, "num");
	int cid = fork();
	if(cid==0){
		// This is child
		/****** set up signal handler ******/
		num = 7;
		msgShared = (char*)malloc(MSGSIZE);
		printf(1,"%d\n", num);
		set_handle(test);

		/* receive parent id */
		int *parid = (int*)malloc(MSGSIZE);
		recv(parid);

		/* send msg to parent to complete rendezvous */
		char* temp = (char*)malloc(MSGSIZE);
		temp = "A";
		send(getpid(),*parid,temp);
		printf(1, "child: handshake done\n");
			
		/* now do stuff */
		acquire(&lock, 2);
		while(num == 7);
		printf(1, "%d\n", num);
		printf(1, "%s\n", msgShared);
		release(&lock, 2);

		free(parid);
		free(temp);

	}else{
		// This is parent
		/***** rendezvous via handshake ******/
		/* sending its pid to child */
		int *parid = (int*)malloc(MSGSIZE);
		*parid = getpid();
		send(getpid(),cid,parid);

		/* recieve msg from child */
		char* temp = (char*)malloc(MSGSIZE);
		recv(temp);
		printf(1, "parent: handshake done\n");

		/* now do stuff */
		char *msg_child = (char *)malloc(MSGSIZE);
		msg_child = "pqrs";
		int a[1];
		a[0] = cid;
		send_multi(getpid(),a,msg_child,1);		
		
		free(parid);
		free(temp);
		
		wait();
	}
	
	exit();
}
