#include "types.h"
#include "stat.h"
#include "user.h"

#define MSGSIZE 8

void test(){
	return;
}

int main(void)
{
	printf(1,"%s\n","IPC Test case");
	
	int cid = fork();
	if(cid==0){
		// This is child
		/****** set up signal handler ******/
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
		msg_child = "P";
		int a[1];
		a[0] = cid;
		send_multi(getpid(),a,msg_child,1);		
		//printf(1,"1 PARENT: msg sent is: %s \n", msg_child );
		
		free(parid);
		free(temp);
	}
	
	exit();
}
