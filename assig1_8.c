#include "types.h"
#include "stat.h"
#include "user.h"

#define MSGSIZE 8

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

  	int num_child = 7;	//number of child proceses
  	int cid[num_child];
  	int block = size/num_child;
  	int par_id = getpid();
  	num = 0;
  	msgShared = (char*)malloc(MSGSIZE);

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
				//set signal handler
				set_handle(sig_handler);

				int* msg = (int*)malloc(sizeof(int));
				*msg = par_sum;
				send(getpid(),par_id,msg);
				free(msg);	

				while(num == 0);

				float* mean = (float*)malloc(sizeof(float));
				*mean = *(float*)msgShared;

				float* sum_sq = (float*)malloc(sizeof(float));
				//int* sum_sq = (int*)malloc(sizeof(int));
				for(int j = start; j <= end; j++)
					*sum_sq += ((float)arr[j] - *mean) * ((float)arr[j] - *mean);

				//printf(1, "sent: %d\n", (int)*sum_sq);
				send(getpid(),par_id,(char*)sum_sq);	
				
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

  		float* mean = (float*)malloc(sizeof(float));
	  	*mean = tot_sum/size;
	
		char *msg = (char*)malloc(sizeof(int));				
		for(int i = 0; i < num_child; i++){
			recv(msg);
		}

		send_multi(par_id,cid,(char*)mean,num_child);	

	  	for(int i = 0; i < num_child; i++){
			recv(msg);
			//printf(1, "received: %d\n", (int)*(float*)msg);
			variance += *(float*)msg;
	  	}

	  	free(mean);
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
