//cp /mnt/c/Users/HP/Documents/iit_acad/col331/lab1/maekawa/maekawa_linux.c
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>	
#include <stdlib.h> 
#include <time.h>
#include <fcntl.h> 
#include <sys/stat.h> 

#define WAITQLENGTH 10
#define MSGSIZE 100

int P,P1,P2,P3;

struct message{
	int pid;	//sender id
	int time;	//timestamp
	char type;	//R:request, L:locked, F:failed, I:inquire, Q:relenquish, S:released, A:access granted to child, E:exit
};

struct waitQItem
{
	int allotted;
	struct message *m;	
};

int *pid;
int *fd;
int *req_set;
int req_size;
int my_id;
int child_id;
int myFD;
int childFD;

int fdFromPid(int pi){
	for(int i = 0; i < P; i++){
		if(pid[i] == pi){
			return fd[2*i+1];
		}
	}
	printf("Error fd From pid\n");
	return 0;
}

void printM(int id,struct message* m){
	printf("%d:pid = %d, %c\n", id, m->pid, m->type);
}

void request_set(int pno){
	int row_size;
	switch(P){
		case 4: req_size = 3; row_size = 2; break;
		case 9: req_size = 5; row_size = 3; break;
		case 16: req_size = 7; row_size = 4; break;
		case 25: req_size = 9; row_size = 5; break;
		default: req_size = 1; row_size = 1; break;
	}
	req_set = (int*)malloc(req_size*sizeof(int));
	for(int i = 0; i < row_size; i++){
		req_set[i] = pid[((pno/row_size)*row_size) + i];
	}
	int k = row_size;
	for(int i = 0; i < row_size; i++){
		if(pno/row_size != i){
			req_set[k] = pid[(pno%row_size) + (i*row_size)];
			k++;
		}
	}
	return;
}

int precedes(struct message* m1, struct message* m2){
	if(m1->time < m2->time || (m1->time == m2->time && m1->pid < m2->pid))
		return 1;
	return 0;
}

void listen(){
	struct message *m = (struct message*)malloc(MSGSIZE);
	struct message *m_reply = (struct message*)malloc(MSGSIZE);
	struct message *lockingRequest = (struct message*)malloc(MSGSIZE);
	struct message *minRequest = m;
	struct waitQItem waitQ[WAITQLENGTH];
	struct waitQItem enquireQ[WAITQLENGTH];
	for(int i = 0; i < WAITQLENGTH; i++){
		waitQ[i].allotted = 0;
		waitQ[i].m = (struct message*)malloc(MSGSIZE);

		enquireQ[i].allotted = 0;
		enquireQ[i].m = (struct message*)malloc(MSGSIZE);
	}
	
	int state = 0;
	int numRelease = 0;
	int numLockedReply = 0;
	int inq_sent = 0;
	int inq_recv = 0;
	int fail_recv = 0;
	
	while(1){
		//recv(m);
		read(myFD, m, MSGSIZE);
		printM(my_id, m);
		switch(m->type){
			case 'R': if(state == 0){
						state = 1;
						
						lockingRequest->pid = m->pid;
						lockingRequest->time = m->time;
						lockingRequest->type = m->type;

						m_reply->pid = my_id;
						m_reply->time = m->time;
						m_reply->type = 'L';
						
						//send(my_id, m->pid, m_reply);
						write(fdFromPid(m->pid), m_reply, MSGSIZE);
					
					} else {
						for(int i = 0; i < WAITQLENGTH; i++){
							if(waitQ[i].allotted == 0){
								waitQ[i].allotted = 1;
								waitQ[i].m->pid = m->pid;
								waitQ[i].m->time = m->time;
								waitQ[i].m->type = m->type;
								break;
							}
						}
						int prec = 0;
						if(precedes(lockingRequest, m)){
							m_reply->pid = my_id;
							m_reply->time = m->time;
							m_reply->type = 'F';
							//send(my_id, m->pid, m_reply);
							printf("LL%d\n", lockingRequest->pid);
							write(fdFromPid(m->pid), m_reply, MSGSIZE);
							prec = 1;
						}
						for(int i = 0; i < WAITQLENGTH; i++){
							if(waitQ[i].allotted == 1 && precedes(waitQ[i].m, m)){
								m_reply->pid = my_id;
								m_reply->time = m->time;
								m_reply->type = 'F';
								//send(my_id, m->pid, m_reply);
								//printf("LL2%d:%d\n", waitQ[i].m->pid, lockingRequest->pid);
								write(fdFromPid(m->pid), m_reply, MSGSIZE);
								prec = 1;
								break;
							}
						}
						if(prec == 1){
							break;
						} else {
							if(inq_sent == 1)
								break;
							inq_sent = 1;
							m_reply->pid = my_id;
							m_reply->time = lockingRequest->time;
							m_reply->type = 'I';
							//send(my_id, lockingRequest->pid, m_reply);
							write(fdFromPid(lockingRequest->pid), m_reply, MSGSIZE);
						}
					}
					break;
			case 'L': numLockedReply++;
					printf("%d:%dhere\n", my_id, numLockedReply);
					if(numLockedReply >= req_size){
						if(my_id != lockingRequest->pid)
							printf("Error L\n");
						numLockedReply = 0;
						fail_recv = 0;
						m_reply->pid = my_id;
						m_reply->time = lockingRequest->time;
						m_reply->type = 'A';
						//send(my_id, child_id, m_reply);
						write(childFD, m_reply, MSGSIZE);
					}
					break;
			case 'F': fail_recv = 1;
					if(inq_recv > 0){
						for(int i = 0; i < WAITQLENGTH; i++){
							if(enquireQ[i].allotted == 1){
								inq_recv--;
								enquireQ[i].allotted = 0;

								m_reply->pid = my_id;
								m_reply->time = enquireQ[i].m->time;
								m_reply->type = 'Q';
								//send(my_id, enquireQ[i].m->pid, m_reply);
								write(fdFromPid(enquireQ[i].m->pid), m_reply, MSGSIZE);

								break;
							}
						}
					}
					break;
			case 'S': numRelease++;
					/*if(numRelease >= P2 + P3){
						return;
					}*/

					inq_sent = 0;
					
					if(m->pid == my_id){
						inq_recv = 0;
					}

					int available = 0;
					for(int i = 0; i < WAITQLENGTH; i++){
						if(waitQ[i].allotted == 1){
							if(available == 0 || precedes(waitQ[i].m, minRequest)){
								available = 1;
								minRequest = waitQ[i].m;
							}
						}
					}
					if(available == 0)
						state = 0;
					else{
						for(int i = 0; i < WAITQLENGTH; i++){
							if(waitQ[i].allotted == 1 && waitQ[i].m->pid == minRequest->pid){
								waitQ[i].allotted = 0;
								break;
							}
						}

						lockingRequest->pid = minRequest->pid;
						lockingRequest->time = minRequest->time;
						lockingRequest->type = minRequest->type;

						m_reply->pid = my_id;
						m_reply->time = minRequest->time;
						m_reply->type = 'L';
						
						//send(my_id, minRequest->pid, m_reply);
						write(fdFromPid(minRequest->pid), m_reply, MSGSIZE);

						for(int i = 0; i < WAITQLENGTH; i++){
							if(waitQ[i].allotted == 1){
								for(int j = 0; j < WAITQLENGTH; j++){
									if(enquireQ[j].allotted == 1 && enquireQ[j].m->pid == waitQ[i].m->pid){
										enquireQ[j].allotted = 0;
										inq_recv--;
										m_reply->pid = my_id;
										m_reply->time = waitQ[i].m->time;
										m_reply->type = 'F';
										write(fdFromPid(waitQ[i].m->pid), m_reply, MSGSIZE);
									}
								}
							}
						}
					}
					break;
			case 'I': if(fail_recv){
						m_reply->pid = my_id;
						m_reply->time = m->time;
						m_reply->type = 'Q';
						//send(my_id, m->pid, m_reply);
						write(fdFromPid(m->pid), m_reply, MSGSIZE);
						numLockedReply--;	
						break;
					} else {
						inq_recv++;
						for(int i = 0; i < WAITQLENGTH; i++){
							if(enquireQ[i].allotted == 0){
								enquireQ[i].allotted = 1;
								enquireQ[i].m->pid = m->pid;
								enquireQ[i].m->time = m->time;
								enquireQ[i].m->type = m->type;
								break;
							}
						}
					}
					break;
			case 'Q': for(int i = 0; i < WAITQLENGTH; i++){
						if(waitQ[i].allotted == 0){
							waitQ[i].allotted = 1;
							waitQ[i].m->pid = lockingRequest->pid;
							waitQ[i].m->time = lockingRequest->time;
							waitQ[i].m->type = lockingRequest->type;
							break;
						}
					}
					inq_sent = 0;
					int available1 = 0;
					for(int i = 0; i < WAITQLENGTH; i++){
						if(waitQ[i].allotted == 1){
							if(available1 == 0 || precedes(waitQ[i].m, minRequest)){
								available1 = 1;
								minRequest = waitQ[i].m;
							}
						}
					}

					for(int i = 0; i < WAITQLENGTH; i++){
						if(waitQ[i].allotted == 1 && waitQ[i].m->pid == minRequest->pid){
							waitQ[i].allotted = 0;
							break;
						}
					}

					lockingRequest->pid = minRequest->pid;
					lockingRequest->time = minRequest->time;
					lockingRequest->type = minRequest->type;

					m_reply->pid = my_id;
					m_reply->time = minRequest->time;
					m_reply->type = 'L';
					
					//send(my_id, minRequest->pid, m_reply);
					write(fdFromPid(minRequest->pid), m_reply, MSGSIZE);

					break;
			case 'E':return;
			default: printf("Error Type : %c\n", m->type); break;
		}
	}
}

void acquire1(){
	struct message *m = (struct message*)malloc(MSGSIZE);
	m->pid = my_id;
	m->time = (unsigned)time(NULL);
	m->type = 'R';
	//printM(my_id, m);
	for(int i = 0; i < req_size; i++){
		//send(my_id, req_set[i], m);
		write(fdFromPid(req_set[i]), m, MSGSIZE);
	}
	//recv(m);
	read(childFD, m, MSGSIZE);
	if(m->type != 'A' || m->pid != my_id)
		printf("Error Acquire\n");
	free(m);
}

void release1(){
	struct message *m = (struct message*)malloc(MSGSIZE);
	m->pid = my_id;
	m->time = (unsigned)time(NULL);
	m->type = 'S';
	for(int i = 0; i < req_size; i++){
		//send(my_id, req_set[i], m);
		write(fdFromPid(req_set[i]), m, MSGSIZE);
	}
	free(m);
}

void read_input(char* filename){
	int fd = open(filename, 0);
	char c;
	P = 0; P1 = 0; P2 = 0; P3 = 0;
	while(1){
		read(fd, &c, 1);
		if((int)(c-'0') < 0 || (int)(c-'0') > 9)
			break;
		P = 10*P + (int)(c-'0');
	}
	while((int)(c-'0') < 0 || (int)(c-'0') > 9)
		read(fd, &c, 1);
	P1 = 10*P1 + (int)(c-'0');
	while(1){
		read(fd, &c, 1);
		if((int)(c-'0') < 0 || (int)(c-'0') > 9)
			break;
		P1 = 10*P1 + (int)(c-'0');
	}
	while((int)(c-'0') < 0 || (int)(c-'0') > 9)
		read(fd, &c, 1);
	P2 = 10*P2 + (int)(c-'0');
	while(1){
		read(fd, &c, 1);
		if((int)(c-'0') < 0 || (int)(c-'0') > 9)
			break;
		P2 = 10*P2 + (int)(c-'0');
	}
	while((int)(c-'0') < 0 || (int)(c-'0') > 9)
		read(fd, &c, 1);
	P3 = 10*P3 + (int)(c-'0');
	while(1){
		int a = read(fd, &c, 1);
		if((int)(c-'0') < 0 || (int)(c-'0') > 9 || a <= 0)
			break;
		P3 = 10*P3 + (int)(c-'0');
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

	printf("P=%d,P1=%d,P2=%d,P3=%d\n", P, P1, P2, P3);

	pid = (int*)malloc(MSGSIZE);
	fd = (int*)malloc(sizeof(int)*2*P);
	int P1id[P1], P2id[P2], P3id[P3];

	int globalParentId = getpid();
	int globalParentFD[2];
	pipe(&globalParentFD[0]);
	int fd_child[2*P];
	for (int i = 0; i < P; i++) {
	    pipe(&fd[2*i]);
	    pipe(&fd_child[2*i]);
	}

	for(int i = 0; i< P1; i++){
		P1id[i] = fork();
		if(P1id[i] == 0){
			my_id = getpid();
			myFD = fd[2*i];
			//recv(pid);
			read(myFD, pid, MSGSIZE);
			listen();
			exit(1);
		}
		pid[i] = P1id[i];
	}

	for(int i = 0; i < P2; i++){
		P2id[i] = fork();
		if(P2id[i] == 0){
			my_id = getpid();
			myFD = fd[2*(P1 + i)];
			childFD = fd_child[2*(P1+i) + 1];
			//recv(pid);
			read(myFD, pid, MSGSIZE);
			request_set(P1 + i);
			child_id = fork();
			if(child_id == 0){
				childFD = fd_child[2*(P1+i)];
				acquire1();
				printf("%d acquired the lock at time %d\n", my_id, (unsigned)time(NULL));
				sleep(2);	//sleep for 2 sec
				printf("%d released the lock at time %d\n", my_id, (unsigned)time(NULL));
				release1();
				//send(getpid(), globalParentId, pid);
				write(globalParentFD[1], pid, MSGSIZE);
				exit(1);	
			}
			listen();
			wait(NULL);	
			exit(1);
		}
		pid[P1+i] = P2id[i];
	}

	for(int i = 0; i < P3; i++){
		P3id[i] = fork();
		if(P3id[i] == 0){
			my_id = getpid();
			myFD = fd[2*(P1 + P2 + i)];
			childFD = fd_child[2*(P1+P2+i) + 1];
			//recv(pid);
			read(myFD, pid, MSGSIZE);
			request_set(P1 + P2 + i);
			child_id = fork();
			if(child_id == 0){
				childFD = fd_child[2*(P1+P2+i)];
				acquire1();
				printf("%d acquired the lock at time %d\n", my_id, (unsigned)time(NULL));
				printf("%d released the lock at time %d\n", my_id, (unsigned)time(NULL));
				release1();
				//send(getpid(), globalParentId, pid);
				write(globalParentFD[1], pid, MSGSIZE);
				exit(1);
			}
			listen();
			wait(NULL);
			exit(1);
		}
		pid[P1+P2+i] = P3id[i];
	}

	//printf("global parent = %d\n", getpid());
	for(int i = 0; i < P; i++){
		//send(getpid(), pid[i], pid);
		write(fdFromPid(pid[i]), pid, MSGSIZE);
	}

	for(int i = 0; i < P2+P3; i++){
		//recv(pid);
		read(globalParentFD[0], pid, MSGSIZE);
	}

	struct message *m = (struct message*)malloc(MSGSIZE);
	m->pid = globalParentId;
	m->type = 'E';

	for(int i = 0; i < P; i++){
		//send(globalParentId, pid[i], m);
		write(fdFromPid(pid[i]), m, MSGSIZE);
	}

	for(int i = 0; i < P; i++){
		wait(NULL);
	}

	/*for(int i = 0; i < 3; i++){
		for(int j = 0; j < 3; j++){
			printf("%d ", pid[i*3 + j]);
		}
		printf("\n");
	}*/

	exit(1);
}