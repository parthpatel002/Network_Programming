#include<syslog.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include<syslog.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/select.h>
#include<sys/types.h>
#include<signal.h>
#include<sys/wait.h>
#include<pwd.h>
#include<errno.h>


#define BUF_SIZE 4096

int main(int argc, char *argv[]) {
    // FILE* fp = fopen("debug2.txt", "w");
    // fprintf(fp, "%s", argv[1]);
    // fclose(fp);
    char buf[BUF_SIZE];
    struct sockaddr_in si_o;
    memset((char*) &si_o, 0, sizeof(si_o) );
    int slen = sizeof(si_o);
    ssize_t numRead;
    while ((numRead = recvfrom(STDIN_FILENO, buf, BUF_SIZE, 0, (struct sockaddr*)&si_o,  &slen)) > 0) {
        // printf("%s\n", buf);
        if (sendto(STDOUT_FILENO, buf, numRead, 0, (struct sockaddr*)&si_o,  slen) != numRead) {
            // syslog(LOG_ERR, "write() failed: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    if (numRead == -1) {
        // syslog(LOG_ERR, "Error from read(): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}