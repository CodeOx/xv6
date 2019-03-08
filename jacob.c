#include "types.h"
#include "stat.h"
#include "user.h"

#define MSGSIZE 100

#define N 20
#define E 0.00001
#define T 100.0
#define P 6
#define L 20000

struct Data{
	int line_no;
	char padding[MSGSIZE - 4 - (4*N)];
	float line[N];
};

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
	//fork here
	for(p_no = 1; p_no < P; p_no++){
		pid[p_no] = fork();
		if( pid[p_no] == 0)
			break;
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
				//check if parent exited (so that it is waiting for child to exit) - unicast
				struct Data *d = (struct Data*)malloc(sizeof(struct Data));
				recv(d);
				//send your values to parent - unicast
				for(int k = start; k <= end; k++){
					d->line_no = k;
					for(int x = 1; x < N-1; x++)
						d->line[x] = w[k][x];
					send(my_id, par_id, d);
				}
				free(d);
				//exit here
				exit();
			}
			break;
		}

		for (i =1; i< N-1; i++)	
			for (j =1; j< N-1; j++) u[i][j] = w[i][j];	//valid only for interior values
		
		// send boundary values to prev and next process - unicast {multicast?}
		// wait for boundary values from the prev and next process - unicast {multicast?}
	}
	//join here
	printf(1, "hello : %d\n", sizeof(struct Data));
	//parent signals children to exit - unicast
  	struct Data *d = (struct Data*)malloc(sizeof(struct Data));
	for(p_no = 1; p_no < P; p_no++)
		send(par_id,pid[p_no],d);	
	//parent receives lines from children
	for(int l = end+1; l < N-1; l++){
		recv(d);
		for(int x = 1; x < N-1; x++)
			u[d->line_no][x] = d->line[x];
	}
	free(d);
	//parent waits for values from children
	for(p_no = 1; p_no < P; p_no++)
		wait();	

	for(i =0; i <N; i++){
		for(j = 0; j<N; j++)
			printf(1,"%d ",((int)u[i][j]));
		printf(1,"\n");
	}
	exit();

}