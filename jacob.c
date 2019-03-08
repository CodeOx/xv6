#include "types.h"
#include "stat.h"
#include "user.h"


#define N 11
#define E 0.00001
#define T 100.0
#define P 6
#define L 20000

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
  	int block = N/P;
  	int par_id = getpid(); pid[0] = par_id;
	//fork here
	for(;;){
		diff = 0.0;
		for(i =1 ; i < N-1; i++){
			for(j =1 ; j < N-1; j++){
				w[i][j] = ( u[i-1][j] + u[i+1][j]+
					    u[i][j-1] + u[i][j+1])/4.0;
				if( fabsm(w[i][j] - u[i][j]) > diff )
					diff = fabsm(w[i][j]- u[i][j]);	
			}
		}
	    count++;
	    //barrier here to send diff to parent - unicast
	    //receive max diff from parent - multicast(1)
		if(diff<= E || count > L){ 
			//check if parent exited (so that it is waiting for child to exit) - unicast
			//send your values to parent
			//exit here
			break;
		}

		for (i =1; i< N-1; i++)	
			for (j =1; j< N-1; j++) u[i][j] = w[i][j];	//valid only for interior values
		
		// send boundary values to prev and next process - unicast {multicast?}
		// wait for boundary values from the prev and next process - unicast {multicast?}
	}
	//join here
	//parent signals children to exit - unicast
	//parent waits for values from children

	for(i =0; i <N; i++){
		for(j = 0; j<N; j++)
			printf(1,"%d ",((int)u[i][j]));
		printf(1,"\n");
	}
	exit();

}