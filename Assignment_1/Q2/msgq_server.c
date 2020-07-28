#include<stdio.h>
#include <sys/types.h>
#include <sys/msg.h>
#include<stdlib.h>
#include<sys/ipc.h>
#include<string.h>

#define CREATE 0
#define LIST 1
#define JOIN 2
#define SEND 3
#define REGISTER 4


#define MAXSIZE 1024
#define MAXUSERSINGROUP 100
#define MAXGROUPS 100

typedef struct {
    long mtype; // 1 is read by server, 2 is read by client for acknowledgment, 3 (notification), beyond are group ids to be read by client.
    char mbody[100]; // send message
    int groupid; // group id
    int userid;
    int option; 
    key_t key;
} message;

typedef struct {
    int size;
    int users[MAXUSERSINGROUP];
    char groupname[20];
} group;

typedef group* GroupList;

typedef struct {
    int msgid;
    int uid;
} messagequeue;

typedef messagequeue* messageQueues;

#define MAXUSERS 100
messageQueues mqs;
GroupList G;

int mqs_size = 0;
int G_size = 0;

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
    key_t publickey = ftok("/home/sankalp/Desktop/NetProg_Assignment1/question2/file3", 121); 
    int welcoming_msgid = msgget(publickey, 0666 | IPC_CREAT); // welcoming queue
    
    messagequeue welcomeQueue;
    welcomeQueue.uid = 0;
    welcomeQueue.msgid = welcoming_msgid;

    mqs = (messageQueues)malloc(sizeof(messagequeue) * MAXUSERS);
    G = (GroupList)malloc(sizeof(group)*MAXGROUPS);

    mqs[0] = welcomeQueue;
    mqs_size++;

    message readMessage;

    while(1) {
        message* reply;
        group g;
        int groupid;
        int userid;
        for(int i = 0; i < mqs_size; i++) {
            while( msgrcv(mqs[i].msgid, &readMessage, sizeof(message), 1, IPC_NOWAIT) != -1) {
                switch(readMessage.option) {
                    case 0 :;
                        strcpy(G[G_size].groupname, readMessage.mbody);
                        G[G_size].size = 1;
                        G[G_size].users[G[G_size].size-1] = readMessage.userid;
                        G_size++; 
                        reply = message_populate(2, "Successfully created group", G_size - 1 + 10, -1, -1, publickey);
                        if (msgsnd(mqs[i].msgid, reply, sizeof(message) - sizeof(long), 0) == -1)
                            perror("Error in sending acknowledgment");
                        break;
                    case 1 :;
                        char* replyString = (char*)malloc(1024*sizeof(char));
                        memset(replyString, 0, 1024);

                        char suffix[50];
                        for(int j = 0; j < G_size; j++) {
                            sprintf(suffix, "%s %d\n", G[j].groupname, (10+j));
                            replyString = strcat(replyString, suffix);
                        }
                        
                        reply = message_populate(2, replyString, -1, -1, -1, publickey);
                        if (msgsnd(mqs[i].msgid, reply, sizeof(message) - sizeof(long), 0) == -1)
                            perror("Error in sending acknowledgment");
                        break;
                    case 2 :
                        groupid = readMessage.groupid;
                        userid = readMessage.userid;
                        int isPresent = 0;
                        for(int j = 0; j < G[groupid-10].size; j++) {
                            if(G[groupid-10].users[j] == userid) {
                                isPresent = 1;
                                break;
                            }
                        }
                        if(isPresent) {
                            reply = message_populate(2, "Already in group!", -1, -1, -1, publickey);
                            if (msgsnd(mqs[i].msgid, reply, sizeof(message) - sizeof(long), 0) == -1)
                                perror("Error in sending acknowledgment");
                            
                        }
                        else{
                            G[groupid - 10].users[G[groupid-10].size] = userid;
                            G[groupid - 10].size++;
                            reply = message_populate(2, "Successfully added to group!", groupid, -1, -1, publickey);
                            if (msgsnd(mqs[i].msgid, reply, sizeof(message) - sizeof(long), 0) == -1)
                                perror("Error in sending acknowledgment");
                        }
                        // printf("Group ID : %d\n", groupid);
                        // for(int j = 0; j < G[groupid-10].size; j++) {
                        //     printf("user id %d, ", G[groupid-10].users[j]);
                        // }
                        // printf("\n");
                        // printf("case 2 in server\n");
                        
                        break;
                    case 3 :
                        groupid = readMessage.groupid;
                        userid = readMessage.userid;
                        char buf[256];
                        strcpy(buf, readMessage.mbody);
                        for(int j = 0; j < G[groupid-10].size; j++) {
                            if(G[groupid-10].users[j] == userid) {
                                continue;
                            }
                            reply = message_populate(3, NULL, groupid, userid, -1, publickey);
                            if (msgsnd(mqs[G[groupid-10].users[j]].msgid, reply, sizeof(message) - sizeof(long), 0) == -1)
                                perror("Error in sending notification");
                            reply = message_populate(groupid, buf, groupid, userid, -1, publickey);
                            if (msgsnd(mqs[G[groupid-10].users[j]].msgid, reply, sizeof(message) - sizeof(long), 0) == -1)
                                perror("Error in sending notification");
                        }
                        // printf("case 3 in server\n");
                        break;
                    case 4 :;
                        messagequeue m;
                        m.msgid = msgget(readMessage.key, 0666 | IPC_CREAT);
                        m.uid = mqs_size;
                        mqs[mqs_size] = m;
                        mqs_size++;

                        reply = message_populate(2, "Successfully registered", -1, mqs_size - 1, -1, publickey);
                        if (msgsnd(m.msgid, reply, sizeof(message) - sizeof(long), 0) == -1)
                            perror("Error in sending acknowledgment");
                        // for(int j = 0; j < mqs_size; j++) {
                        //     printf("msg id %d , uid %d\n", mqs[j].msgid, mqs[j].uid);
                        // }
                        // printf("case 4 in server\n");
                        break;
                    default :
                        printf("Invalid option received at server!\n");
                }
            }
        } 
    }
}
