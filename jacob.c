#include "types.h"
#include "stat.h"
#include "user.h"

#define MSGSIZE 100

#define N 20
#define E 0.00001
#define T 100.0
#define P 6
#define L 20000

volatile int num;
volatile char* msgShared;
volatile char* msgShared_temp;

struct Data{
	int line_no;
	char padding[MSGSIZE - 4 - (4*N)];
	float line[N];
};

//signal handler
void sig_handler(char* msg){
	int i = MSGSIZE;
	msgShared_temp = msgShared;
	while(i--)
		*msgShared_temp++ = *msg++;
	num = 1;
	return;
}

float fabsm(float a){
	if(a<0)
	return -1*a;
return a;
}
int main(int argc, char *argv[])
{
	float diff;
	int i,j;
	float mean;
	float u[N][N];
	float w[N][N];

	int count=0;
	mean = 0.0;
	for (i = 0; i < N; i++){
		u[i][0] = u[i][N-1] = u[0][i] = T;
		u[N-1][i] = 0.0;
		mean += u[i][0] + u[i][N-1] + u[0][i] + u[N-1][i];
	}
	mean /= (4.0 * N);
	for (i = 1; i < N-1; i++ )
		for ( j= 1; j < N-1; j++) u[i][j] = mean;
	
	int pid[P];
  	int block = (N-2)/P;
  	int par_id = getpid(); pid[0] = par_id;
	int p_no;
	/**************fork here****************/
	for(p_no = 1; p_no < P; p_no++){
		pid[p_no] = fork();
		if( pid[p_no] == 0)
			break;
		printf(1, "%d\n", pid[p_no]);
	}
	int my_id = getpid();
	int start = p_no*block + 1;
	int end = (p_no+1)*block;
	if(p_no == P-1){
		end = N-2;
	}
	if(p_no == P){
		start = 1;
		end = block;
	}
	//set signal handler
	set_handle(sig_handler);
	msgShared = (char*)malloc(MSGSIZE);

	/********** set prev and next ***********/
	/****************************************/
	int prev=-1, next=-1;
	int* d1 = (int*)malloc(MSGSIZE);
	if(p_no == P){
		next = pid[1];
		//send prev to children
		for(int px = 1; px < P; px++){
			*d1 = pid[px-1];
			send(par_id, pid[px], d1);
		}
		for(int px = 1; px < P; px++){
			recv(d1);
		}
		//send next to children
		for(int px = 1; px < P-1; px++){
			*d1 = pid[px+1];
			send(par_id, pid[px], d1);
		}
	} else {
		//receive pid of prev from parent
		recv(d1);
		prev = *d1;
		//reply d
		send(my_id, par_id, d1);
		//receive pid of next from parent
		if(p_no != P-1)
			recv(d1);
		next = *d1;
	}
	free(d1);
	printf(1, "%d:%d\n", prev, next);
	/*****************************************/

	for(;;){
		diff = 0.0;
		for(i =start ; i <= end; i++){
			for(j =1 ; j < N-1; j++){
				w[i][j] = ( u[i-1][j] + u[i+1][j]+
					    u[i][j-1] + u[i][j+1])/4.0;
				if( fabsm(w[i][j] - u[i][j]) > diff )
					diff = fabsm(w[i][j]- u[i][j]);	
			}
		}
	    count++;
	    
	    if(my_id != par_id){
	    	//barrier here to send diff to parent - unicast
	    	float* d = (float*)malloc(MSGSIZE);
	    	*d = diff;
	    	send(my_id, par_id, d);
	    	//receive max diff from parent - unicast {multicast?}
	    	recv(d);
	    	diff = *d;
	    	free(d);
	    } else {
	    	//parent receives diff from each child and send the max to each child
	    	float* d = (float*)malloc(MSGSIZE);
	    	for(int px = 1; px < P; px++){
				recv(d);
				if(*d > diff)
					diff = *d;
	    	}
	    	*d = diff;
	    	for(int px = 0; px < P; px++){
				send(par_id,pid[px],d);
	    	}
	    	recv(d);
	    	free(d);
	    }
	    
	    if(diff<= E || count > L){ 
			if(my_id != par_id){
				struct Data *d = (struct Data*)malloc(sizeof(struct Data));
				//send your values to parent - unicast
				for(int k = start; k <= end; k++){
					d->line_no = k;
					for(int x = 1; x < N-1; x++)
						d->line[x] = w[k][x];
					send(my_id, par_id, d);
				}
				//check if parent exited (so that it is waiting for child to exit) - unicast
				recv(d);
				
				free(d);
				exit();
			}
			break;
		}
		
		for (i =1; i< N-1; i++)	
			for (j =1; j< N-1; j++) u[i][j] = w[i][j];	//valid only for interior values
		
		/*struct Data *d = (struct Data*)malloc(sizeof(struct Data));
		// send boundary values to prev and next process - unicast {multicast?}
		// first doesn't send to a precess before, p_no == P denotes parent -> first block
		if(p_no != P){
			d->line_no = start;
			for(int x = 1; x < N-1; x++)
				d->line[x] = w[start][x];
			send(my_id, prev, d);
		}
		
		// last doesn't send to process after
		if(p_no != P-1){
			d->line_no = end;
			for(int x = 1; x < N-1; x++)
				d->line[x] = w[end][x];
			send(my_id, next, d);
		}
		
		// wait for boundary values from the prev and next process - unicast {multicast?}
		recv(d);
		for(int x = 1; x < N-1; x++)
			u[d->line_no][x] = d->line[x];
		
		// first and last receive from only one process
		if(p_no != P && p_no != P-1){
			recv(d);
			for(int x = 1; x < N-1; x++)
				u[d->line_no][x] = d->line[x];
		}
		free(d);*/
	}

	/**************join here***************/
	//parent receives lines from children
	struct Data *d = (struct Data*)malloc(sizeof(struct Data));
	for(int l = end+1; l < N-1; l++){
		recv(d);
		for(int x = 1; x < N-1; x++)
			u[d->line_no][x] = d->line[x];
	}
	//parent signals children to exit - unicast
	for(p_no = 1; p_no < P; p_no++)
		send(par_id,pid[p_no],d);	
	free(d);
	//parent waits for children to exit
	for(p_no = 1; p_no < P; p_no++)
		wait();	
	/***************************************/

	for(i =0; i <N; i++){
		for(j = 0; j<N; j++)
			printf(1,"%d ",((int)u[i][j]));
		printf(1,"\n");
	}
	exit();

}