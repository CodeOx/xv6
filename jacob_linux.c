//cp /mnt/c/Users/HP/Documents/iit_acad/col331/lab1/maekawa/jacob_linux.c
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>	
#include <stdlib.h> 
#include <time.h>
#include <fcntl.h> 
#include <sys/stat.h> 

#define MSGSIZE 100

/*#define N 11
#define E 0.00001
#define T 100.0
#define P 6
#define L 20000*/

int N,P,L;
float E,T;

volatile int num;
volatile char* msgShared;
volatile char* msgShared_temp;

struct Data{
	int line_no;
	//char padding[MSGSIZE - 4 - (4*N)];
	float line[MSGSIZE-4];
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

void read_input(char* filename){
	int fd = open(filename, 0);
	char c;
	float ratio;
	N = 0; P = 0; L = 0; E = 0.0; T = 0.0;
	while(1){
		read(fd, &c, 1);
		if((int)(c-'0') < 0 || (int)(c-'0') > 9)
			break;
		N = 10*N + (int)(c-'0');
	}
	while((int)(c-'0') < 0 || (int)(c-'0') > 9)
		read(fd, &c, 1);
	
	E = 10*E + (int)(c-'0');
	
	while(1){
		read(fd, &c, 1);
		if((int)(c-'0') < 0 || (int)(c-'0') > 9)
			break;
		E = 10*E + (int)(c-'0');
	}
	if((int)(c-'0') != -2){
		printf("Incorrect input file format\n");
	}
	ratio = 0.1;
	while(1){
		read(fd, &c, 1);
		if((int)(c-'0') < 0 || (int)(c-'0') > 9)
			break;
		E = E + ratio*(int)(c-'0');
		ratio *= 0.1;
	}

	while((int)(c-'0') < 0 || (int)(c-'0') > 9)
		read(fd, &c, 1);
	
	T = 10*T + (int)(c-'0');
	
	while(1){
		read(fd, &c, 1);
		if((int)(c-'0') < 0 || (int)(c-'0') > 9)
			break;
		T = 10*T + (int)(c-'0');
	}

	if((int)(c-'0') == -2){
		ratio = 0.1;
		while(1){
			read(fd, &c, 1);
			if((int)(c-'0') < 0 || (int)(c-'0') > 9)
				break;
			T = T + ratio*(int)(c-'0');
			ratio *= 0.1;
		}
	}

	while((int)(c-'0') < 0 || (int)(c-'0') > 9)
		read(fd, &c, 1);
	P = 10*P + (int)(c-'0');
	while(1){
		int a = read(fd, &c, 1);
		if((int)(c-'0') < 0 || (int)(c-'0') > 9 || a <= 0)
			break;
		P = 10*P + (int)(c-'0');
	}

	while((int)(c-'0') < 0 || (int)(c-'0') > 9)
		read(fd, &c, 1);
	L = 10*L + (int)(c-'0');
	while(1){
		int a = read(fd, &c, 1);
		if((int)(c-'0') < 0 || (int)(c-'0') > 9 || a <= 0)
			break;
		L = 10*L + (int)(c-'0');
	}
	close(fd);
}

int main(int argc, char *argv[])
{
	if(argc < 2){
		printf("Error: no input file\n");
		exit(1);
	}

	char *filename;
	filename=argv[1];
	read_input(filename);

	//printf("N=%d,E=%d,T=%d,P=%d,L=%d\n", N, (int)(10000*E), (int)T, P, L);

	volatile float diff;
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
	int fd[2*P];
	int diffFD[2*P];
  	int block = (N-2)/P;
  	int par_id = getpid(); pid[0] = par_id;
	int p_no;
	int bno = 0;//set_barrier(P);

	for (int i = 0; i < P; i++) {
	    pipe(&fd[2*i]);
	    pipe(&diffFD[2*i]);
	}

	clock_t start_t, end_t;
 	double cpu_time_used;
 	start_t = clock();

	/**************fork here****************/
	for(p_no = 1; p_no < P; p_no++){
		pid[p_no] = fork();
		if( pid[p_no] == 0)
			break;
	}
	int my_id = getpid();
	int myFD;
	int nextFD;
	int prevFD;
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
	//set_handle(sig_handler);
	msgShared = (char*)malloc(MSGSIZE);

	/********** set prev and next ***********/
	/****************************************/
	int prev=-1, next=-1;
	if(P > 1){
		int* d1 = (int*)malloc(MSGSIZE);
		if(p_no == P){
			myFD = fd[0];
			nextFD = fd[3];

			next = pid[1];
			//send prev to children
			for(int px = 1; px < P; px++){
				*d1 = pid[px-1];
				//send(par_id, pid[px], d1);
				write(fd[2*px + 1], d1, MSGSIZE); 
			}
			for(int px = 1; px < P; px++){
				//recv(d1);
				read(myFD, d1, MSGSIZE);
			}
			//send next to children
			for(int px = 1; px < P-1; px++){
				*d1 = pid[px+1];
				//send(par_id, pid[px], d1);
				write(fd[2*px + 1], d1, MSGSIZE);
			}
		} else {
			//receive pid of prev from parent
			myFD = fd[2*p_no];
			prevFD = fd[2*p_no-1];
			//recv(d1);
			read(myFD, d1, MSGSIZE);
			prev = *d1;
			//reply d
			//send(my_id, par_id, d1);
			write(fd[1], d1, MSGSIZE);
			//receive pid of next from parent
			if(p_no != P-1){
				nextFD = fd[2*p_no+3];
				//recv(d1);
				read(myFD, d1, MSGSIZE);
			}
			next = *d1;
		}
		free(d1);
	}

	//printf("%d:%d:%d\n", prev, getpid(), next);
	
	/*****************************************/
	/***************parallel*****************/
	float* d2 = (float*)malloc(MSGSIZE);
	struct Data *d = (struct Data*)malloc(sizeof(struct Data));
	
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
	    
	    if(P > 1){
		    if(my_id != par_id){
		    	*d2 = diff;
		    	//send(my_id, par_id, d2);	//barrier here to send diff to parent - unicast
		    	//write(fd[1], d2, MSGSIZE);
		    	write(diffFD[1], d2, MSGSIZE);
		    	//recv(d2);	//receive max diff from parent - unicast {multicast?}
		    	//read(myFD, d2, MSGSIZE);
		    	read(diffFD[2*p_no], d2, MSGSIZE);
		    	diff = *d2;
		    } else {
		    	//parent receives diff from each child and send the max to each child
		    	for(int px = 1; px < P; px++){
					//recv(d2);
					//read(myFD, d2, MSGSIZE);
					read(diffFD[0], d2, MSGSIZE);
					if(*d2 > diff)
						diff = *d2;
		    	}
		    	*d2 = diff;
		    	for(int px = 0; px < P; px++){
					//send(par_id,pid[px],d2);
					//write(fd[2*px + 1], d2, MSGSIZE);
					write(diffFD[2*px + 1], d2, MSGSIZE);
		    	}
		    	//recv(d2);
		    	//read(myFD, d2, MSGSIZE);
		    	read(diffFD[0], d2, MSGSIZE);
		    }

		    //printf("%f\n", diff);

		    //barrier(bno);
		}

	    if(diff<= E || count > L){ 
	    	free(d2);
			if(my_id != par_id){
				//send your values to parent - unicast
				for(int k = start; k <= end; k++){
					d->line_no = k;
					for(int x = 1; x < N-1; x++)
						d->line[x] = w[k][x];
					//send(my_id, par_id, d);
					write(fd[1], d, MSGSIZE);		
				}
				//check if parent exited (so that it is waiting for child to exit)
				//recv(d)
				
				free(d);
				exit(1);
			}
			break;
		}
		
		for (i =1; i< N-1; i++)	
			for (j =1; j< N-1; j++) u[i][j] = w[i][j];	//valid only for interior values
		
		// send boundary values to prev and next process - unicast {multicast?}
		// first doesn't send to a precess before, p_no == P denotes parent -> first block
		if(P > 1){
			if(p_no != P){
				d->line_no = start;
				for(int x = 1; x < N-1; x++)
					d->line[x] = w[start][x];
				//send(my_id, prev, d);
				write(prevFD, d, MSGSIZE);
			}
			
			// last doesn't send to process after
			if(p_no != P-1){
				d->line_no = end;
				for(int x = 1; x < N-1; x++)
					d->line[x] = w[end][x];
				//send(my_id, next, d);
				write(nextFD, d, MSGSIZE);
			}

			// wait for boundary values from the prev and next process - unicast {multicast?}
			//recv(d);
			read(myFD, d, MSGSIZE);
			for(int x = 1; x < N-1; x++)
				u[d->line_no][x] = d->line[x];
			
			// first and last receive from only one process
			if(p_no != P && p_no != P-1){
				//recv(d);
				read(myFD, d, MSGSIZE);
				for(int x = 1; x < N-1; x++)
					u[d->line_no][x] = d->line[x];
			}

			//barrier(bno);
		}
	}

	/**************join here***************/
	//parent receives lines from children
	for(int l = end+1; l < N-1; l++){
		//recv(d);
		read(myFD, d, MSGSIZE);
		for(int x = 1; x < N-1; x++)
			u[d->line_no][x] = d->line[x];
	}
	//parent signals children to exit - unicast
	/*for(p_no = 1; p_no < P; p_no++)
		send(par_id,pid[p_no],d);	
	*/
	free(d);

	//parent waits for children to exit
	for(p_no = 1; p_no < P; p_no++)
		wait(NULL);	
	
	for(int l = start; l <= end; l++){
		for(int x = 1; x < N-1; x++)
			u[l][x] = w[l][x];
	}
	/***************************************/

	for(i =0; i <N; i++){
		for(j = 0; j<N; j++)
			printf("%f ",(u[i][j]));
		printf("\n");
	}

	end_t = clock();
	cpu_time_used = ((double) (end_t - start_t)) / CLOCKS_PER_SEC;
	printf("Time used = %f\n", cpu_time_used);

	exit(1);

}