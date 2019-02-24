#include "types.h"
#include "stat.h"
#include "user.h"

#define MSGSIZE 8

volatile int num;

void test(){
	printf(1, "*****inside signal handler\n");
	num = 9;
	return;
}

int main(void)
{
	int cid = fork();
	if(cid==0){
		// This is child
		/****** set up signal handler ******/
		num = 7;
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
		printf(1,"%d\n", num);
		printf(1,"%d\n", num);
		printf(1,"%d\n", num);
		printf(1,"%d\n", num);
		printf(1,"%d\n", num);
		printf(1,"%d\n", num);		
		free(parid);
		free(temp);

	}else{
		// This is parent
		printf(1,"handle ptrorig: %x\n", test);
		printf(1,"value handle ptrorig: %x\n", *test);
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
		msg_child = "P";
		int a[1];
		a[0] = cid;
		send_multi(getpid(),a,msg_child,1);		
		
		free(parid);
		free(temp);
		
		wait();
	}
	
	exit();
}
