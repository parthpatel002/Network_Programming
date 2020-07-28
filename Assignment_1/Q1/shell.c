#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>

#define BUFLEN 4096
#define LSH_TOK_DELIM " \t\r\n\a"

typedef struct {
  int num_of_commands;
  char ** command_list;
  int p; // 0 if only single pipes, 1 if a double pipe, 2 if a triple pipe
} commands;

char* lookup[100];



void handleSC(char* buf) {
    int index;
    sscanf(buf + 5, "%d", &index);
    char option = buf[4];
    if(option == 'i') {
        lookup[index] = (char*)malloc(200 * sizeof(char));
        memset(lookup[index], 0, 200);
        int i = 5;
        while(buf[i] == ' '){
            i++;
        }
        while(buf[i] != ' '){
            i++;
        }

        strcpy(lookup[index], buf + i);
    }
    else if(option == 'd') {
        lookup[index] = NULL;
    }
}

char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

int search(char ch, char* str) {
    int i = 0;
    while(str[i] != '\0') {
        if(str[i] == ch) {
            str[i] = '\0';
            return 1;
        }
        i++;
    }
    return 0;
}

char* sliceString(int left, int right, char* str) {
    char* s = (char*) malloc( (right - left + 1) * sizeof(char));
    int j = 0;
    for(int i = left; i <= right; i++){
        s[j++] = str[i];
    }

    return s;
}

int* getDescriptors(char* command) {
    int* fds = (int*) malloc(2*sizeof(int));
    char* inputfile = (char*) malloc(20);
    char* outputfile = (char*) malloc(20);
    //Default file numbers
    fds[0] = STDIN_FILENO;
    fds[1] = STDOUT_FILENO;
    int i = 0;
    int j = 0;
    while(command[i] != '\0') {
        if(command[i] == '>'){
            command[i-1] = '\0';
            j = 0;
            if(command[i+1] == '>') {
                if(command[i+2] == ' '){
                    i+=2;
                    while(command[i] == ' '){
                        i++;
                    }
                    while(command[i]!= '\0' && command[i] != ' '){
                        outputfile[j++] = command[i++];
                    }
                    int outputfd = open(outputfile, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
                    fds[1] = outputfd;
                }
                else{
                    i+=2;
                    while(command[i]!= '\0' && command[i] != ' '){
                        outputfile[j++] = command[i++];
                    }
                    int outputfd = open(outputfile, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
                    fds[1] = outputfd;
                }
            }
            else if(command[i+1] == ' '){
                //single >
                i+=2;
                while(command[i]!= '\0' && command[i] != ' '){
                        outputfile[j++] = command[i++];
                    }
                int outputfd = open(outputfile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                fds[1] = outputfd;
            }
            else{
                i+=1;
                while(command[i]!= '\0' && command[i] != ' '){
                        outputfile[j++] = command[i++];
                    }
                int outputfd = open(outputfile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                fds[1] = outputfd;
            }
        }
        else if(command[i] == '<') {
            command[i-1] = '\0';
            j = 0;
            if(command[i+1] == ' '){
                //single <
                i+=2;
                while(command[i]!= '\0' && command[i] != ' '){
                        inputfile[j++] = command[i++];
                    }
                int inputfd = open(inputfile, O_RDONLY);
                fds[0] = inputfd;
            }
            else{
                i+=1;
                while(command[i]!= '\0' && command[i] != ' '){
                        inputfile[j++] = command[i++];
                    }
                int inputfd = open(inputfile, O_RDONLY);
                fds[0] = inputfd;
            }
        }
        i++;
    }
    return fds;
}

char ** tokenize(char *line) {
  int bufsize = 64, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += 64;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

commands* process_command(char * str){
	commands* c = (commands*)malloc(sizeof(commands));
  	c->num_of_commands = 1;
    c->p = 0; 
    int i = 0;
    
    while(str[i] != '\0') {
        if(str[i] == '|'){
            if((i+1) < strlen(str) && str[i+1] == '|') {
                if((i+2) < strlen(str) && str[i+2] == '|') {
                    //triple pipe
                    c->num_of_commands++;
                    i+=2;
                    c-> p = 2;
                }
                else {
                    //double pipe
                    c->num_of_commands++;
                    i+=1;
                    c-> p = 1;
                }
            }
            else {
                c->num_of_commands++;
            }
        }
        i++;
    }
    // No of commands obtained

    c->command_list = (char**) malloc(sizeof(char*) * c->num_of_commands);
    
    i = 0;
    int left = 0;
    int command_num = 0;

    while(str[i] != '\0') {
        if(str[i] == '|'){
            if(str[i+1] == '|') {
                if(str[i+2] == '|'){
                    //triple pipe
                    c->command_list[command_num++] = sliceString(left, i-1, str);
                    left = i + 3;
                    i+=2;
                }
                else {
                    //double pipe
                    c->command_list[command_num++] = sliceString(left, i-1, str);
                    left = i + 2;
                    i+=1;
                }
            }
            else {
                c->command_list[command_num++] = sliceString(left, i-1, str);
                left = i + 1;
            }
        }
        i++;
    }

    c->command_list[command_num] =  sliceString(left, i-1, str);

    return c;
}

char* findPath(char* token0) {
    const char* path = getenv("PATH");
    char* buf;
    char buf3[400];
    strcpy(buf3, path);
    char* buf2 = (char*)malloc(50*sizeof(char));
    int i = 0;int left = 0;
    while(buf3[i] != '\0') {
        if(buf3[i] == ':') {
            buf = sliceString(left, i-1, buf3);
            left = i+1;
            strcpy(buf2, buf);
            buf2 = strcat(buf2, "/");
            buf2 = strcat(buf2, token0);
            if(access(buf2, X_OK) == 0) {
                return buf2;
            }
        }
        i++;
    }
    buf = sliceString(left, i-1, buf3);
    strcpy(buf2, buf);
    buf2 = strcat(buf2, "/");
    buf2 = strcat(buf2, token0);
    if(access(buf2, X_OK) == 0) {
        return buf2;
    }
    return NULL;

}

void execute(char* buf) {
    commands* c = process_command(buf);
        int rem_commands = c->num_of_commands - 1;
        
        int pipeArray[c->num_of_commands + c->p][2];
        for(int i = 0; i < c->num_of_commands + c->p; i++){
            pipe(pipeArray[i]);
            printf("Pipe fds of pipe %d : Read end %d, Write end %d\n", i, pipeArray[i][0], pipeArray[i][1]);
        }


        for(int i = 0; i < c->num_of_commands; i++, rem_commands--) {
            //tokenize ith command
            char commandbuf[50];
            strcpy(commandbuf, c->command_list[i]);
            char ** tokens = tokenize(c->command_list[i]);

            if(i == 0) {
                if(rem_commands == 0) {
                    //one command
                    
                    int* filedescriptors = getDescriptors(commandbuf);
                    if(filedescriptors[0] != STDIN_FILENO) {
                        printf("Standard input 0 remapped to fd %d\n", filedescriptors[0]);
                    }
                    if(filedescriptors[1] != STDOUT_FILENO) {
                        printf("Standard output 1 remapped to fd %d\n", filedescriptors[1]);
                    }
                    int pid = fork();
                    if (pid == 0) {
                        // Child process
                        dup2(filedescriptors[0], STDIN_FILENO);
                        dup2(filedescriptors[1], STDOUT_FILENO);
                        tokens = tokenize(commandbuf);
                        
                        char* result = findPath(tokens[0]);
                        if(result == NULL){
                            perror("Error! No such executable file");
                        }
                        else{
                            if (execv(result, tokens) == -1) {
                            perror("error in execv\n");
                        }
                        exit(EXIT_FAILURE);
                        }

                    } else if (pid < 0) {
                        // Error forking
                        perror("Error forking");
                    } else {
                        // Parent process
                        waitpid(pid, NULL, WUNTRACED);
                        printf("Process id for process used is %d\n", pid);
                    }

                }

                else {
                    // first command in a chain of commands
                    int* filedescriptors = getDescriptors(commandbuf);

                    if(filedescriptors[0] != STDIN_FILENO) {
                        printf("Standard input 0 remapped to fd %d\n", filedescriptors[0]);
                    }
                    if(filedescriptors[1] != STDOUT_FILENO) {
                        printf("Standard output 1 remapped to fd %d\n", filedescriptors[1]);
                    }

                    int pid = fork();
                    if(pid == 0) {
                        //child
                        close(pipeArray[0][0]);
                        dup2(filedescriptors[0], STDIN_FILENO);
                        dup2(pipeArray[0][1], STDOUT_FILENO);
                        tokens = tokenize(commandbuf);
                        // if (execvp(tokens[0], tokens) == -1) {
                        //     perror("error in execvp\n");
                        // }
                        // exit(EXIT_FAILURE);
                        char* result = findPath(tokens[0]);
                        if(result == NULL){
                            perror("Error! No such executable file");
                        }
                        else{
                            if (execv(result, tokens) == -1) {
                            perror("error in execv\n");
                        }
                        exit(EXIT_FAILURE);
                        }
                    }
                    else if (pid < 0) {
                        perror("Error forking");
                    }
                    else {
                        waitpid(pid, NULL, WUNTRACED);
                        printf("Process id for process used is %d\n", pid);
                    }
                }

            }
            
            else if (i == c->num_of_commands - 1) {
                //last command, read from some pipe, write to stdout / file descriptors
                if(c-> p == 0){
                    int* filedescriptors = getDescriptors(commandbuf);
                    if(filedescriptors[0] != STDIN_FILENO) {
                        printf("Standard input 0 remapped to fd %d\n", filedescriptors[0]);
                    }
                    if(filedescriptors[1] != STDOUT_FILENO) {
                        printf("Standard output 1 remapped to fd %d\n", filedescriptors[1]);
                    }
                    int pid = fork();
                    if(pid == 0) {
                        //child
                        close(pipeArray[i-1][1]);
                        dup2(pipeArray[i-1][0], STDIN_FILENO);
                        dup2(filedescriptors[1], STDOUT_FILENO);
                        tokens = tokenize(commandbuf);
                        // if (execvp(tokens[0], tokens) == -1) {
                        //     perror("error in execvp\n");
                        // }
                        // exit(EXIT_FAILURE);
                        char* result = findPath(tokens[0]);
                        if(result == NULL){
                            perror("Error! No such executable file");
                        }
                        else{
                            if (execv(result, tokens) == -1) {
                            perror("error in execv\n");
                        }
                        exit(EXIT_FAILURE);
                        }
                    }
                    else if (pid < 0) {
                        perror("Error forking");
                    }
                    else {
                        close(pipeArray[i-1][0]);
                        close(pipeArray[i-1][1]);
                        waitpid(pid, NULL, WUNTRACED);
                        printf("Process id for process used is %d\n", pid);
                    }
                }
                else if(c-> p == 1) {
                    //delimit last command using commas
                   
                    char* command1 = strtok(commandbuf, ",");
                    char* command2 = strtok(NULL, ",");
                   
                    char ** tokens1 = tokenize(command1);
                    char ** tokens2 = tokenize(command2);
                   
                    //read from pipe into a buffer
                    char buf[BUFLEN];
                    int nbread = read(pipeArray[c->num_of_commands + c->p - 3][0], buf, BUFLEN);
                    
                    //write buffer into write ends of both pipes
                    
                    
                    
                    
                    //create two new processes, each associated with a pipe
                    int pid1 = fork();
                    if(pid1 == 0) {
                        //child
                        close(pipeArray[c->num_of_commands + c->p -2][1]);
                        dup2(pipeArray[c->num_of_commands + c->p - 2][0], STDIN_FILENO);
                        close(pipeArray[c->num_of_commands + c->p -1][1]);
                        close(pipeArray[c->num_of_commands + c->p -1][0]);
                        // if (execvp(tokens1[0], tokens1) == -1) {
                        //     perror("error in execvp\n");
                        // }
                        // exit(EXIT_FAILURE);
                        char* result = findPath(tokens1[0]);
                        if(result == NULL){
                            perror("Error! No such executable file");
                        }
                        else{
                            if (execv(result, tokens1) == -1) {
                            perror("error in execv\n");
                        }
                        exit(EXIT_FAILURE);
                        }
                    }
                    else if (pid1 < 0) {
                        perror("Error forking");
                    }
                    else {
                        write(pipeArray[c->num_of_commands + c->p - 2][1], buf, nbread*sizeof(char));
                        close(pipeArray[c->num_of_commands + c->p - 2][0]);
                        close(pipeArray[c->num_of_commands + c->p - 2][1]);
                        
                    }

                    
                    int pid2 = fork();
                    if(pid2 == 0) {
                        //child
                        close(pipeArray[c->num_of_commands + c->p  -1][1]);
                        dup2(pipeArray[c->num_of_commands + c->p  - 1][0], STDIN_FILENO);
                        close(pipeArray[c->num_of_commands + c->p  -2][1]);
                        close(pipeArray[c->num_of_commands + c->p  -2][0]);
                        // if (execvp(tokens2[0], tokens2) == -1) {
                        //     perror("error in execvp\n");
                        // }
                        // exit(EXIT_FAILURE);
                        char* result = findPath(tokens2[0]);
                        if(result == NULL){
                            perror("Error! No such executable file");
                        }
                        else{
                            if (execv(result, tokens2) == -1) {
                            perror("error in execv\n");
                        }
                        exit(EXIT_FAILURE);
                        }
                    }
                    else if (pid2 < 0) {
                        perror("Error forking");
                    }
                    else {
                        write(pipeArray[c->num_of_commands + c->p - 1][1], buf, nbread * sizeof(char));
                        close(pipeArray[c->num_of_commands + c->p - 1][0]);
                        close(pipeArray[c->num_of_commands + c->p - 1][1]);
                        waitpid(pid2, NULL, WUNTRACED);
                        waitpid(pid1, NULL, WUNTRACED);
                        printf("Process id for process used is %d\n", pid1);
                        printf("Process id for process used is %d\n", pid2);
                    }
                }
                else if(c-> p == 2) {
                    //delimit last command using commas
                    char* command1 = strtok(commandbuf, ",");
                    char* command2 = strtok(NULL, ",");
                    char* command3 = strtok(NULL, ",");
                    char ** tokens1 = tokenize(command1);
                    char ** tokens2 = tokenize(command2);
                    char ** tokens3 = tokenize(command3);
                    //read from pipe into a buffer
                    char buf[BUFLEN];
                    int nbread = read(pipeArray[c->num_of_commands + c->p - 4][0], buf, BUFLEN);
                    //write buffer into write ends of both pipes

                    //create two new processes, each associated with a pipe
                    int pid1 = fork();
                    if(pid1 == 0) {
                        //child
                        close(pipeArray[c->num_of_commands + c->p -3][1]);
                        dup2(pipeArray[c->num_of_commands + c->p - 3][0], STDIN_FILENO);
                        close(pipeArray[c->num_of_commands + c->p -2][1]);
                        close(pipeArray[c->num_of_commands + c->p -2][0]);
                        close(pipeArray[c->num_of_commands + c->p -1][1]);
                        close(pipeArray[c->num_of_commands + c->p -1][0]);
                        // if (execvp(tokens1[0], tokens1) == -1) {
                        //     perror("error in execvp\n");
                        // }
                        // exit(EXIT_FAILURE);
                        char* result = findPath(tokens1[0]);
                        if(result == NULL){
                            perror("Error! No such executable file");
                        }
                        else{
                            if (execv(result, tokens1) == -1) {
                            perror("error in execv\n");
                        }
                        exit(EXIT_FAILURE);
                        }
                    }
                    else if (pid1 < 0) {
                        perror("Error forking");
                    }
                    else {
                        write(pipeArray[c->num_of_commands + c->p - 3][1], buf, nbread*sizeof(char));
                        close(pipeArray[c->num_of_commands + c->p - 3][0]);
                        close(pipeArray[c->num_of_commands + c->p - 3][1]);
                    }

                    
                    int pid2 = fork();
                    if(pid2 == 0) {
                        //child
                        close(pipeArray[c->num_of_commands + c->p -2][1]);
                        dup2(pipeArray[c->num_of_commands + c->p - 2][0], STDIN_FILENO);
                        close(pipeArray[c->num_of_commands + c->p -1][1]);
                        close(pipeArray[c->num_of_commands + c->p -1][0]);
                        close(pipeArray[c->num_of_commands + c->p -3][1]);
                        close(pipeArray[c->num_of_commands + c->p -3][0]);
                        // if (execvp(tokens2[0], tokens2) == -1) {
                        //     perror("error in execvp\n");
                        // }
                        // exit(EXIT_FAILURE);
                        char* result = findPath(tokens2[0]);
                        if(result == NULL){
                            perror("Error! No such executable file");
                        }
                        else{
                            if (execv(result, tokens2) == -1) {
                            perror("error in execv\n");
                        }
                        exit(EXIT_FAILURE);
                        }
                    }
                    else if (pid2 < 0) {
                        perror("Error forking");
                    }
                    else {
                        write(pipeArray[c->num_of_commands + c->p - 2][1], buf, nbread*sizeof(char));
                        close(pipeArray[c->num_of_commands + c->p - 2][0]);
                        close(pipeArray[c->num_of_commands + c->p - 2][1]);
                    }

                    int pid3 = fork();
                    if(pid3 == 0) {
                        //child
                        close(pipeArray[c->num_of_commands + c->p -1][1]);
                        dup2(pipeArray[c->num_of_commands + c->p - 1][0], STDIN_FILENO);
                        close(pipeArray[c->num_of_commands + c->p -2][1]);
                        close(pipeArray[c->num_of_commands + c->p -2][0]);
                        close(pipeArray[c->num_of_commands + c->p -3][1]);
                        close(pipeArray[c->num_of_commands + c->p -3][0]);
                        // if (execvp(tokens3[0], tokens3) == -1) {
                        //     perror("error in execvp\n");
                        // }
                        // exit(EXIT_FAILURE);
                        char* result = findPath(tokens3[0]);
                        if(result == NULL){
                            perror("Error! No such executable file");
                        }
                        else{
                            if (execv(result, tokens3) == -1) {
                            perror("error in execv\n");
                        }
                        exit(EXIT_FAILURE);
                        }
                    }
                    else if (pid3 < 0) {
                        perror("Error forking");
                    }
                    else {
                        write(pipeArray[c->num_of_commands + c->p - 1][1], buf, nbread*sizeof(char));
                        close(pipeArray[c->num_of_commands + c->p - 1][0]);
                        close(pipeArray[c->num_of_commands + c->p - 1][1]);
                        
                        waitpid(pid3, NULL, WUNTRACED);
                        waitpid(pid2, NULL, WUNTRACED);
                        waitpid(pid1, NULL, WUNTRACED);
                        printf("Process id for process used is %d\n", pid1);
                        printf("Process id for process used is %d\n", pid2);
                        printf("Process id for process used is %d\n", pid3);
                    }
                }
            }
            else {
                //middle command, read from some pipe, write to some pipe
                int pid = fork();
                    if(pid == 0) {
                        //child
                        close(pipeArray[i-1][1]);
                        dup2(pipeArray[i-1][0], STDIN_FILENO);
                        close(pipeArray[i][0]);
                        dup2(pipeArray[i][1], STDOUT_FILENO);
                        // if (execvp(tokens[0], tokens) == -1) {
                        //     perror("error in execvp\n");
                        // }
                        // exit(EXIT_FAILURE);
                        char* result = findPath(tokens[0]);
                        if(result == NULL){
                            perror("Error! No such executable file");
                        }
                        else{
                            if (execv(result, tokens) == -1) {
                            perror("error in execv\n");
                        }
                        exit(EXIT_FAILURE);
                        }
                    }
                    else if (pid < 0) {
                        perror("Error forking");
                    }
                    else {
                        close(pipeArray[i-1][0]);
                        close(pipeArray[i-1][1]);
                        waitpid(pid, NULL, WUNTRACED);
                        printf("Process id for process used is %d\n", pid);
                    }
            }
        }
}

void sighandle(int signo){
    printf("You have entered short cut sc mode\n");
    int index;
    scanf("%d", &index);
    if(lookup[index] == NULL) {
        printf("No entry exists for index %d\n", index);
        return;
    }
    int isBackground = search('&', lookup[index]);
    int pid = fork();
    if(pid > 0) { //top level parent
        if(!isBackground) {
            tcsetpgrp(STDIN_FILENO, pid);
            waitpid(pid, NULL, WUNTRACED);
        }
    }
    else {
        setpgid(0, getpid());
        execute(lookup[index]);
        if(!isBackground) {
            tcsetpgrp(STDIN_FILENO, getppid());
        }
        exit(0);
    }
}

char* trimleadingspaces(char* buf) {
    
    while(buf[0] == ' '){
        buf++;
    }
    return buf;
}



int main(){

char * buf = malloc(sizeof(char) * BUFLEN);
memset(buf, 0, sizeof(char) * BUFLEN);
signal(SIGINT, sighandle);

while(1){
    printf(">");
	fgets(buf, BUFLEN, stdin);
    buf[strlen(buf)-1] = '\0';
    if(strcmp(buf, "exit") == 0) {
        exit(0);
    }
    buf = trimleadingspaces(buf);
    
    if(buf[0] != '\0' && buf[1] != '\0' && buf[2] != '\0' && buf[0] == 's' && buf[1] == 'c' && buf[2] == ' ') {
        handleSC(buf);
        continue;
    }
    int isBackground = search('&', buf);
    int pid = fork();
    if(pid > 0) { //top level parent
        if(!isBackground) {
            int status;
            tcsetpgrp(STDIN_FILENO, pid);
            waitpid(pid, &status, WUNTRACED);
            printf("Process id is %d and exit status is %d\n", pid, status);
        }
    }
    else {
        setpgid(0, getpid());
        execute(buf);
        if(!isBackground) {
            tcsetpgrp(STDIN_FILENO, getppid());
        }
        exit(0);
    }
}

return 0;
}






