#include "types.h"
#include "user.h"
#include "date.h"

int
main(int argc ,char* argv[])
{
	if(argc < 3){
		printf(1, "user_add: too few arguments\n");	
	}
	else if(argc > 3){
		printf(1, "user_add: too many arguments\n");	
	}
	else{
		printf(1, "%d\n", add(atoi(argv[1]), atoi(argv[2])));
	}
	exit () ;
}
