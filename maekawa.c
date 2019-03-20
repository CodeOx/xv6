#include "types.h"
#include "stat.h"
#include "user.h"

#define WAITQLENGTH 10
#define MSGSIZE 100
#define P 9
#define P1 3
#define P2 3
#define P3 3

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
int *req_set;
int req_size;
int my_id;
int child_id;

void printM(int id,struct message* m){
	printf(1, "%d:pid = %d, %c\n", id, m->pid, m->type);
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
		recv(m);
		//printM(my_id, m);
		switch(m->type){
			case 'R': if(state == 0){
						state = 1;
						
						lockingRequest->pid = m->pid;
						lockingRequest->time = m->time;
						lockingRequest->type = m->type;

						m_reply->pid = my_id;
						m_reply->time = m->time;
						m_reply->type = 'L';
						
						send(my_id, m->pid, m_reply);
					
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
							send(my_id, m->pid, m_reply);
							prec = 1;
						}
						for(int i = 0; i < WAITQLENGTH; i++){
							if(waitQ[i].allotted == 1 && precedes(waitQ[i].m, m)){
								m_reply->pid = my_id;
								m_reply->time = m->time;
								m_reply->type = 'F';
								send(my_id, m->pid, m_reply);
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
							send(my_id, lockingRequest->pid, m_reply);
						}
					}
					break;
			case 'L': numLockedReply++;
					if(numLockedReply >= req_size){
						if(my_id != lockingRequest->pid)
							printf(1, "Error L\n");
						numLockedReply = 0;
						m_reply->pid = my_id;
						m_reply->time = lockingRequest->time;
						m_reply->type = 'A';
						send(my_id, child_id, m_reply);
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
								send(my_id, enquireQ[i].m->pid, m_reply);
								
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
						
						send(my_id, minRequest->pid, m_reply);
					}
					break;
			case 'I': if(fail_recv){
						m_reply->pid = my_id;
						m_reply->time = m->time;
						m_reply->type = 'Q';
						send(my_id, m->pid, m_reply);
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
					
					send(my_id, minRequest->pid, m_reply);

					break;
			case 'E':return;
			default: printf(1, "Error Type : %c\n", m->type); break;
		}
	}
}

void acquire1(){
	struct message *m = (struct message*)malloc(MSGSIZE);
	m->pid = my_id;
	m->time = uptime();
	m->type = 'R';
	//printM(my_id, m);
	for(int i = 0; i < req_size; i++){
		send(my_id, req_set[i], m);
	}
	recv(m);
	if(m->type != 'A' || m->pid != my_id)
		printf(1, "Error Acquire\n");
	free(m);
}

void release1(){
	struct message *m = (struct message*)malloc(MSGSIZE);
	m->pid = my_id;
	m->time = uptime();
	m->type = 'S';
	for(int i = 0; i < req_size; i++){
		send(my_id, req_set[i], m);
	}
	free(m);
}

int main(int argc, char *argv[])
{
	pid = (int*)malloc(MSGSIZE);
	int P1id[P1], P2id[P2], P3id[P3];

	int globalParentId = getpid();

	for(int i = 0; i< P1; i++){
		P1id[i] = fork();
		if(P1id[i] == 0){
			my_id = getpid();
			recv(pid);
			listen();
			exit();
		}
		pid[i] = P1id[i];
	}

	for(int i = 0; i < P2; i++){
		P2id[i] = fork();
		if(P2id[i] == 0){
			my_id = getpid();
			recv(pid);
			request_set(P1 + i);
			child_id = fork();
			if(child_id == 0){
				acquire1();
				printf(1, "%d acquired the lock at time %d\n", getpid(), uptime());
				sleep(200);	//sleep for 2 sec
				printf(1, "%d released the lock at time %d\n", getpid(), uptime());
				release1();
				send(getpid(), globalParentId, pid);
				exit();	
			}
			listen();
			wait();	
			exit();
		}
		pid[P1+i] = P2id[i];
	}

	for(int i = 0; i < P3; i++){
		P3id[i] = fork();
		if(P3id[i] == 0){
			my_id = getpid();
			recv(pid);
			request_set(P1 + P2 + i);
			child_id = fork();
			if(child_id == 0){
				acquire1();
				printf(1, "%d acquired the lock at time %d\n", getpid(), uptime());
				printf(1, "%d released the lock at time %d\n", getpid(), uptime());
				release1();
				send(getpid(), globalParentId, pid);
				exit();
			}
			listen();
			wait();
			exit();
		}
		pid[P1+P2+i] = P3id[i];
	}

	printf(1, "global parent = %d\n", getpid());
	for(int i = 0; i < P; i++){
		send(getpid(), pid[i], pid);
	}

	for(int i = 0; i < P2+P3; i++){
		recv(pid);
	}

	struct message *m = (struct message*)malloc(MSGSIZE);
	m->pid = globalParentId;
	m->type = 'E';

	for(int i = 0; i < P; i++){
		send(globalParentId, pid[i], m);
	}

	for(int i = 0; i < P; i++){
		wait();
	}

	/*for(int i = 0; i < 3; i++){
		for(int j = 0; j < 3; j++){
			printf(1, "%d ", pid[i*3 + j]);
		}
		printf(1, "\n");
	}*/

	exit();
}