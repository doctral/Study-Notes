#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h> 
#include <sys/types.h>
#include <sys/wait.h>

#define READ  0
#define WRITE 1
#define MAX_LENGTH 128 
static char* args[MAX_LENGTH];    // command line input
static char line[MAX_LENGTH];     // command line input
static int n = 0;             

pid_t pid;              
int command_pipe[2];              // for pipe
char *his[100];	                  // for history, the 100 most recent commands
int curr=0;                       // array length of the most recent commands
char *all_his[1000];              // for histat, all possible commands
int all_curr=0;                   // array length of all possible commands

// print the 100 most recent commands
static int print_history(char *his[], int curr){

	printf("The history commands are showed as following:\n");
	for (int index=0; index<curr;index++){
		printf("%d %s\n",index+1,his[index]);
	}
	return 0;
}

// check duplicate, if yes, return 0, otherwise, return 1
static int check_dup(char *list[1000],char cmd[128],int length);
// get frequency of a specific command in all commands
static int get_frequency(char *list[1000],char cmd[128],int length);

// print the top 10 most frequent commands from all commands
static int print_histat(char *all_his[], int all_curr){
	printf("The top 10 most frequent commands are displayed as following:\n");
	char *unique_cmd[1000];   // for unique commands 
	int uni_index=0;          // total number of unique commands
	for(int i=0;i<1000; i++){
		unique_cmd[i]=NULL;
	}

// get all unique commands and the total number 
	for(int i=0; i<all_curr;i++){
		// if the command is already in the array of unique commands, then skip it
		if(check_dup(unique_cmd,all_his[i],uni_index)==0){
			continue;
		} 
		// else, store it
		else{
			free(unique_cmd[uni_index]);
			unique_cmd[uni_index]=strdup(all_his[i]);
			uni_index++;
		}
	}

	int freq[uni_index];   // for unique commands, frequency of each unique command
	for(int i=0; i<uni_index;i++){
		freq[i]=get_frequency(all_his,unique_cmd[i],all_curr);  // get frequency
	}
	printf("Frequency  Command\n");
// for histat, if the most frequent command has been printed out, then flag=0, otherwise, flag=1
	int flag[uni_index];     
	for(int i=0; i<uni_index;i++){
		flag[i]=1;
	}
	// if the total number of unique commands <10
	if(uni_index<10){
		int s=0;
		while(s<uni_index){
			int max=0;
			int max_index=0;
			for(int i=0; i<uni_index; i++){
				if(max<freq[i] && flag[i]==1){
					max=freq[i];
					max_index=i;
				}
			}
			flag[max_index]=0;
			printf("%d          %s\n",max,unique_cmd[max_index]);
			s++;
		}
	}
	// if the total number of unique commands >10
	else{
		int s=0;
		while(s<10){
			int max=0;
			int max_index=0;
			for(int i=0; i<uni_index;i++){
				if(max<freq[i] && flag[i]==1){
					max=freq[i];
					max_index=i;
				}
			}
			flag[max_index]=0;
			printf("%d          %s\n",max,unique_cmd[max_index]);
			s++;
		}
	}


    return 0;
}
// if string cmd has already been in the list, then return 0, otherwise, return 1.
static int check_dup(char *list[1000], char cmd[128], int length){
	for(int i=0; i<length; i++){

		if(strcmp(list[i],cmd)==0)
			return 0;
	}
	return 1; 
}
// get frequency of a specific command in all commands
static int get_frequency(char *list[1000],char cmd[128],int length){
	int f=0;
	for(int i=0; i<length; i++){
		if(strcmp(list[i],cmd)==0){
			f++;
		}
	}
	return f;

}

// execute command using fork() and execvp()
static int command(int input, int first, int last)
{
	int pipe_cmd[2];
	pipe(pipe_cmd);	
	pid = fork();

	if (pid == 0) {
		if (first == 1 && last == 0 && input == 0) {
			dup2( pipe_cmd[WRITE], STDOUT_FILENO );
		} else if (first == 0 && last == 0 && input != 0) {
			dup2(input, STDIN_FILENO);
			dup2(pipe_cmd[WRITE], STDOUT_FILENO);
		} else {
			dup2( input, STDIN_FILENO );
		}
		if (execvp( args[0], args) == -1)
			_exit(EXIT_FAILURE); 
	}
 
	if (input != 0) 
		close(input);
 
	close(pipe_cmd[WRITE]);
 
	if (last == 1)
		close(pipe_cmd[READ]);
 
	return pipe_cmd[READ];
}

// reset the state 
static void cleanup(int n)
{
	int i;
	for (i = 0; i < n; ++i) 
		wait(NULL); 
}
 
static int run(char* cmd, int input, int first, int last);

static void split(char* cmd);

// run the command based on method command and other methods 
static int run(char* cmd, int input, int first, int last)
{
	split(cmd);
	if (args[0] != NULL) {
		if (strcmp(args[0], "quit") == 0) 
			exit(0);
		if(strcmp(args[0],"history")==0)
			return print_history(his,curr);
		if(strcmp(args[0],"histat")==0)
			return print_histat(all_his,all_curr);
		n += 1;
		return command(input, first, last);
	}
	return 0;
}

// skip all white spaces 
static char* skipwhite(char* s)
{
	while (isspace(*s)) ++s;
	return s;
}

// split the command 
static void split(char* cmd)
{
	cmd = skipwhite(cmd);
	char* next = strchr(cmd, ' ');
	int i = 0;
 
	while(next != NULL) {
		next[0] = '\0';
		args[i] = cmd;
		++i;
		cmd = skipwhite(next + 1);
		next = strchr(cmd, ' ');
	}
 
	if (cmd[0] != '\0') {
		args[i] = cmd;
		next = strchr(cmd, '\n');
		next[0] = '\0';
		++i; 
	}
 
	args[i] = NULL;
}

int main()
{
	// initial both his[] and all_his[]
    curr=0;
	for(int i=0; i<100; i++){
		his[i]=NULL;
	}
	all_curr=0;
	for(int i=0; i<1000;i++){
		all_his[i]=NULL;
	}

	printf("Type \"quit\" to exit.\n");
	while (1) {
		printf("$> ");
		fflush(NULL);
 
		if (!fgets(line, 128, stdin)) 
			return 0;
		
		free(his[curr]);
		free(all_his[all_curr]);
		// if the input command is empty, then skip it
		if(strcmp(line,"\n")==0){
			continue;
		}
		// otherwise, store and execute valid command
		else{
			all_his[all_curr]=strdup(line);
		    his[curr]=strdup(line);
		    all_curr=(all_curr+1)%1000;
		    curr=(curr+1)%100;

		    int input = 0;
		    int first = 1;
		    char* cmd = line;
		    char* next = strchr(cmd, '|'); 
 
		    while (next != NULL) {
			
			    *next = '\0';
			    input = run(cmd, input, first, 0);
 
			    cmd = next + 1;
			    next = strchr(cmd, '|'); 
			    first = 0;
		}

		input = run(cmd, input, first, 1);
		cleanup(n);
		n = 0;
		}

	}
	return 0;
}