#include "types.h"
#include "user.h"
#include "date.h"

int
main(int argc ,char* argv[])
{
	ps();
	int id=create_container();
	int child = fork();
	if(child == 0){
		join_container(id);
		ps();
		char *argv[] = {"ls", 0};
		exec("/ls",argv);
		printf(1, "child : ls exec failed\n");
		exit();
	}
	wait();
	exit () ;
}
