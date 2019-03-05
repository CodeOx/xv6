#include "types.h"
#include "stat.h"
#include "user.h"
#define N 10
#define EPSILON 0.01

double fabs(double n)
{
	if (n >= 0)
		return n;
	return -1*n;
}

int main(int argc, char *argv[])
{
	double diff;
	int i,j;
	double mean;
	double u[N][N];
	double w[N][N];
	int count=0;

	mean = 0.0;
	for (i = 0; i < N; i++){
		u[i][0] = u[i][N-1] = u[0][i] = 100.0;
		u[N-1][i] = 0.0;
		mean += u[i][0] + u[i][N-1] + u[0][i] + u[N-1][i];
	}
	mean /= (4.0 * N);
	for (i = 1; i < N-1; i++ )
		for ( j= 1; j < N-1; j++) u[i][j] = mean;
	for(;;){
		diff = 0.0;
		for(i =1 ; i < N-1; i++){
			for(j =1 ; j < N-1; j++){
				w[i][j] = ( u[i-1][j] + u[i+1][j]+
					    u[i][j-1] + u[i][j+1])/4.0;
				if( fabs(w[i][j] - u[i][j]) > diff )
					diff = fabs(w[i][j]- u[i][j]);
			count++;	
			}
		}
		if(diff <= EPSILON) break;
		for (i =1; i< N-1; i++)	
			for (j =1; j< N-1; j++) u[i][j] = w[i][j];
	}
	for(i =0; i <N; i++){
		for(j = 0; j<N; j++){
			printf(1, "%d",(int)u[i][j]);
			printf(1, ".");
			printf(1, "%d,\t",(int)((u[i][j]-(int)u[i][j])*100));
		}
		printf(1, "\n");
	}
	printf(1, "\nNumber of iteration: %d\n",count);
	exit();
}
