#include<stdio.h>
#include <sys/types.h>
#include <sys/msg.h>
#include<stdlib.h>
#include <sys/ipc.h> 
#include<string.h>
#include<fcntl.h>
#include<unistd.h>

#define CREATE 0
#define LIST 1
#define JOIN 2
#define SEND 3
#define REGISTER 4
#define CONNECT 5
#define EXIT 6

#define MAXSIZE 1024

typedef struct {
    long mtype; // 1 is read by server, 2 is read by client for acknowledgment, 3 (notification), beyond are group ids to be read by client.
    char mbody[100]; // send message
    int groupid; // group id
    int userid;
    int option;
    key_t key;
} message;

message* message_populate(long mtype, char* mbody, int groupid, int userid, int option, key_t key) {
    message* m = (message*) malloc(sizeof(message));
    m->mtype = mtype;
    if(mbody!= NULL){
        strcpy(m->mbody, mbody);
    }
    m->groupid = groupid;
    m->userid = userid;
    m->option = option;
    m->key = key;
    return m;
}


int main() {
        key_t key = ftok("/home/sankalp/Desktop/NetProg_Assignment1/question2/file1", getpid()); 
        int msgid = msgget(key, 0666 | IPC_CREAT);

        int USERID;
        int isRegistered = 0;
        int pid = fork();
        if(pid == 0) {
            while(1) {
                message* initial = (message*)malloc(sizeof(message));

                msgrcv(msgid, initial, MAXSIZE, 3, 0);

                printf("Received a message from group id %d\n", initial->groupid);
            }
        }
        else {
            while(1) {
                printf("Menu\n");
                printf("0. CREATE\n1. LIST\n2. JOIN\n3. SEND\n4. REGISTER\n5. CONNECT\n6. EXIT\n");
                int option;
                
                scanf("%d", &option);
                message* initial;
                int groupid;
                switch(option) {
                    case 0:
                        printf("Enter group name\n");
                        char groupname[256];
                        scanf(" %[^\n]", groupname);

                        initial = message_populate(1l, groupname, -1, USERID, option, key);
                        
                        if (msgsnd(msgid, initial, sizeof(message) - sizeof(long), 0) == -1)
                            perror("Error in sending");
                        
                        printf("Create group request sent\n");
                        
                        msgrcv(msgid, initial, MAXSIZE, 2, 0);
                        printf("Group id of created group named %s is %d\n", groupname, initial->groupid);
                        break;
                    case 1:
                        
                        
                        initial = message_populate(1l, NULL, -1, USERID, option, key);
                        
                        if (msgsnd(msgid, initial, sizeof(message) - sizeof(long), 0) == -1)
                            perror("Error in sending");
                        
                        printf("List groups request sent\n");
                        
                        msgrcv(msgid, initial, MAXSIZE, 2, 0);
                        printf("Here is a list of groups:\n%s\n", initial->mbody);
                        break;
                    case 2:
                        
                        printf("Enter group id\n");
                        scanf("%d", &groupid);

                        
                        
                        initial = message_populate(1l, NULL, groupid, USERID, option, key);
                        
                        if (msgsnd(msgid, initial, sizeof(message) - sizeof(long), 0) == -1)
                            perror("Error in sending");
                        
                        printf("Join group request sent\n");
                        
                        msgrcv(msgid, initial, MAXSIZE, 2, 0);

                        if(initial->groupid == -1) {
                            printf("You are already in the group %d\n", groupid);
                        }
                        else {
                            printf("Successfully joined the group %d\n", initial->groupid);
                        }
                        break;
                    case 3:
                        
                        printf("Enter group id\n");
                        scanf("%d", &groupid);

                        printf("Enter message to be sent\n");
                        char messagebody[256];
                        scanf(" %[^\n]", messagebody);

                        
                        
                        initial = message_populate(1l, messagebody, groupid, USERID, option, key);

                        if (msgsnd(msgid, initial, sizeof(message) - sizeof(long), 0) == -1)
                            perror("Error in sending");
                        
                        printf("Message request sent\n");

                        break;
                    case 4:;
                    if(isRegistered){
                        printf("You have been registered already!\n");
                        continue;
                    }
                        key_t publickey = ftok("/home/sankalp/Desktop/NetProg_Assignment1/question2/file3", 121); 
                        int welcoming_msgid = msgget(publickey, 0666 | IPC_CREAT); // welcoming queue

                        
                        
                        initial = message_populate(1l, NULL, -1, -1, option, key);
                        
                        if (msgsnd(welcoming_msgid, initial, sizeof(message) - sizeof(long), 0) == -1)
                            perror("Error in sending");
                        
                        printf("Registration request sent\n");
                        
                        msgrcv(msgid, initial, MAXSIZE, 2, 0);
                        printf("Acknowledgement received!\nYour user id is %d\n", initial->userid);
                        USERID = initial->userid;
                        isRegistered = 1;

                        break;
                    case 5:
                       
                        printf("Enter group id\n");
                        scanf("%d", &groupid);
                        
                        int anythingRead = 0;
                        int isPrinted = 0;

                        initial = (message*) malloc(sizeof(message));
                        
                        //unblock msgrcv
                        while( msgrcv(msgid, initial, MAXSIZE, groupid, 0 | IPC_NOWAIT) != -1) {
                            anythingRead = 1;
                            if(isPrinted == 0) {
                                printf("Received a message from group id %ld\n", initial->mtype);
                                isPrinted = 1;
                            }
                            printf("Sender of the message is %d\n", initial->userid);
                            printf("Content : %s\n", initial->mbody);
                        }
                        if(anythingRead == 0) {
                            printf("You have no new messages from this group\n");
                        }
                    
                        break;
                    case 6:
                        printf("You are exiting!\n");
                        exit(0);
                    default : printf("Invalid option\n");
                }
            }
        }

}
