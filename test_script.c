#include "types.h"
#include "user.h"
#include "date.h"
#include "fcntl.h"


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

int
main(int argc ,char* argv[])
{
	// ps();
	int cid1=create_container();
	int cid2=create_container();
	int cid3=create_container();

	int fd = open("my_file", O_CREATE);
	close(fd);
	
	int child = fork();
	// ps();
	if(child == 0){
		ps();
		join_container(cid1);
		// ps();
		int c1=getpid();
		printf(1,"pid=%d created\n", c1);
		for (int i = 0; i < 100; ++i)
		{
			i--;
		}
		
		// // sleep(100);
		// printf(1,"child 1 container 1\n");
		// int pid=getpid();
		// char *spid = (char *)malloc(4);
  //   	spid=itoa(pid,spid);
  //   	char s[15]="Modified by:  ";
  //   	int i=0;
  //   	while(spid[i]!='\0')
  //   	{
  //   		printf(1,"insdie while %c\n", spid[i]);
  //   		s[i+12]=spid[i];
  //   		i++;
  //   	}
		// ps();

		// fd = open("my_file", O_WRONLY);
		// // char s
		// write(fd,s,sizeof(s));
		// close(fd);
		// printf(1,"yaha hooo\n");

		// char *argv[2];
		// argv[0] ="cat";
		// argv[1] = 0;
		// if(fork() == 0) 
		// {
		// 	printf(1,"inside cat\n");
		// 	close(0);
		// 	open("my_file", O_RDONLY);
		// 	exec("cat", argv);
		// }


		// if(fork() == 0)
		// {
		// 	join_container(cid1);
		// 	printf(1, "ls inside cont 1\n");
		// 	char *argv[] = {"ls", 0};
		// 	exec("/ls",argv);
		// 	printf(1, "child : ls exec failed\n");
		// }

		leave_container();

		exit();
	}



	int child2 = fork();
	if(child2 == 0)
	{
		join_container(cid1);
		int c2=getpid();
		printf(1,"pid=%d created\n", c2);

		// printf(1,"child 2 container 1\n" );
		for (int i = 0; i < 100; ++i)
		{
			i--;
		}
		leave_container();

		exit();
	}

	int child3 = fork();
	if(child3 == 0)
	{
		join_container(cid1);
		int c3=getpid();

		printf(1,"pid=%d created\n", c3);

		// printf(1,"child 2 container 1\n" );
		// sleep(100);
		for (int i = 0; i < 100; ++i)
		{
			i--;
		}
		leave_container();

		exit();
	}


	if(fork() == 0)
	{
		join_container(cid2);
		int c4=getpid();

		printf(1,"pid=%d created\n", c4);

		// printf(1,"child in container 2\n" );
		// ps();
		for (int i = 0; i < 100; ++i)
		{
			i--;
		}
		leave_container();

		exit();
	}


	// int child3 = fork();
	// if(child3 == 0)
	// {
	// 	join_container(cid1);

	// 	leave_container();

	// 	exit();
	// }

	// int child4 = fork();
	// if(child4 == 0)
	// {
	// 	join_container(cid2);



	// 	int pid=getpid();
	// 	char *spid = (char *)malloc(4);
 //    	spid=itoa(pid,spid);
 //    	char s[15]="Modified by:  ";
 //    	int i=0;
 //    	while(spid[i]!='\0')
 //    	{
 //    		s[i+12]=spid[i];
 //    	}
	// 	ps();

	// 	fd = open("my_file", O_WRONLY);
	// 	// char s
	// 	write(fd,s,sizeof(s));
	// 	close(fd);


	// 	char *argv[2];
	// 	argv[0] ="cat";
	// 	argv[1] = 0;
	// 	if(fork() == 0) 
	// 	{
	// 		close(0);
	// 		open("my_file", O_RDONLY);
	// 		exec("cat", argv);
	// 	}

	// 	leave_container();

	// 	exit();
	// }


	// int child5 = fork();
	// if(child5 == 0)
	// {
	// 	join_container(cid3);

	// 	int pid=getpid();
	// 	char *spid = (char *)malloc(4);
 //    	spid=itoa(pid,spid);
 //    	char s[15]="Modified by:  ";
 //    	int i=0;
 //    	while(spid[i]!='\0')
 //    	{
 //    		s[i+12]=spid[i];
 //    	}
	// 	ps();

	// 	fd = open("my_file", O_WRONLY);
	// 	// char s
	// 	write(fd,s,sizeof(s));
	// 	close(fd);


	// 	char *argv[2];
	// 	argv[0] ="cat";
	// 	argv[1] = 0;
	// 	if(fork() == 0) 
	// 	{
	// 		close(0);
	// 		open("my_file", O_RDONLY);
	// 		exec("cat", argv);
	// 	}

	// 	leave_container();
	// 	exit();
	// }
	
	// wait();
	// wait();
	// wait();
	// wait();
	// wait(); //all child processes exit, destroy container now

	// int c = fork();
	// if(c == 0){
	// 	printf(1, "ls inside cont 0\n");
	// 	char *argv[] = {"ls", 0};
	// 	exec("/ls",argv);
	// 	printf(1, "child : ls exec failed\n");
	// }


	wait();
	wait();
	wait();
	wait();

	destroy_container(cid1);
	destroy_container(cid2);
	destroy_container(cid3);
	//unlink("yohoooooooo");


	exit();
}
