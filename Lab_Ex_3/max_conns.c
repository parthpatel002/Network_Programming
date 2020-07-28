#include<stdio.h>    
#include<stdlib.h>
#include<string.h>
#include<unistd.h>    
#include<sys/types.h>
#include <sys/wait.h>
#include<sys/socket.h>
#include<arpa/inet.h> 
#include<netdb.h>
#include<signal.h>
#include<errno.h>
#include<fcntl.h>
#define MAX_PER_PROCESS 800

int main(int argc, char* argv[]) {

    if(argc != 2) {
        perror("Invalid input");
        exit(1);
    }

    char * url= (char*)malloc(sizeof(char)*192);
    char* temporary;
    char* remaining_url;
    int current_cnt = 0;
    int num_processes = 0;
    int s, x, conn;
    pid_t pid;
    struct addrinfo h;
    struct addrinfo *r, *t;
    bzero(&h, sizeof(h));
    h.ai_family = AF_UNSPEC;
    h.ai_socktype = SOCK_STREAM;
    struct sockaddr_in * si;

    strcpy(url, argv[1]);
    remaining_url = url;    
    temporary = strtok_r(remaining_url, ":", &remaining_url);
    if((strcmp("https", temporary) != 0) && (strcmp("http", temporary) != 0)) 
        remaining_url = url;
    else{
        //remaining_url = remaining_url + 2;
        temporary = strtok_r(remaining_url, "/\n\0", &remaining_url);
        remaining_url = temporary;
    }
    printf("%s\n", remaining_url);
    if ((x = getaddrinfo(remaining_url, "http", &h, &r)) != 0) {
        perror("getaddrinfo()");
        return 1;
    }

    printf("List of IP addresses found for given URL: \n");
    t = r;    
    while(t != NULL) {
        si = (struct sockaddr_in*)(t->ai_addr);
        printf("IP Address: %s, Port Number: %d\n", inet_ntoa(si->sin_addr), ntohs(si->sin_port));
        t = t->ai_next;
    } 

    t = r;
    while(t != NULL) {
        si = (struct sockaddr_in*)(t->ai_addr);
        if(strcmp(inet_ntoa(si->sin_addr), "0.0.0.0") != 0){
            break;
        }
        else
            t = t->ai_next;
    }
    si = (struct sockaddr_in*)(t->ai_addr);
    printf("Using IP Address: %s, Port Number: %d\n", inet_ntoa(si->sin_addr), ntohs(si->sin_port));

    while(1) {
        if(current_cnt == MAX_PER_PROCESS) {
            num_processes = num_processes + 1;

            if(fork() > 0){
                wait(NULL);
                return 0;   
            }
            else{
                for(int fd = 3; fd < current_cnt + 3; fd++) 
                    close(fd);
                current_cnt = 0;
            }
        }

        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0) { 
            perror("socket():All available file descriptors for the process exhausted");
            for(int fd = 3; fd < current_cnt + 3; fd++) {
                close(fd);
            }
            return 0;
        }
  
        conn = connect(s, t->ai_addr, sizeof(struct sockaddr_in));         
        if(conn != 0) {
            printf("Maximum number of concurrent TCP connections supported by the given url is %d\n", num_processes * MAX_PER_PROCESS + current_cnt);
            perror("connect()");
            break;
        }
        else {
            current_cnt++;
            printf("Successfully established %d concurrent TCP connections to the given url.\n", num_processes * MAX_PER_PROCESS + current_cnt);
        }
    }

    return 0;
}