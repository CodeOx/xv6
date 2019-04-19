#include "types.h"
#include "user.h"
#include "date.h"

int
main(int argc ,char* argv[])
{
	ps();
	int cid=create_container();
	
	int child = fork();
	if(child == 0){
		join_container(cid);
		ps();
		int fd = mkdir("yohoooooooo");
		printf(1, "fd: %d\n", fd);
		char *argv[] = {"ls", 0};
		exec("/ls",argv);
		printf(1, "child : ls exec failed\n");
		exit();
	}
	
	wait();
	//unlink("yohoooooooo");
	exit();
}
