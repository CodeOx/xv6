#include "types.h"
#include "user.h"
#include "date.h"

int
main(int argc ,char* argv[])
{
	ps();
	int id=create_container();
	join_container(id);
	ps();
	exit () ;
}
