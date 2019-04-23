#include "types.h"
#include "user.h"
#include "date.h"
#include "fcntl.h"

int
main(int argc ,char* argv[])
{
	ps();
	int cid=create_container();
	/*int fd = open("test_fil", O_CREATE);
	close(fd);*/

	int child = fork();
	if(child == 0){
		join_container(cid);

		int fd = open("test_fil", O_CREATE);
		close(fd);
		
		// fd = open("test_fil", O_WRONLY);
		// close(fd);
		
		printf(1, "fd: %d\n", fd);
		
		int c = fork();
		if(c == 0){
			printf(1, "ls inside cont 0\n");
			char *argv[] = {"ls", 0};
			exec("/ls",argv);
			printf(1, "child : ls exec failed\n");
		}
		wait();
		printf(1, "ls inside cont 1\n");
		char *argv[] = {"ls", 0};
		exec("/ls",argv);
		printf(1, "child : ls exec failed\n");
		exit();
	}

	wait();
	// unlink("test_fil");
	// unlink("test_fil_1");
	exit();
}
