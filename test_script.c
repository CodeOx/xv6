#include "types.h"
#include "user.h"
#include "date.h"
#include "fcntl.h"

#define MSGSIZE 100

int parentId;
char* m;

void reverse(char str[], int length) 
{ 
    int start = 0; 
    int end = length -1; 
    while (start < end) 
    { 
        // swap(*(str+start), *(str+end)); 
            char temp;
            temp=*(str+start);
            *(str+start)=*(str+end);
            *(str+end)=temp;
        start++; 
        end--; 
    } 
} 
  
char* itoa(int num, char* str) 
{ 
    int i = 0; 
    int base=10;
  
    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0) 
    { 
        str[i++] = '0'; 
        str[i] = '\0'; 
        return str; 
    } 
 
    while (num != 0) 
    { 
        int rem = num % base; 
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0'; 
        num = num/base; 
    } 
  
    str[i] = '\0'; // Append string terminator 
  
    // Reverse the string 
    reverse(str, i); 
  
    return str; 
} 

void child(){
	int pid = getpid();
	int cid = getcid();
	while(1){
		recv(m);
		if(m[0] == 'p'){
			ps();
		}
		if(m[0] == 'e'){
			char filename[10] = "file_0";
			itoa(pid, filename+6);
			unlink(filename);

			char filename1[10] = "my_file_1";
			itoa(cid, filename1+8);
			unlink(filename1);

			leave_container();
			exit();
		}
		if(m[0] == 'h'){
			sleep(1);
			int a = 0;
			for(int i = 0; i < 100000; i++){
				for(int j = 0; j < 100000; j++){
					for(int j = 0; j < 100000; j++){
						a++;
					}
				}
			}
			//printf(1, "done heavy pid:%d\n", pid);
		}
		if(m[0] == 'l'){
			if(fork() == 0){
				join_container(cid);
				char *argv[] = {"ls", 0};
				exec("/ls",argv);
				printf(1, "child : ls exec failed\n");
				exit();
			}
			wait();
		}
		if(m[0] == 'c'){
			char filename[10] = "file_0";
			itoa(pid, filename+5);
			int fd = open(filename, O_CREATE);
			close(fd);
		}
		if(m[0] == 'd'){
			char filename[10] = "my_file";
			int fd = open(filename, O_CREATE|O_WRONLY);
			char text[15] = "Modified by: 0";
			itoa(pid, text+13);
			write(fd, text, 15);
			close(fd);
		}
		if(m[0] == 't'){
			if(fork() == 0){
				join_container(cid);
				char *argv[] = {"cat", "my_file", 0};
				exec("/cat",argv);
				printf(1, "child : cat exec failed\n");
				exit();
			}
			wait();
		}
		send(pid, parentId, m);
	}
}

int 
main(int argc ,char* argv[])
{
	int cid1=create_container();
	int cid2=create_container();
	int cid3=create_container();
	
	int child1, child2, child3, child4, child5;

	m = (char*)malloc(MSGSIZE);
	parentId = getpid();

	/*** Container 1 ***/
	//Child1
	if((child1 = fork()) == 0){
		join_container(cid1);
		send(getpid(), parentId, m);
		child();
		exit();
	}

	//Child2
	if((child2 = fork()) == 0){
		join_container(cid1);
		send(getpid(), parentId, m);
		child();
		exit();
	}

	//Child3
	if((child3 = fork()) == 0){
		join_container(cid1);
		send(getpid(), parentId, m);
		child();
		exit();
	}

	/*** Container 2 ***/
	//Child1
	if((child4 = fork()) == 0){
		join_container(cid2);
		send(getpid(), parentId, m);
		child();
		exit();
	}

	/*** Container 3 ***/
	//Child1
	if((child5 = fork()) == 0){
		join_container(cid3);
		send(getpid(), parentId, m);
		child();
		exit();
	}

	/* All child process joined container */
	for(int i = 0; i < 5; i++){
		recv(m);
	}

	/* All lead child to do ps */
	m[0] = 'p';
	send(parentId, child1, m);
	send(parentId, child4, m);
	send(parentId, child5, m);

	recv(m);
	recv(m);
	recv(m);

	printf(1, "\n");

	/* Scheduler logs */
	m[0] = 'h';
	send(parentId, child1, m);
	send(parentId, child2, m);
	send(parentId, child3, m);
	send(parentId, child4, m);
	send(parentId, child5, m);

	scheduler_log_on();
	sleep(2);
	scheduler_log_off();

	for(int i = 0; i < 5; i++){
		recv(m);
	}

	printf(1, "\n");

	/* File system test */
	/* ls() */
	m[0] = 'l';
	send(parentId, child1, m);
	recv(m);
	send(parentId, child2, m);
	recv(m);
	send(parentId, child3, m);
	recv(m);
	send(parentId, child4, m);
	recv(m);
	send(parentId, child5, m);	
	recv(m);

	printf(1, "\n");

	/* create file file_pid */
	m[0] = 'c';
	send(parentId, child1, m);
	send(parentId, child2, m);
	send(parentId, child3, m);
	send(parentId, child4, m);
	send(parentId, child5, m);

	for(int i = 0; i < 5; i++){
		recv(m);
	}

	printf(1, "created files\n\n");	

	/* ls() */
	m[0] = 'l';
	send(parentId, child1, m);
	recv(m);
	send(parentId, child2, m);
	recv(m);
	send(parentId, child3, m);
	recv(m);
	send(parentId, child4, m);
	recv(m);
	send(parentId, child5, m);	
	recv(m);

	printf(1, "\n");

	/* create my_file */
	m[0] = 'd';
	send(parentId, child1, m);
	send(parentId, child4, m);
	send(parentId, child5, m);

	recv(m);
	recv(m);
	recv(m);

	printf(1, "created my_file\n\n");	

	/* cat */
	m[0] = 't';
	send(parentId, child1, m);
	recv(m);
	send(parentId, child4, m);
	recv(m);
	send(parentId, child5, m);
	recv(m);

	printf(1, "\n");

	/* All child to exit */
	m[0] = 'e';
	send(parentId, child1, m);
	send(parentId, child2, m);
	send(parentId, child3, m);
	send(parentId, child4, m);
	send(parentId, child5, m);

	for(int i = 0; i < 5; i++){
		wait();
	}

	destroy_container(cid1);
	destroy_container(cid2);
	destroy_container(cid3);

	exit();
}
