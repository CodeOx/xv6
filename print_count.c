#include "types.h"
#include "user.h"
#include "date.h"

int
main(int argc ,char* argv[])
{
	ps();
	int id=create_container();
	// id++;
	sleep(100);
	join_container(id);
	sleep(100);
	ps();
	// print_count () ;
	printf(1,"hellooooo\n");
	exit () ;
}
