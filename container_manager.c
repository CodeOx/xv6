#include "types.h"
#include "stat.h"
#include "user.h"

#define MSGSIZE 100

volatile int num;
volatile char* msgShared;
volatile char* msgShared_temp;

//signal handler
//calling other funcions doesn't work inside signal handler (why? or maybe just printf doesn't work?)
//can't do syscall here
//can't use lock here, unaivailable lock doesn't return to scheduler
void sig_handler(char* msg){
	int i = MSGSIZE;
	msgShared_temp = msgShared;
	while(i--)
		*msgShared_temp++ = *msg++;
	num = 1;
	return;
}

int
main(int argc, char *argv[])
{
	exit();
}
