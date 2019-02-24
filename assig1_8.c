#include "types.h"
#include "stat.h"
#include "user.h"

#define MSGSIZE 8

int
main(int argc, char *argv[])
{
	if(argc< 2){
		printf(1,"Need type and input filename\n");
		exit();
	}
	char *filename;
	filename=argv[2];
	int type = atoi(argv[1]);
	printf(1,"Type is %d and filename is %s\n",type, filename);

	int tot_sum = 0;	
	float variance = 0.0;

	int size=1000;
	short arr[size];
	char c;
	int fd = open(filename, 0);
	for(int i=0; i<size; i++){
		read(fd, &c, 1);
		arr[i]=c-'0';
		read(fd, &c, 1);
	}	
  	close(fd);
  	// this is to supress warning
  	printf(1,"first elem %d\n", arr[0]);
  
  	//----FILL THE CODE HERE for unicast sum and multicast variance

  	int num_child = 2;	//number of child proceses
  	int cid[num_child];
  	int block = size/num_child;
  	int par_id = getpid();

  	//unicast sum : common for both type 0 and 1
  	for(int i = 0; i < num_child; i++){
  		cid[i] = fork();
  		if(cid[i]==0){
			// This is child
			int par_sum = 0; //partial sum
			int start = i*block;
			int end = (i+1)*block - 1;
			if(i == num_child-1){
				end = size-1;
			}
			for(int j = start; j <= end; j++)
				par_sum += arr[j];
			
			int* msg = (int*)malloc(sizeof(int));
			*msg = par_sum;
			send(getpid(),par_id,msg);
			
			free(msg);
			exit();
		}
  	}

  	for(int i = 0; i < num_child; i++){
  		int *msg = (int*)malloc(sizeof(int));
		recv(msg);
		tot_sum += msg[0];
		free(msg);
  	}

  	for(int i = 0; i < num_child; i++){
  		wait();
  	}

  	//------------------

  	if(type==0){ //unicast sum
		printf(1,"Sum of array for file %s is %d\n", filename,tot_sum);
	}
	else{ //mulicast variance
		printf(1,"Variance of array for file %s is %d\n", filename,(int)variance);
	}
	exit();
}
