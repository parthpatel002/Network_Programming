#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> 
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include<sys/epoll.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/select.h>
#include<math.h>

#define TIMEOUT 5
#define MAXRETRANSMITS 3
#define RRQOPCODE 1
#define DATAOPCODE 3
#define ACKOPCODE 4
#define ERROROPCODE 5

#define INITIALRTT 1
#define INITIALDEVIATION 0.1
#define ALPHA 0.125
#define BETA 0.25

typedef struct {
    unsigned short int OPCODE;
    unsigned short int BLOCKNO;
    char data[512];
} DATA;

typedef struct {
    unsigned short int OPCODE;
    unsigned short int BLOCKNO_OR_ERRORCODE;
    char errmsg[48];
} ACKORERROR;

void die(char *s)
{
    perror(s);
    exit(1);
}

typedef struct mysocket {
    int sockfd;
    clock_t sentAt;
    FILE* fp;
    struct mysocket* next;
    unsigned short int blocknumber;
    DATA datapkt;
    struct sockaddr_in si_other;
    int no_of_retransmits;
    double estimatedRTT;
    double estimatedDeviation;
    double estimatedTimeout;
} mySocket;

typedef struct queue {
    mySocket* head;
    mySocket* tail;
    int size;
} Queue;

typedef struct RRQ{
    unsigned short int OPCODE;
    char fileandmode[512];
} RRQ;

Queue* initialiseQueue() {
    Queue* q = (Queue*) malloc(sizeof(Queue));
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    return q;
}


mySocket* initialiseSocket(int sockfd, clock_t sentAt, FILE* fp) {
    mySocket* m = (mySocket*) malloc(sizeof(mySocket));
    m->sockfd = sockfd;
    m->sentAt = sentAt;
    m->next = NULL;
    m->fp = fp;
    m->blocknumber = 1;
    m->no_of_retransmits = 0;
    m->estimatedDeviation = INITIALDEVIATION;
    m->estimatedRTT = INITIALRTT;
    m->estimatedTimeout = INITIALRTT + 4 * INITIALDEVIATION;
    return m;
}

Queue* q;

void addToBack(mySocket* m) {
    if(q->size == 0) {
        q->head = m;
        q->tail = m;
        q->size++;
    }
    else {
        q->tail->next = m;
        q->tail = m;
        q->size++;
    }
}

void delete(int sockfd) {
	mySocket *a = q->head;
	mySocket *b = NULL;
	while(a != NULL){
		if (a->sockfd == sockfd){
			if (a == q->head){
				q->head = a->next;
				// free(a);
				a = q->head;
				q->size--;
			}
			else{
				b->next = a->next;
				// free(a);
				a = b->next;
				q->size--;
			}
		}
		else {
			b = a;
			a = a->next;
		}
	}
	q->tail = b;
}

mySocket* find(int sockfd) {
    mySocket* tmp = q->head;
    while(tmp != NULL) {
        if(tmp->sockfd == sockfd) {
            return tmp;
        }
        tmp = tmp->next;
    }
    return NULL;
}

void addToCliArr(int sockfd, int* arr) {
    for(int i = 0; i < 1024; i++) {
        if(arr[i] == -1) {
            arr[i] = sockfd;
            return;
        }
    }
}

void deleteFromCliArr(int sockfd, int* arr) {
    for(int i = 0; i < 1024; i++) {
        if(arr[i] == sockfd) {
            arr[i] = -1;
            return;
        }
    }
}

mySocket* findMinTimeout() {
    double minTimeout = INT64_MAX;
    mySocket* tmp = q->head;
    mySocket* minNode = NULL;
    while(tmp != NULL) {
        if(tmp->estimatedTimeout < minTimeout) {
            minNode = tmp;
            minTimeout = tmp->estimatedTimeout;
        }
        tmp = tmp->next;
    }
    return minNode;
}

int main(int argc, char* argv[]) {
    // printf("%ld\n", sizeof(ACKORERROR));
    q = initialiseQueue();
    int cliArr[1024];
    for(int i = 0; i < 1024; i++)
        cliArr[i] = -1; // client array to keep track of which sockets to check for in select
    struct sockaddr_in si_me, si_other;
    int listenfd, i, socket_len = sizeof(si_other) , recv_len;
    int nread; // number of bytes read
    int newskt; // for communicating to client via new port
    FILE* fp; // pointer to file to be sent
    mySocket* m = (mySocket*) malloc(sizeof(mySocket));

    // char buf[BUFLEN];
     
    if ((listenfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
     
    memset((char *) &si_me, 0, sizeof(si_me));
    memset((char *) &si_other, 0, sizeof(si_other));

     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(atoi(argv[1]));
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    if( bind(listenfd , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }

    int max_sd; // to keep track of first argument of select call

    fd_set master_set, working_set; // master set is used as working set gets modified by select()
	struct timeval timeout;

    DATA* datapkt = (DATA*) malloc(sizeof(DATA));
    ACKORERROR* ackpkt = (ACKORERROR*) malloc(sizeof(ACKORERROR));
    RRQ* rrqpkt = (RRQ*) malloc(sizeof(RRQ));
    char* mode;


    FD_ZERO(&master_set);
   	max_sd = listenfd;
   	FD_SET(listenfd, &master_set);
    
    timeout.tv_sec  = 5;
   	timeout.tv_usec = 0;

    char readFromFile[512]; // to read from file

    while(1) {
        memset(readFromFile, 0, 512);
         // to clear out filedata for every read
        memcpy(&working_set, &master_set, sizeof(master_set));
        int rc;
        if(q->size == 0) {
        
            rc = select(max_sd + 1, &working_set, NULL, NULL, NULL); // NULL for timeout means wait infinitely

            if(rc < 0)
                die("select()");
            else if(rc > 0) {
                if(FD_ISSET(listenfd, &working_set)) {
                    if(recvfrom(listenfd, rrqpkt, sizeof(RRQ), 0, (struct sockaddr*)&si_other, &socket_len) == -1 ) {
                        die("recvfrom()");
                    }
                    else {
                        printf("\nReceived RRQ, Client %s %d connected, asking for file %s\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), rrqpkt->fileandmode);
                        void* ptr = rrqpkt; 
                        mode = (char*)(ptr + 2 + strlen(rrqpkt->fileandmode) + 1); // refer to RFC
                        if ((newskt=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
                        {
                            die("socket");
                        }
                        addToCliArr(newskt, cliArr);
                        if(newskt > max_sd) {
                            max_sd = newskt;
                        }
                        
                        fp = fopen(rrqpkt->fileandmode, "r");
                        if(fp == NULL) {
                            ACKORERROR* errpkt = (ACKORERROR*) malloc(sizeof(ACKORERROR));
                            errpkt->BLOCKNO_OR_ERRORCODE = htons(1);
                            errpkt->OPCODE = htons(ERROROPCODE);
                            strcpy(errpkt->errmsg,"File not found.");
                            if(sendto(newskt, errpkt,4 + strlen(errpkt->errmsg) + 1, 0, (struct sockaddr*)&si_other, sizeof(si_other)) == -1) {
                                die("sendto()");
                            }
                            deleteFromCliArr(newskt, cliArr);
                            printf("\nSent ERROR to %s %d, file not found\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

                            continue;
                        }
                        
                        m = initialiseSocket(newskt, clock(), fp);
                        nread = fread(readFromFile, 1, 512, fp);
                        if(nread == -1) {
                            die("nread");
                        }

                        // datapkt->OPCODE = htons(DATAOPCODE);
                        datapkt->OPCODE = htons(DATAOPCODE);
                        datapkt->BLOCKNO = htons(1);
                        // char* ch_p = datapkt;
                        strcpy(datapkt->data, readFromFile);

                        int sentsize=0;
                        if( (sendto(newskt, datapkt, 4 + strlen(datapkt->data), 0, (struct sockaddr*)&si_other, sizeof(si_other))) == -1) {
                            die("sendto()");
                        }
        
                        printf("\nSent DATA to %s %d, %d bytes with block no %d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), nread, ntohs(datapkt->BLOCKNO));
                        // void* ptr1 = (void*)datapkt;
                        // for(int k = 0; k < 516; k++) {
                        //     putchar( *(char*) (ptr1 + k) + 48);
                        // }
                        if(nread < 512 && nread >= 0) {
                            close(newskt);
                            deleteFromCliArr(newskt, cliArr);
                        }
                        else {
                            m->datapkt = *datapkt;
                            m->si_other = si_other;
                            addToBack(m);
                            FD_SET(newskt, &master_set);
                        }
                    }
                }
            } 
        }
        else {
            // timeout.tv_sec = TIMEOUT - (clock() - q->head->sentAt) / CLOCKS_PER_SEC;
            // printf("Here!\n");
            // printf("%ld\n", timeout.tv_sec);
            // printf("%ld\n", q->head->sentAt);
            mySocket* tmp1 = findMinTimeout();

            timeout.tv_sec = tmp1->estimatedTimeout;
            timeout.tv_usec = 0;   
            printf("Came inside here\n");
            memcpy(&working_set, &master_set, sizeof(master_set));
        
            rc = select(max_sd + 1, &working_set, NULL, NULL, &timeout);

            if(rc < 0)
                die("select()");
            else if (rc == 0) {
                if(tmp1->no_of_retransmits < MAXRETRANSMITS) { 
                    if(sendto(tmp1->sockfd, &(tmp1->datapkt), 4 + strlen(tmp1->datapkt.data) + 1, 0, (struct sockaddr*)&(tmp1->si_other), sizeof(tmp1->si_other)) == -1 ) {
                        die("sendto()");
                    }
                    printf("\nRETRANSMISSION %d: Sent DATA to %s %d, %d bytes with block no %d\n", tmp1->no_of_retransmits+1,inet_ntoa(tmp1->si_other.sin_addr), ntohs(tmp1->si_other.sin_port), (int)strlen(tmp1->datapkt.data), ntohs(tmp1->datapkt.BLOCKNO));
                    // mySocket* m1 = (mySocket*) malloc(sizeof(mySocket));
                    // m->blocknumber = q->head->blocknumber;
                    // m->datapkt = q->head->datapkt;
                    // m->fp = q->head->fp;
                    // q->head->next = NULL;
                    tmp1->sentAt = clock();
                    tmp1->estimatedTimeout *= 2;
                    printf("\nEstimated RTO : %lf Estimated RTT : %lf\n", tmp1->estimatedTimeout, tmp1->estimatedRTT);
                    // q->head->si_other = q->head->si_other;
                    // m->sockfd = q->head->sockfd;

                    // mySocket* temp = q->head;
                    tmp1->no_of_retransmits++;
                    // delete(tmp1->sockfd);
                    
                    // addToBack(tmp1);
                }
                else {
                    printf("\nExhausted the number of retransmissions to %s %d.\n", inet_ntoa(tmp1->si_other.sin_addr), ntohs(tmp1->si_other.sin_port));
                    FD_CLR(tmp1->sockfd, &master_set);
                    deleteFromCliArr(tmp1->sockfd, cliArr);
                    close(tmp1->sockfd);
                    delete(tmp1->sockfd);
                    
                }
            }
            else if(rc > 0) {
                if(FD_ISSET(listenfd, &working_set)) {

                    if(recvfrom(listenfd, rrqpkt, sizeof(RRQ), 0, (struct sockaddr*)&si_other, &socket_len) == -1 ) {
                        die("recvfrom()");
                    }
                    else {
                        printf("\nReceived RRQ, Client %s %d connected, asking for file %s\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), rrqpkt->fileandmode);
                        void* ptr = rrqpkt;
                        mode = (char*)(ptr + 2 + strlen(rrqpkt->fileandmode) + 1);

                        // int newskt;
                        if ((newskt=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
                        {
                            die("socket");
                        }

                        addToCliArr(newskt, cliArr);

                        if(newskt>max_sd) {
                            max_sd = newskt;
                        }

                        fp = fopen(rrqpkt->fileandmode, "r");
                        if(fp == NULL) {
                            ACKORERROR* errpkt = (ACKORERROR*) malloc(sizeof(ACKORERROR));
                            errpkt->BLOCKNO_OR_ERRORCODE = htons(1);
                            errpkt->OPCODE = htons(ERROROPCODE);
                            strcpy(errpkt->errmsg,"File not found.");
                            if(sendto(newskt, errpkt,4 + strlen(errpkt->errmsg), 0, (struct sockaddr*)&si_other, sizeof(si_other)) == -1) {
                                die("sendto()");
                            }
                            deleteFromCliArr(newskt, cliArr);
                            printf("\nSent ERROR to %s %d, file not found\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
                            continue;
                        }
                        
                        m = initialiseSocket(newskt, clock(), fp);
                        nread = fread(readFromFile, 1, 512, fp);
                        if(nread == -1) {
                            die("nread");
                        }
                        datapkt->OPCODE = htons(DATAOPCODE);
                        datapkt->BLOCKNO = htons(1);
                        strcpy( datapkt->data , readFromFile);
                        if(sendto(newskt, datapkt,4 + strlen(datapkt->data), 0, (struct sockaddr*)&si_other, sizeof(si_other)) == -1) {
                            die("sendto()");
                        }
                        printf("\nSent DATA to %s %d, %d bytes with block no %d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), nread, ntohs(datapkt->BLOCKNO));
                        if(nread < 512 && nread >= 0) {
                            close(newskt);
                            deleteFromCliArr(newskt, cliArr);
                        }
                        else {
                            m->datapkt = *datapkt;
                            m->si_other = si_other;
                            m->next = NULL;
                            addToBack(m);
                            FD_SET(newskt, &master_set);
                        }
                    }
                }
                for(int j = 0; j < 1024; j++) {
                    if(cliArr[j] < 0) {
                        continue;
                    }
                    if(FD_ISSET(cliArr[j], &working_set)) {
                        int bytesrecvd = 0;
                        if( recvfrom(cliArr[j], ackpkt, sizeof(ACKORERROR), 0, (struct sockaddr*)&si_other, &socket_len) == -1 ) {
                            die("recvfrom()");
                        }
                        else {
                            if(ntohs(ackpkt->OPCODE) == ACKOPCODE) {
                                printf("\nReceived ACK from %s %d with block number %d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), ntohs(ackpkt->BLOCKNO_OR_ERRORCODE));
                                mySocket* m1;
                                m1 = find(cliArr[j]);
                                if(m1 == NULL) {
                                    die("Entry is NULL");
                                }
                                if(ntohs(ackpkt->BLOCKNO_OR_ERRORCODE) == m1->blocknumber) {
                                    // delete(cliArr[j]);
                                    // char readFromFile[512];
                                    if(m1->no_of_retransmits == 0) {
                                        double sampleRTT = (clock() - m1->sentAt) / CLOCKS_PER_SEC;
                                        m1->estimatedDeviation = (1-BETA) * m1->estimatedDeviation + BETA * abs(sampleRTT - m1->estimatedRTT);
                                        m1->estimatedRTT = (1-ALPHA) * m1->estimatedRTT + ALPHA * sampleRTT;
                                        m1->estimatedTimeout = m1->estimatedDeviation * 4 + m1->estimatedRTT;
                                    }
                                    printf("\nEstimated RTO : %lf Estimated RTT : %lf\n", m1->estimatedTimeout, m1->estimatedRTT);

                                    nread = fread(readFromFile, 1, 512, m1->fp);
                                    if(nread == -1) {
                                        die("nread");
                                    }
                                    
                                    datapkt->OPCODE = htons(DATAOPCODE);
                                    // printf(" blocknumber is %d\n", m1->blocknumber);
                                    datapkt->BLOCKNO = htons(m1->blocknumber+1);
                                    strcpy( datapkt->data , readFromFile);
                                    
                                    // int sentsize = 0;
                                    if( sendto(cliArr[j], datapkt, 4 + strlen(datapkt->data), 0, (struct sockaddr*)&si_other, sizeof(si_other)) == -1) {
                                        die("sendto()");
                                    }
                                    printf("\nSent DATA to %s %d, %d bytes with block no %d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), nread, ntohs(datapkt->BLOCKNO ));
                                    if(nread < 512 && nread >= 0) {
                                        delete(cliArr[j]);
                                        FD_CLR(cliArr[j], &master_set);
                                        close(cliArr[j]);
                                        deleteFromCliArr(cliArr[j], cliArr);
                                    }
                                    else {
                                        m1->sentAt = clock();
                                        m1->blocknumber++;
                                        m1->datapkt = *datapkt;
                                        m1->no_of_retransmits = 0;
                                        // m1->next = NULL;
                                        // m->si_other = si_other;
                                        // addToBack(m1);
                                    }
                                }
                            }
                            else if (ntohs(ackpkt->OPCODE) == ERROROPCODE) {
                                printf("\nReceived ERROR from %s %d with error code %d\nERROR message %s\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), ntohs(ackpkt->BLOCKNO_OR_ERRORCODE), ackpkt->errmsg);
                            }
                        }
                    }
                }
            } 

        }
        
        
        
        
        
    } // end of while

} // end of main
