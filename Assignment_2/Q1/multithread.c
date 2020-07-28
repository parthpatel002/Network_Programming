#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h> 
#include<sys/types.h>   //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> 
#include <netinet/in.h>
#include <netinet/tcp.h>

#define MAX_OUTSTANDING 20

typedef struct client{
    char name[50];
    int sockfd;
    pthread_t thread_id;
    struct client* next;
} Client;

typedef struct client_linked_list {
    Client* head;
    Client* tail;
    int size;
} client_list;

client_list* clients;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

void* serviceRequest(void* pointer);

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

    // if(clients->head == NULL) {
        // printf("Indeed NULL\n");
    // }
    // else if(clients->head != NULL) {
        // printf("not NULL\n");
    // }
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

void delete_client_list(client_list* clients, char* name) {
    if(clients->size == 0) return;
    
    if(clients->size == 1) {
        if(strcmp(clients->head->name, name) == 0) {
            free(clients->head);
            clients->head = NULL;
            clients->tail = NULL;
            clients->size--;
        }
        return;
    }

    if(strcmp(clients->head->name, name) == 0) {
            Client* temp = clients->head;
            clients->head = clients->head->next;
            clients->size--;
            free(temp);
            return;
    }


    Client* temp = clients->head;
    Client* prev = NULL;
    while(temp != NULL) {
        if(strcmp(temp->name, name) == 0) {
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
    // printf("Bound to port. \n");
    
    listen(listenfd , MAX_OUTSTANDING);
    
    // printf("Waiting for incoming connections...\n");
    
    pthread_t thread_id;

    while( (connfd = accept(listenfd, (struct sockaddr *)&clientAddr, &clientLen)) )
    {
        int i = 1;
        // setsockopt( connfd, IPPROTO_TCP, TCP_NODELAY, (void *)&i, sizeof(i));

        // printf("Connection accepted\n");
        int *pointer_to_connfd = (int*) malloc(sizeof(int));
        *pointer_to_connfd = connfd;
        if( pthread_create( &thread_id , NULL ,  serviceRequest , (void*) pointer_to_connfd) < 0)
        {
            perror("could not create thread\n");
            return 1;
        }
        
        printf("Inside main thread, handler assigned\n");
    }
    
    if (connfd < 0)
    {
        printf("Accept failed\n");
        return 1;
    }
    
    return 0;
}

void* serviceRequest(void* pointer) {
    int* int_pointer = (int*) pointer;
    int connfd = *int_pointer;
    pthread_detach(pthread_self());

    free(pointer);
    char* buf;
    // char buf[200];
    char name[20];
    char* tmp;
    int nread;
    int bytesread = 0;
    int readCompleteMessage = 1;
    int isJoined = 0;
    while(1) {
        if(readCompleteMessage) {
            buf = (char*)malloc(sizeof(char) * 5120);
            tmp = buf;
        }
        // memset(buf, 0, 200);
        sleep(0.5);
        if( readCompleteMessage && (nread = recv(connfd, buf, 200, 0)) == -1) {
            printf("recv()\n");
            exit(1);
        }

        switch(buf[0]) {
            case 'J' : {
                // printf("Here\n");
                if(isJoined == 1) {
                    printf("This client has already joined using username %s\n", name);
                    continue;
                }

                if(readCompleteMessage) {
                    bytesread = 0;
                }
                bytesread += 5;
                FILE* fp = fopen("throughput.txt", "a");
                fprintf(fp, "J %lf\n", (double)clock());
                fclose(fp);
                int i, j;
                // printf("Value of nread is %d\n", nread);
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
                
                Client* entry = allocate_client(name, connfd, pthread_self());
                pthread_mutex_lock(&mtx);
                add_to_client_list(clients, entry);
                isJoined = 1;
                pthread_mutex_unlock(&mtx);
                printf("%s joined\n", name);
                // if(send(connfd, "You have joined successfully#\n", 30, 0) == -1) {
                    // printf("send()");
                    // exit(1);
                // }
                // sleep(0.1);
                
            break;
            }
            case 'L' : {
                if(isJoined == 0) {
                    printf("Please join the server first using JOIN message\n");
                    continue;
                }
                if(buf[1] == 'I') {
                    char reply[300] = "";
                    strcat(reply, "Here is the list of users online\n");
                    pthread_mutex_lock(&mtx);
                    Client* temp = clients->head;
                    while(temp != NULL) {
                        strcat(reply, temp->name);
                        strcat(reply, "\n");
                        temp = temp->next;
                    }
                    pthread_mutex_unlock(&mtx);
                    if(send(connfd, reply, strlen(reply), 0) == -1) {
                        printf("send()");
                        exit(1);
                    }
                    
                }
                else if(buf[1] == 'E') {
                    pthread_mutex_lock(&mtx);
                    delete_client_list(clients, name);
                    pthread_mutex_unlock(&mtx);
                    // if(send(connfd, "You have been removed from the client list", 42, 0) == -1) {
                        // printf("send()");
                        // exit(1);
                    // }
                    // close(connfd);
                }
                else {
                    printf("Error in message\n");
                    exit(1);
                }
            break;
            }
            case 'U' : {
                char receiver[20];
                if(isJoined == 0) {
                    printf("Please join the server first using JOIN message\n");
                    continue;
                }
                if(readCompleteMessage) {
                    bytesread = 0;
                }
                // printf("Buf has %s\n", buf);
                char msg[100];
                int carriage_return_index = 0;
                
                while(buf[carriage_return_index] != '\r') { //find position of first \r
                    carriage_return_index++;
                    bytesread++;
                }
                int i, j;
                bytesread+=3;
                for(i = 5, j = 0; i <= carriage_return_index - 1; i++,j++) { // extract name from buf
                    receiver[j] = buf[i];
                }
                receiver[j] = '\0';
                // printf("Name in UMSG is %s\n", receiver);

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
                // printf("Message in UMSG is %s\n", msg);

                pthread_mutex_lock(&mtx);
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
                pthread_mutex_unlock(&mtx);
                FILE* fp = fopen("throughput.txt", "a");
                fprintf(fp, "U %lf\n", (double)clock());
                fclose(fp);
            break;
            }
            case 'B' : {
                if(isJoined == 0) {
                    printf("Please join the server first using JOIN message\n");
                    continue;
                }
                char msg[100];
                int i, j;
                for(i = 5, j = 0; i < nread - 2; i++, j++)
                    msg[j] = buf[i];
                msg[j] = '\0';

                pthread_mutex_lock(&mtx);
                Client* temp = clients->head;
                while(temp != NULL) {
                    int senderfd = temp->sockfd;
                    if(send(senderfd, msg, strlen(msg), 0) == -1) {
                        printf("send()");
                        exit(1);
                    }
                    // sleep(0.1);
                    temp = temp->next;
                }
                pthread_mutex_unlock(&mtx);
            break;
            }
        }
        if(readCompleteMessage) {
            free(tmp);
        }
    }
    //service remaining request
}