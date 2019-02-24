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
			
			if(type == 1){
				//rendezvous here before continuing
				char* temp = (char*)malloc(MSGSIZE);
				recv(temp);
				free(temp);

				float* mean = (float*)malloc(sizeof(float));
				*mean = 4;

				//float* sum_sq = (float*)malloc(sizeof(float));
				int* sum_sq = (int*)malloc(sizeof(int));
				for(int j = start; j <= end; j++)
					*sum_sq += (arr[j] - *mean) * (arr[j] - *mean);

				printf(1, "sent: %d\n", *sum_sq);
				send(getpid(),par_id,sum_sq);	
				
				free(mean);
				free(sum_sq);
			}

			exit();
		}
  	}

  	int *msg = (int*)malloc(sizeof(int));
  	for(int i = 0; i < num_child; i++){
		recv(msg);
		tot_sum += msg[0];
  	}
  	free(msg);

  	//multicast variance : only for type 1
  	if(type==1){
  		//rendezvoud message to all children (via unicast since blocking is required)
  		char* temp = (char*)malloc(1);
  		temp = "R";
  		for(int i = 0; i < num_child; i++){
  			printf(1, "cid: %d\n", cid[i]);
  			send(par_id, cid[i], temp);
  		}

	  	//float mean = ((float)tot_sum)/size;
	  	
	  	int *msg = (int*)malloc(sizeof(int));
	  	for(int i = 0; i < num_child; i++){
			recv(msg);
			printf(1, "received: %d\n", *msg);
			variance += msg[0];
	  	}

	  	free(temp);
	  	free(msg);
	}

	variance /= size;

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
