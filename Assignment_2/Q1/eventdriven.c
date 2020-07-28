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

#define MAX_OUTSTANDING 40
#define EXPECTED_CLIENTS 10
#define MAX_EVENTS 100

typedef struct client{
    char name[50];
    int sockfd;
    pthread_t thread_id; // Never used
    struct client* next;
} Client;

typedef struct client_linked_list {
    Client* head;
    Client* tail;
    int size;
} client_list;

typedef struct mesg_buffer { 
    long mesg_type; 
    // char request[200]; 
    // int received_at_fd;
    struct epoll_event ev;
} message; 

client_list* clients;
int msgid, epfd;

void* serviceRequest(void* pointer);

message* createMessage (struct epoll_event ev) {
    message* m = (message*) malloc(sizeof(message));
    m->mesg_type = 1;
    m->ev = ev;
    return m;
}

client_list* initialise() {
    client_list* c = (client_list*) malloc(sizeof(client_list));
    c->head = NULL;
    c->tail = NULL;
    c->size = 0;
    return c;
}

Client* allocate_client(char* name, int sockfd, pthread_t thread_id) {
    Client* c = (Client*) malloc(sizeof(Client));
    strcpy(c->name, name);
    c->sockfd = sockfd;
    c->thread_id = thread_id;
    c->next = NULL;
    return c;
}

void add_to_client_list(client_list* clients, Client* client) {

    if(clients->head == NULL) {
        printf("Indeed NULL\n");
    }
    else if(clients->head != NULL) {
        printf("not NULL\n");
    }
    Client* temp = clients->head;
    while(temp != NULL) {
        if(strcmp(temp->name, client->name) == 0) {
            printf("Such a client name already exists\n");
            printf("%s %s\n", temp->name, client->name);
            return;
        }
        temp = temp->next;
    }

    if(clients->size == 0) {
        clients->head = client;
        clients->tail = client;
    }
    else {
        clients->tail->next = client;
        clients->tail = client;
    }
    clients->size++;
    return;
}

void delete_client_list(client_list* clients, int sockfd) {
    if(clients->size == 0) return;
    
    if(clients->size == 1) {
        if(clients->head->sockfd == sockfd) {
            free(clients->head);
            clients->head = NULL;
            clients->tail = NULL;
            clients->size--;
        }
        return;
    }

    if(clients->head->sockfd == sockfd) {
            Client* temp = clients->head;
            clients->head = clients->head->next;
            clients->size--;
            free(temp);
            return;
    }


    Client* temp = clients->head;
    Client* prev = NULL;
    while(temp != NULL) {
        if(temp->sockfd == sockfd) {
            if(temp->next == NULL) { // element is at end of linked list
                clients->tail = prev;
            }
            prev->next = temp->next;
            clients->size--;
            free(temp);
            return;
        }
        prev = temp;
        temp = temp->next;
    }
    return;
}

int main(int argc, char* argv[]) {
    int connfd , listenfd, clientLen;

    clients = initialise();
    key_t key;
    key = ftok("path.txt", atoi(argv[2]));
    msgid = msgget(key, 0666 | IPC_CREAT);  

    struct sockaddr_in serverAddr , clientAddr;
    
    clientLen = sizeof(clientAddr);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        printf("Could not create socket\n");
    }
    printf("Socket created\n");
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons( atoi(argv[1]) );
    

    if( bind(listenfd,(struct sockaddr *)&serverAddr , sizeof(serverAddr)) < 0) {
        printf("bind() to port failed\n");
        return 1;
    }
    printf("Bound to port. \n");
    
    listen(listenfd , MAX_OUTSTANDING);
    
    printf("Waiting for incoming connections...\n");
    
    epfd = epoll_create(EXPECTED_CLIENTS);
    struct epoll_event ev;
    ev.data.fd = listenfd;
    ev.events = EPOLLIN | EPOLLET;

    if(epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
        printf("Error in epoll_ctl()\n");
        exit(1);
    }

    pthread_t thread_id;
    
    int *pointer_to_listenfd = (int*) malloc(sizeof(int));
    *pointer_to_listenfd = listenfd;

    if( pthread_create( &thread_id , NULL ,  serviceRequest , (void*)pointer_to_listenfd) < 0) {
            perror("could not create thread\n");
            return 1;
    }

    struct epoll_event evlist[MAX_EVENTS];

    while(1) {
        printf("Now waiting on epoll_wait\n");
        int ready = epoll_wait(epfd, evlist, MAX_EVENTS, -1);
        // printf("196\n");
        if (ready == -1) {
            if (errno == EINTR)
            continue;
            /* Restart if interrupted by signal */
            else {
                printf("epoll_wait\n");
                exit(1);
            }        
        }

        int j;
        // printf("208\n");
        printf("Now adding to message queue\n");
        for(j = 0; j < ready; j++) {
            message* m = createMessage(evlist[j]);
            msgsnd(msgid, m, sizeof(message), 0);
        }
        // printf("213\n");
    }
    return 0;
}

void* serviceRequest(void* pointer) {
    int* int_pointer = (int*) pointer;
    int listenfd = *int_pointer;
    pthread_detach(pthread_self());

    struct sockaddr_in cliAddr;
    int clilen = sizeof(cliAddr);

    // char buf[200];
    char name[20];
    char * buf;
    char * temp1;
    int bytesread = 0;
    int readCompleteMessage = 1;
    int nread;
    // int isJoined = 0;
    
    while(1) {
        message msg;
        // printf("233\n");
        printf("Now gonna read from msgq\n");
        if( readCompleteMessage && (msgrcv(msgid, &msg, sizeof(msg), 1, 0) == -1 ) ) {
            printf("Error in msgrcv\n");
            exit(1);
        }
        // printf("238\n");
        struct epoll_event ev = msg.ev;
        if(ev.events & EPOLLIN) {
            if(ev.data.fd == listenfd) {
                int connfd = accept(listenfd, (struct sockaddr*) &cliAddr, &clilen);
                printf("Accepted the connection\n");
                fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFL, 0) | O_NONBLOCK);
                struct epoll_event EV;
                EV.data.fd = connfd;
                EV.events = EPOLLIN | EPOLLET;
                if(epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &EV) == -1) {
                    printf("Error in epoll_ctl()\n");
                    exit(1);
                }
            }
            else {
                if(readCompleteMessage) {
                    buf = (char*)malloc(sizeof(char) * 5120);
                    temp1 = buf;
                }
                //memset(buf, 0, 200);
                if(readCompleteMessage && ( (nread = recv(ev.data.fd, buf, 200, 0)) == -1) ) {
                    perror("recv()");
                    exit(1);
                }
                if(nread == 0) {
                    close(ev.data.fd);
                }
                else {
                    switch(buf[0]) {
                        case 'J' : {
                            if(readCompleteMessage) {
                                bytesread = 0;
                            }
                            bytesread += 5;
                            int i, j;
                            printf("Value of nread is %d\n", nread);
                            for(i = 5, j = 0; i < nread - 2; i++, j++) {
                                if(buf[i] == '\r') {
                                    readCompleteMessage = 0;
                                    buf = buf + i + 2;
                                    break;
                                }
                                name[j] = buf[i];
                                bytesread++;
                            }
                            name[j] = '\0';
                            bytesread += 2;
                            printf("Name read is %s\n", name);
                            if(bytesread == nread) {
                                readCompleteMessage = 1;
                            }
                            Client* entry = allocate_client(name, ev.data.fd, pthread_self());
                            add_to_client_list(clients, entry);
                            printf("%s joined\n", name);
                            
                            // if(send(ev.data.fd, "You have joined successfully", 28, 0) == -1) {
                                // printf("send()");
                                // exit(1);
                            // }
                            
                        break;
                        }
                        case 'L' : {
                            if(buf[1] == 'I') {
                                char reply[300] = "";
                                strcat(reply, "Here is the list of users online\n");
                                Client* temp = clients->head;
                                while(temp != NULL) {
                                    strcat(reply, temp->name);
                                    strcat(reply, "\n");
                                    temp = temp->next;
                                }
                                if(send(ev.data.fd, reply, strlen(reply), 0) == -1) {
                                    printf("send()");
                                    exit(1);
                                } 
                            }
                            else if(buf[1] == 'E') {
                                delete_client_list(clients, ev.data.fd);
                                // if(send(ev.data.fd, "You have been removed from the client list", 42, 0) == -1) {
                                    // printf("send()");
                                    // exit(1);
                                // }
                                // close(ev.data.fd);
                            }
                            else {
                                printf("Error in message\n");
                                exit(1);
                            }
                        break;
                        }
                        case 'U' : {
                            char receiver[20];
                            printf("Buf has %s\n", buf);
                            char msg[100];
                            int carriage_return_index = 0;
                            
                            if(readCompleteMessage) {
                                bytesread = 0;
                            }

                            while(buf[carriage_return_index] != '\r') { //find position of first \r
                                carriage_return_index++;
                                bytesread++;
                            }
                            int i, j;
                            bytesread += 3;
                            for(i = 5, j = 0; i <= carriage_return_index - 1; i++,j++) { // extract name from buf
                                receiver[j] = buf[i];
                            }
                            receiver[j] = '\0';

                            printf("Name in UMSG is %s\n", receiver);

                            for(i = carriage_return_index + 3, j =0; i < nread - 2; i++, j++) { // extract message data from buf
                                if(buf[i] == '\r') {
                                    readCompleteMessage = 0;
                                    buf = buf + i + 2;
                                    break;
                                }
                                msg[j] = buf[i];
                                bytesread++;
                            }
                            msg[j] = '\0';
                            bytesread += 2;
                            if(bytesread == nread) {
                                readCompleteMessage = 1;
                            }
                            printf("Message in UMSG is %s\n", msg);

                            Client* temp = clients->head;
                            while(temp != NULL) {
                                if(strcmp(temp->name, receiver) == 0){
                                    int senderfd = temp->sockfd;
                                    if(send(senderfd, msg, strlen(msg), 0) == -1) {
                                        printf("send()");
                                        exit(1);
                                    }
                                    printf("Sent\n");
                                    break;
                                }
                                temp = temp->next;
                            }
                            

                        break;
                        }
                        case 'B' : {
                            char msg[100];
                            int i, j;
                            for(i = 5, j = 0; i < nread - 2; i++, j++)
                                msg[j] = buf[i];
                            msg[j] = '\0';

                            Client* temp = clients->head;
                            while(temp != NULL) {
                                int senderfd = temp->sockfd;
                                if(send(senderfd, msg, strlen(msg), 0) == -1) {
                                    printf("send()");
                                    exit(1);
                                }
                                temp = temp->next;
                            }
                            break;
                        }

                    }
                }
                if(readCompleteMessage) {
                    free(temp1);
                }
            }
        }
    }
}
            
        
    
    //service remaining request
