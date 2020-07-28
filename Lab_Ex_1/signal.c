#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <math.h>

int num_of_signals_rcvd = 0;

int get_random_signal_number(){
int sn = 9;
float temp;
while((sn==9) || (sn==10) || (sn == 19)){
	temp = ((float)rand()/RAND_MAX)*30;
	sn = floor(temp)+1;
}
return sn;
}

int get_random_process_number(int N, int K){
int pn = floor((rand()/(float)RAND_MAX)*(N*K+N+1));
if(pn == N*K+N+1)
	return pn-1;
else
	return pn;
}

void append_to_file(){
FILE * fp = fopen("./PIDs.txt", "a");
fprintf(fp, "%d\n", getpid());
fclose(fp);
}

int read_from_file(int pn){
int pid;
FILE * fp = fopen("./PIDs.txt", "r");
fseek(fp, pn*(sizeof(int)+1), SEEK_SET);
/*for(int i = 0; i < pn; i++)
	fscanf(fp, "%d", &pid);
*/
fscanf(fp, "%d", &pid);
fclose(fp);
return pid;
}

void sig_usr1_handler(int sig_num){
return;
}

void signal_handler(int sig_num){
num_of_signals_rcvd++;
//printf("Process %d received signal number %d.", getpid(), sig_num);
}

void set_signal_handlers(){
for(int i = 1; i < 32; i++)
	if ((i == 9) || (i == 19))
		continue;
	else if (i == 10)
		signal(i, sig_usr1_handler);
	else
		signal(i, signal_handler);
}

int main(int argc, char * argv[]){

int N = atoi(argv[1]); 
int K = atoi(argv[2]);
int L = atoi(argv[3]);
int M = atoi(argv[4]);
int children[N];
int childrenOfChildren[K];
int pid1 = 0, pid2 = 0, fd, pid, sn, pn, kill_pid, var = 0;

set_signal_handlers();

fd = creat("./PIDs.txt", 0777);

FILE * fp = fopen("./PIDs.txt", "w");
fprintf(fp, "%d\n", getpid());
fclose(fp);

for(int i = 0; i < N; i++){
	pid1 = fork();
	if(pid1 != 0)
		children[i] = pid1;
	else{
		set_signal_handlers();
		append_to_file();	
		break;
	}
}

if(pid1 == 0){
	for(int i = 0; i < K; i++){
		pid2 = fork();
		if(pid2 != 0)
			childrenOfChildren[i] = pid2;
		else{
			set_signal_handlers();
			append_to_file();
			pause();
			break;
		}
	}
	if(pid2 != 0)
		pause();
		
}
if(pid1 != 0){
	sleep(4); //sleep for 4s so that all processes are created.
	FILE * fp = fopen("./PIDs.txt","r");
	fscanf(fp, "%d", &pid);
	while(fscanf(fp, "%d", &pid) > 0)
		kill(pid, 10); //SIG_USR1(10) signal wakes up all child processes.
 	fclose(fp);
}

while(1){

	if(pid1 != 0){
		for(int i = 0; i < N; i++)
			waitpid(children[i], NULL, WNOHANG);
		int j = 0;
		for(j = 0; j < N; j++){
			if(kill(children[j], 0) == -1)
				continue;
			else
				break;
		}
		if(j == N)
			var = -1;
	}

	if((pid1 == 0) && (pid2 != 0)){
		for(int i = 0; i < K; i++)
			waitpid(childrenOfChildren[i], NULL, WNOHANG);
		int j = 0;
		for(j = 0; j < K; j++){
			if(kill(childrenOfChildren[j], 0) == -1)
				continue;
			else
				break;
		}
		if(j == K)
			var = -1;
	}

	if(var == -1){
		printf("Process %d exiting because all its children exited.\n", getpid());
		exit(0);
	}

	for(int i = 0; i < M; i++){
		sn = get_random_signal_number();
		pn = get_random_process_number(N, K);
		pid = read_from_file(pn);
		kill_pid = kill(pid, sn); //Upon successful completion, kill_pid = 0, else -1.
		printf("Process  %d sent signal number %d.\n", getpid(), sn);
		//printf("Process %d sent signal number %d to process %d.", getpid(), sn, pid);
	}

	if(num_of_signals_rcvd > L){
		printf("Process %d received %d number of signals. So, terminating.\n", getpid(), num_of_signals_rcvd);
		exit(0);
	}

}

return 0;

}

