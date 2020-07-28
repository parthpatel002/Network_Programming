#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>  
#include <string.h>   
#include <unistd.h>  
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>


void runClient(char* argv[], int M, int processNo) {
    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) { printf ("Error in opening a socket"); exit (0);}

    struct sockaddr_in serverAddr;
    memset (&serverAddr,0,sizeof(serverAddr)); 

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[1])); //You can change port number here
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //Specify server's IP address here
 
    int c = connect (sock, (struct sockaddr*) &serverAddr , sizeof (serverAddr));
    
    int pid = fork();
    if(pid == 0) {
        while(1) {
            // sleep(2);
            char reply[512]="";
            int bread;
            if((bread = recv(sock, reply, 512, 0)) == -1) {
                    printf("send()");
                    exit(1);
            }
                reply[bread] = '\0';
                printf("\nMessages received by Process %d is : %s\n", processNo, reply);
                double recvtime;
                recvtime = clock();
                FILE* fp = fopen("output.txt", "a");
                fprintf(fp, "%d %lf\n", processNo, recvtime);
                fclose(fp);
        }
    }

    // if(processNo == 1) {
    //     char msg[200]="";
    //     strcat(msg, "UMSG ");
    //     strcat(msg, "yyy");
    //     strcat(msg, "\r\n ");
    //     strcat(msg, "Hiyyy");
    //     strcat(msg, "\r\n");
    //     printf("message being sent is %s\n", msg);
    //     if(send(sock, msg, strlen(msg), 0) == -1 ) {
    //         printf("send()");
    //         exit(1);
    //     }
    //     sleep(0.1);
    // }


    if (c < 0) 
    { printf ("Error while establishing connection"); 
        exit (0);
    }
    printf ("Connection Established\n");
 
    char msg[200]="";
    strcat(msg, "JOIN ");
    char uname[20]="";
    sprintf(uname, "user%d", processNo);
    strcat(msg, uname);
    strcat(msg, "\r\n");
    printf("User name is %s\n", uname);
    if(send(sock, msg, strlen(msg), 0) == -1 ) {
        printf("send()");
        exit(1);
    }
    memset(msg, 0, 200);
    
    sleep(0.1);

    int msgSent = 0;

    double sendtime;

    while(msgSent < M) {
        memset(msg, 0, 200);
        // strcat(msg, "BMSG ");
        // strcat(msg, "Hi everyone");
        // strcat(msg, "\r\n");
        strcat(msg, "UMSG ");
        strcat(msg, uname);
        strcat(msg, "\r\n ");
        strcat(msg, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\n");
        if(send(sock, msg, strlen(msg), 0) == -1 ) {
            printf("send()");
            exit(1);
        }
        sendtime = clock();
        FILE* fp = fopen("output.txt", "a");
        fprintf(fp, "%d %lf\n", processNo, sendtime);
        fclose(fp);
        sleep(0.1);
        msgSent++;
    }

    // sleep(10);
    // memset(msg, 0, 200);
    // strcat(msg, "LEAV");
    // if(send(sock, msg, strlen(msg), 0) == -1 ) {
    //     printf("send()");
    //     exit(1);
    // }


    printf("Process %d exiting\n", processNo);
    exit(1);
}


int main(int argc, char* argv[]) {
    int N = atoi(argv[2]); // Number of concurrent connections to the server
    int M = atoi(argv[3]); // Number of messages within a connection
    int T = atoi(argv[4]); // Total number of connections to make

    int noOfProcessesCreated = 0;
    int isChild = 0;

    int i = 0;
    for(int i = 0; i < N; i++) {
        int pid = fork();
        if(pid > 0) {
            noOfProcessesCreated++;
            if(noOfProcessesCreated == N) {
                break;
            }
        }
        else {
            isChild = 1;
            break;
        }
    }

    int isChild2 = 0;

    if(isChild == 1) {
        runClient(argv, M, noOfProcessesCreated);
        //execute client program
    }
    else { // parent
        while(noOfProcessesCreated < T) {
            wait(NULL);
            int pid = fork();
            if(pid > 0) {
                noOfProcessesCreated++;
                if(noOfProcessesCreated == T) {
                    break;
                }
            }
            else {
                isChild2 = 1;
                break;
            }
        }

        if(isChild2 == 1) {
            runClient(argv, M, noOfProcessesCreated);
            //execute client program
        }
        else {
            // while(waitpid(-1, NULL, WNOHANG) != -1); //parent keeps waiting
            while(1) {
                wait(NULL);
            }
        }
    }
    return 0;
}
