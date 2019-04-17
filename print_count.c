#include "types.h"
#include "user.h"
#include "date.h"

int
main(int argc ,char* argv[])
{
	ps();
	int id=create_container1();
	join_container(id);
	ps();
	sleep(100);
	exit() ;
}
