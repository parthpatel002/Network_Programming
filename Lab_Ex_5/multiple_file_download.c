#include<stdio.h>
#include<stdlib.h>
#include<string.h>    
#include<unistd.h> 
#include<sys/socket.h>
#include<arpa/inet.h>    
#include<netdb.h>
#include<sys/types.h>
#include<time.h>
#include<errno.h>
#include<fcntl.h>

/*
The following files were downloaded for test purposes:

Internal Link - http://id/TIMETABLE/TimeTableSem2018/1/TIMETABLE.pdf
External Link - http://humanstxt.org/humans.txt
*/

int main(int argc, char* argv[]) {

    char url[1024];
    char name_of_host[1024];
    char file_path[1024];
    char* temporary, *buffer;
    char* remaining_url;
    int s1, s2, x;
    int ix = 0;
    int header_flag = 0;
    int nr= 0;
    struct addrinfo h;
    struct addrinfo *r, *t;
    bzero(&h, sizeof(h));
    h.ai_family = AF_UNSPEC;
    h.ai_socktype = SOCK_STREAM;
    char query[1024] = "";
    struct sockaddr_in *si;
    double t1, t2;
    FILE* file;
    float total_time;

    if(argc != 2) {
        perror("Invalid input");
        exit(1);
    }
    strcpy(url, argv[1]);

    remaining_url = url;    
    temporary = strtok_r(remaining_url, ":", &remaining_url);
    if((strcmp("https", temporary) != 0) && (strcmp("http", temporary) != 0)) {
        remaining_url = url;
        strcpy(name_of_host, strtok_r(remaining_url, "/", &remaining_url));
        strcpy(file_path, strtok_r(remaining_url, "\n\0", &remaining_url));
    }
    else {
        strcpy(name_of_host, strtok_r(remaining_url, "/", &remaining_url));
        strcpy(file_path, strtok_r(remaining_url, "\n\0", &remaining_url));
    }

    if ((x = getaddrinfo(name_of_host, "http", &h, &r)) != 0) {
        perror("getaddrinfo()");
        exit(1);
    }

    t = r;

    while(t != NULL) {
        si = (struct sockaddr_in*)(t->ai_addr);
        if(strcmp(inet_ntoa(si->sin_addr), "0.0.0.0") != 0){
            break;
        }
        t = t->ai_next;
    }
   
    strcat(query, "GET /");
    strcat(query, file_path);
    strcat(query, " HTTP/1.1\r\nHost: ");
    strcat(query, name_of_host);
    strcat(query, "\r\n\r\nConnection: keep-alive\r\n\r\nKeep-Alive: 300\r\n");

    s1 = socket(AF_INET, SOCK_STREAM, 0);
    
    int b = 16192; //16KB
    if (setsockopt(s1, SOL_SOCKET, SO_RCVBUF, &b, sizeof(int)) == -1) {
        perror("setsockopt()");
    }

    int conn = connect(s1, (struct sockaddr*) (t->ai_addr), sizeof(struct sockaddr) );
    if(conn == -1) {
        perror("connect()");
        exit(1);
    }

    file = fopen("file1", "wb");
    buffer = (char*) malloc(sizeof(char) * 1024);
    memset(buffer, 0, 1024);
    
    t1 = clock();
    if ( send( s1, query, strlen(query), 0) == -1) {
        perror("send()");
    }
    while( (nr= recv(s1, buffer, 1024, 0)) > 0) {
        if(header_flag == 0) {
            while(ix-4 < nr && !(buffer[ix] == '\r' && buffer[ix+1] == '\n' && buffer[ix+2] == '\r' && buffer[ix+3] == '\n')) 
                ix++;
            char* buf = buffer + ix + 4;
            fwrite(buf, 1, nr- ix - 4, file);
            memset(buffer, 0, 1024);
            header_flag = 1;
            continue;
        }
        fwrite(buffer, 1, nr, file);
        memset(buffer, 0, 1024);
    }
    t2 = clock();
    total_time = (t2-t1) / CLOCKS_PER_SEC;
    printf("Receive buffer size: 16KB, Time taken in seconds : %lf\n", total_time);
    fclose(file);
    close(s1);

    s2 = socket(AF_INET, SOCK_STREAM, 0);

    b = 2097152; //2MB
    if (setsockopt(s2, SOL_SOCKET, SO_RCVBUF, &b, sizeof(int)) == -1) {
        perror("setsockopt()");
    }

    conn = connect(s2, (struct sockaddr*) (t->ai_addr), sizeof(struct sockaddr) );
    if(conn == -1) {
        perror("connect()");
        exit(1);
    }

    file = fopen("file2", "wb");
    memset(buffer, 0, 1024);
    ix = 0;
    nr= 0;
    header_flag = 0;

    t1 = clock();
    if ( send( s2, query, strlen(query), 0) == -1) {
        perror("send()");
    }

    while( (nr= recv(s2, buffer, 1024, 0)) > 0) {
        if(header_flag == 0) {
            while(ix-4 < nr&& !(buffer[ix] == '\r' && buffer[ix+1] == '\n' && buffer[ix+2] == '\r' && buffer[ix+3] == '\n'))
                ix++;
            char* buf = buffer + ix + 4;
            fwrite(buf, 1, nr- ix - 4, file);
            memset(buffer, 0, 1024);
            header_flag = 1;
            continue;
        }
        fwrite(buffer, 1, nr, file);
        memset(buffer, 0, 1024);
    }
    t2 = clock();
    total_time = (t2-t1) / CLOCKS_PER_SEC;
    printf("Receive buffer size: 2MB, Time taken in seconds : %lf\n", total_time);
    
    fclose(file);
    close(s2);

    return 0;
}
