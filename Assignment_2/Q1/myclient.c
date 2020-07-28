#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>  
#include <string.h>   
#include <unistd.h>  

int main(int argc, char* argv[])
{
 
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
            char reply[200];
            int bread;
            if((bread = recv(sock, reply, 200, 0)) == -1) {
                    printf("send()");
                    exit(1);
            }
                reply[bread] = '\0';
                printf("%s\n", reply);
        }
    }
    if (c < 0) 
    { printf ("Error while establishing connection"); 
        exit (0);
    }
    printf ("Connection Established\n");
 
    while(1) {
        char msg[200] = "";
        printf("1. JOIN\n2. LIST\n3. UMSG\n4. BMSG\n5. LEAV\n");
        int choice;
        scanf("%d", &choice);
        switch(choice) {
            case 1: {
                strcat(msg, "JOIN ");
                printf("Enter your username\n");
                char uname[20];
                scanf("%s", uname);
                strcat(msg, uname);
                strcat(msg, "\r\n");
                printf("%s", msg);
                if(send(sock, msg, strlen(msg), 0) == -1 ) {
                    printf("send()");
                    exit(1);
                }
                char reply[200];
                memset(reply, 0, 200);
                // if(recv(sock, reply, 200, 0) == -1) {
                //     printf("send()");
                //     exit(1);
                // }
                // printf("%s\n", reply);
                break;
            }
            case 2: {
                strcat(msg, "LIST"); 
                if(send(sock, msg, strlen(msg), 0) == -1 ) {
                    printf("send()");
                    exit(1);
                }
                char reply[200];
                memset(reply, 0, 200);
                // if(recv(sock, reply, 200, 0) == -1) {
                //     printf("send()");
                //     exit(1);
                // }
                // printf("%s\n", reply);

                break;
            }
            case 3: {
                strcat(msg, "UMSG ");
                printf("Enter name of user to send message to\n");
                char uname[20];
                scanf("%s", uname);
                strcat(msg, uname);
                strcat(msg, "\r\n ");
                printf("Enter the message you want to send\n");
                char message[50];
                scanf("%s", message);
                strcat(msg, message);
                strcat(msg, "\r\n");
                // printf("message being sent is %s\n", msg);
                if(send(sock, msg, strlen(msg), 0) == -1 ) {
                    printf("send()");
                    exit(1);
                }
                // char reply[200];
                // memset(reply, 0, 200);
                // if(recv(sock, reply, 200, 0) == -1) {
                //     printf("send()");
                //     exit(1);
                // }
                // printf("%s\n", reply);

                break;
            }
            case 4: {
                strcat(msg, "BMSG ");
                printf("Enter the message\n");
                char message[50];
                scanf("%s", message);
                strcat(msg, message);
                strcat(msg, "\r\n");
                if(send(sock, msg, strlen(msg), 0) == -1 ) {
                    printf("send()");
                    exit(1);
                }
                // char reply[200];
                // memset(reply, 0, 200);
                // if(recv(sock, reply, 200, 0) == -1) {
                //     printf("send()");
                //     exit(1);
                // }
                // printf("%s\n", reply);
                break;
            }
            case 5: {
                strcat(msg, "LEAV");
                if(send(sock, msg, strlen(msg), 0) == -1 ) {
                    printf("send()");
                    exit(1);
                }
                // char reply[200];
                // memset(reply, 0, 200);
                // if(recv(sock, reply, 200, 0) == -1) {
                //     printf("send()");
                //     exit(1);
                // }
                // printf("%s\n", reply);
                break;
            }
        }

    }
close(sock);
}