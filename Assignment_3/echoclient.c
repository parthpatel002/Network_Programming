#include <stdio.h>
#include <sys/socket.h>  //for socket(), connect(), send(), recv() functions
#include <arpa/inet.h>  // different address structures are declared here
#include <stdlib.h>   // atoi() which convert  string to integer
#include <string.h>   
#include <unistd.h>  // close() function
#define BUFSIZE 32
int main()
{
 /* CREATE A TCP SOCKET*/
 int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
 if (sock < 0) { printf ("Error in opening a socket"); exit (0);}
 printf ("Client Socket Created\n");

/*CONSTRUCT SERVER ADDRESS STRUCTURE*/
 struct sockaddr_in serverAddr;
 memset (&serverAddr,0,sizeof(serverAddr)); 

/*memset() is used to fill a block of memory with a particular value*/
 serverAddr.sin_family = AF_INET;
 serverAddr.sin_port = htons(7); //You can change port number here
 serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //Specify server's IP address here
 printf ("Address assigned\n");
 
/*ESTABLISH CONNECTION*/
 int c = connect (sock, (struct sockaddr*) &serverAddr , sizeof (serverAddr));
 printf ("%d\n",c);
 if (c < 0) 
   { printf ("Error while establishing connection"); 
     exit (0);
   }
//  printf ("Connection Established\n");
 
/*SEND DATA*/
 printf ("ENTER REAL NUMBER FOR SERVER\n");
 char msg[BUFSIZE];
 fgets(msg,BUFSIZE,stdin);
 int bytesSent = send (sock, msg, strlen(msg), 0);
 if (bytesSent != strlen(msg)) 
   { printf("Error while sending the message"); 
     exit(0);
   }
 printf ("Data Sent\n");

 /*RECEIVE BYTES*/
 char recvBuffer[BUFSIZE];
 int bytesRecvd = recv (sock, recvBuffer, BUFSIZE-1, 0);
 if (bytesRecvd < 0) 
   { printf ("Error while receiving data from server"); 
     exit (0);
   }
 recvBuffer[bytesRecvd] = '\0';
 printf ("%s\n",recvBuffer);

close(sock);
}