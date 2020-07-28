#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include<syslog.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/select.h>
#include<sys/types.h>
#include<signal.h>
#include<sys/wait.h>
#include<pwd.h>
#include<errno.h>


#define BD_NO_CHDIR 01
#define BD_NO_CLOSE_FILES 02 
#define BD_NO_REOPEN_STD_FDS 04
#define BD_NO_UMASK0 010
#define BD_MAX_CLOSE 8192
#define MAXOUTSTANDING 20

typedef struct mapper {
    int pid;
    int sockfd;
    char path_to_exe[150];
    int protocol_field; // 6 for tcp, 17 for udp
    int wait_no_wait; // 1 for wait, 0 for no wait
    char servicename[150];
    char username[150];
    unsigned short portno;
    char* CLA;
} mapper;

mapper table[100];
int table_index = 0;
fd_set masterset, workingset;
int maxsd = -1;
int filefd;
struct sockaddr_in si_other;

void populateMapper(int sockfd, char *path_to_exe, int protocol_field, int wait_no_wait, char *servicename, char *username, unsigned short portno, char* CLA) {
    table[table_index].sockfd = sockfd;
    table[table_index].protocol_field = protocol_field;
    // printf("Proto field: %d\n", protocol_field);
    table[table_index].wait_no_wait = wait_no_wait;
    table[table_index].portno = portno;
    strcpy(table[table_index].path_to_exe, path_to_exe);
    strcpy(table[table_index].servicename, servicename);
    strcpy(table[table_index].username, username);

    if(CLA != NULL) {
        table[table_index].CLA = (char*) malloc(sizeof(char) * 150);
        strcpy(table[table_index].CLA, CLA);
    }
    else
        table[table_index].CLA = NULL;
    
    table[table_index].pid = -1;
    // printf("Proto field: %d\n", table[table_index].protocol_field);
    table_index++;
}

unsigned short getPortNo(char* servicename, char* protocol_field) {
    FILE* fp = fopen("/etc/services", "r");
    if(fp == NULL) {
        perror("fp");
        exit(0);
    }
    char line[128];
    char* token1,* token2, * token3, *rest;
    while(fgets(line, 128, fp) != NULL) {
        rest = line;
        token1 = strtok_r(rest, " \t", &rest);
        // printf("%s\n", token);
        token2 = strtok_r(rest, "/", &rest);
        // printf("%d\n", atoi(token));
        token3 = strtok_r(rest, " \n\t", &rest);
        // printf("%s", token);
        if(strcmp(token1, servicename) == 0 && strcmp(protocol_field, token3) == 0) {
            return atoi(token2);
        }
    }
    return -1;
}

void createSockets() {
    FILE* fp = fopen("my_inetd.conf", "r");
    if(fp == NULL) {
        perror("fopen");
        exit(0);
    }
    printf("85\n");
    char line[256];
    unsigned short int portno;
    int sockfd;
    
    FD_ZERO(&masterset);

    struct sockaddr_in si_me;

    char *token1,*token2,*token3,*token4,*token5,*token6,*token7;
    char* rest;
    while(fgets(line, 256, fp) != NULL) {
        if(line[0] == '#')
            continue;
        rest = line;
        token1 = strtok_r(rest, " \t", &rest);
        // printf("%s\n", token1);
        token2 = strtok_r(rest, " \t", &rest);
        // printf("%s\n", token2);
        token3 = strtok_r(rest, " \t", &rest);
        // printf("%s\n", token3);
        token4 = strtok_r(rest, " \t", &rest);
        // printf("%s\n", token4);
        token5 = strtok_r(rest, " \t", &rest);
        // printf("%s\n", token5);
        token6 = strtok_r(rest, " \t\n", &rest);
        // printf("%s\n", token6);
        if(rest != NULL) {
            token7 = strtok_r(rest, "\n", &rest);
            // printf("%s\n", token7);
        }
        else 
            token7 = NULL;
        
        // printf("106\n");
        portno = getPortNo(token1, token3);
        // printf("port no %d\n", portno);
        // printf("108\n");
        if(portno > 0) {
            sockfd = socket(AF_INET,  (strcmp(token3, "tcp") == 0) ? SOCK_STREAM : SOCK_DGRAM, 0);
            if(sockfd < 0) {
                perror("socket:");
                exit(0);
            }
            memset((char*) &si_me, 0, sizeof(si_me));
            si_me.sin_addr.s_addr = htonl(INADDR_ANY);
            si_me.sin_family = AF_INET;
            si_me.sin_port = htons(portno);
            if(bind(sockfd, (struct sockaddr*) &si_me, sizeof(si_me)) == -1 ) {
                perror("bind()");
                exit(0);
            }
            if(strcmp(token3, "tcp") == 0) {
                if(listen(sockfd, MAXOUTSTANDING) == -1) {
                    perror("listen()");
                    exit(0);
                }
            }
            // printf("129\n");
            populateMapper(sockfd, token6, strcmp("tcp", token3) == 0? 6 : 17, strcmp("wait", token4) == 0? 1 : 0, token1, token5, portno, token7);
            // printf("131\n");
            FD_SET(sockfd, &masterset);
            
            if(maxsd < sockfd) {
                maxsd = sockfd;
            }
            
        }
    } // end while
}

void handler(int signo) {
    int pid;
	int status;

	if((pid = waitpid(-1, &status, WNOHANG)) == -1) {
		perror("waitpid");
		exit(0);
	}

	for(int i = 0; i < table_index; i++)
		if(table[i].pid == pid && table[i].wait_no_wait == 1) {
			table[i].pid = -1;
			FD_SET(table[i].sockfd, &masterset);
			break;
		}
    
    // char buf[200];
    // sprintf(buf, "%s%u%s%s%d%s", "inetd: Child ", (unsigned)pid, WIFEXITED(status) ? " exited": " was terminated", 
    // " with error code ",WEXITSTATUS(status), "\n");
    
    // write(1, buf, strlen(buf));

    // if(FD_ISSET(table[1].sockfd, &masterset)) {
        // printf("fafadf\n");
    // }
}

int main(int argc, char* argv[]) {
    int maxfd, fd;
    int flags = 0;

    switch (fork()) {
    case -1: return -1;
    case 0: break;
    default: _exit(EXIT_SUCCESS);
    }

    if (setsid() == -1)
        return -1;

    switch (fork()) {
        case -1: return -1;
        case 0: break;
        default: _exit(EXIT_SUCCESS);
    }

    if (!(flags & BD_NO_UMASK0))
        umask(0);

    if (!(flags & BD_NO_CHDIR))
        chdir("/");

    if (!(flags & BD_NO_CLOSE_FILES)) { 
        maxfd = sysconf(_SC_OPEN_MAX);
        if (maxfd == -1)
            maxfd = BD_MAX_CLOSE;
        for (fd = 0; fd < maxfd; fd++)
            close(fd);
    }

    if (!(flags & BD_NO_REOPEN_STD_FDS)) {
        close(STDIN_FILENO);
        fd = open("/dev/null", O_RDWR);
        if (fd != STDIN_FILENO)
            return -1;
        if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
            return -1;
        if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
            return -1;
    }

    // filefd = open("/home/sankalp/Desktop/NetProg_Assignment_3/debug.txt", O_RDWR | O_CREAT);
    // dup2(filefd, 1);
    // dup2(filefd, 2);
    // DAEMON PROCESS CREATED
    // printf("Here\n");
    createSockets();
    // printf("outside sockets\n");

    
    int rc;
    struct sigaction act;
	memset (&act, '\0', sizeof(act));
    act.sa_handler = &handler;
    act.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &act, NULL) < 0) {
		perror ("sigaction");
		return 1;
	}

    while(1) {
        memcpy(&workingset, &masterset, sizeof(masterset));
        // if(FD_ISSET(table[1].sockfd, &workingset)) {
            // printf("fafadf\n");
        // }
        rc = select(maxsd + 1, &workingset, NULL, NULL, NULL);
        // printf("245\n");
        // printf("%d\n", rc);
        if(rc < 0) {
            if(errno == EINTR) {
                continue;
            }
            perror("select()");
            
            exit(1);
        }
        else if(rc > 0) {
            // printf("251\n");
            // printf("Tableindex:%d\n", table_index);
            for(int i = 0; i < table_index; i++) {
                // printf("253\n");
                // printf("server is of %s\n", table[i].servicename);
                if(FD_ISSET(table[i].sockfd, &workingset)) {
                    // printf("255\n");
                    // printf("protocol field is %d\n", table[i].protocol_field);
                    if(table[i].protocol_field == 6) {
                        // printf("257\n");
                        int clilen = sizeof(si_other);
                        int connfd = accept(table[i].sockfd, (struct sockaddr*) &si_other, &clilen);
                        // printf("265 connfd is %d\n", connfd);
                        int pid = fork();
                        if(pid == 0) {
                                // printf("268\n");
                                for(int j = 0; j < 1024; j++) {
                                    if(j == connfd) {
                                        continue;
                                    }
                                    close(j);
                                }
                                dup2(connfd, 0);
                                dup2(connfd, 1);
                                dup2(connfd, 2);
                                close(connfd);
                                // FILE* fp1 = fopen("/home/sankalp/Desktop/NetProg_Assignment_3/child.txt", "w");
                                // fprintf(fp1, "280\n");
                                // fclose(fp1);
                                // if(strcmp(table[i].username, "root") != 0) {
                                //     struct passwd* pwd = getpwnam(table[i].username);
                                //     setuid(pwd->pw_uid);
                                //     setgid(pwd->pw_gid);
                                // }
                                char temp[256];
                                // fp1 = fopen("/home/sankalp/Desktop/NetProg_Assignment_3/child.txt", "a");
                                // fprintf(fp1, "289\n");
                                // fclose(fp1);
                                
                                // fp1 = fopen("/home/sankalp/Desktop/NetProg_Assignment_3/child.txt", "a");
                                // fprintf(fp1, "293\n");
                                // fclose(fp1);
                                if(table[i].CLA != NULL){
                                    strcpy(temp, table[i].CLA);
                                    char* token, *rest;
                                    int cnt = 0;
                                    rest = temp;

                                    while( (token = strtok_r(rest, " \t\n", &rest)) != NULL ) {
                                        cnt++;
                                    }

                                    char temp2[256];
                                    strcpy(temp2, table[i].CLA);
                                    char* arr[cnt + 2];
                                    arr[0] = table[i].path_to_exe;
                                    arr[cnt+1] = NULL;
                                    rest = temp2;
                                    for(int j = 1; j <= cnt; j++) {
                                        arr[j] = strtok_r(rest, " \t\n", &rest);
                                    }
                                    // fp1 = fopen("/home/sankalp/Desktop/NetProg_Assignment_3/child.txt", "a");
                                    // fprintf(fp1, "%s", table[i].path_to_exe);
                                    // fclose(fp1);
                                    execv(table[i].path_to_exe, arr);
                                    exit(1);
                                }
                                else {
                                    char* arr[2];
                                    // arr[0] = (char*) malloc(sizeof(char) * 100);
                                    // strcpy(arr[0], table[i].path_to_exe);
                                    // strcpy(arr[0], "echoserver");
                                    arr[0] = table[i].path_to_exe;
                                    // arr[0] = table[i].path_to_exe;
                                    arr[1] = NULL;
                                    // fp1 = fopen("/home/sankalp/Desktop/NetProg_Assignment_3/child.txt", "a");
                                    // fprintf(fp1, "%s", table[i].path_to_exe);
                                    // fclose(fp1);
                                    int p = execv(table[i].path_to_exe, arr);
                                    // fp1 = fopen("/home/sankalp/Desktop/NetProg_Assignment_3/child.txt", "a");
                                    // fprintf(fp1, "323 %d %d\n", p, errno);
                                    // fclose(fp1);
                                    exit(1);
                                }
                                
                        }
                        else {
                            close(connfd);
                            // printf("358\n");
                            if(table[i].wait_no_wait == 1) {
                                // printf("360\n");
                                table[i].pid = pid;
                                // printf("Child pid is %d\n", pid);
                                FD_CLR(table[i].sockfd, &masterset);
                            }
                        }
                    }
                    else if(table[i].protocol_field == 17) {
                        int pid = fork();
                        if(pid == 0) {
                                for(int j = 0; j < 1024; j++) {
                                    if(j == table[i].sockfd ) {
                                        continue;
                                    }
                                    close(j);
                                }
                                dup2(table[i].sockfd, 0);
                                dup2(table[i].sockfd, 1);
                                dup2(table[i].sockfd, 2);
                                close(table[i].sockfd);
                                
                                // if(strcmp(table[i].username, "root") != 0) {
                                //     struct passwd* pwd = getpwnam(table[i].username);
                                //     setuid(pwd->pw_uid);
                                //     setgid(pwd->pw_gid);
                                // }
                                if(table[i].CLA != NULL) {
                                    char temp[256];
                                    strcpy(temp, table[i].CLA);
                                    char* token, *rest;
                                    int cnt = 0;
                                    rest = temp;
                                    
                                    

                                    while( (token = strtok_r(rest, " \t\n", &rest)) != NULL ) {
                                        cnt++;
                                    }

                                    char temp2[256];
                                    strcpy(temp2, table[i].CLA);
                                    char* arr[cnt + 2];
                                    arr[0] = table[i].path_to_exe;
                                    arr[cnt+1] = NULL;
                                    rest = temp2;
                                    for(int j = 1; j <= cnt; j++) {
                                        arr[j] = strtok_r(rest, " \t\n", &rest);
                                    }
                                    
                                    execv(table[i].path_to_exe, arr);
                                }
                                else {
                                    char* arr[2];
                                    // arr[0] = (char*) malloc(sizeof(char) * 100);
                                    // strcpy(arr[0], table[i].path_to_exe);
                                    arr[0] = table[i].path_to_exe;
                                    // arr[0] = table[i].path_to_exe;
                                    arr[1] = NULL;
                                    // fp1 = fopen("/home/sankalp/Desktop/NetProg_Assignment_3/child.txt", "a");
                                    // fprintf(fp1, "%s", table[i].path_to_exe);
                                    // fclose(fp1);
                                    int p = execv(table[i].path_to_exe, arr);
                                    // fp1 = fopen("/home/sankalp/Desktop/NetProg_Assignment_3/child.txt", "a");
                                    // fprintf(fp1, "323 %d %d\n", p, errno);
                                    // fclose(fp1);
                                    exit(1);
                                }
                        }
                        else {
                            if(table[i].wait_no_wait == 1) {
                                table[i].pid = pid;
                                FD_CLR(table[i].sockfd, &masterset);
                            }
                        }
                    }
                }
            }
        }
        // sleep(2);
    }//end while
}
