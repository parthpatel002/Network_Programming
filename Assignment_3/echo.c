#include<syslog.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[]) {
    // FILE* fp = fopen("debug1.txt", "w");
    // fprintf(fp, "%s", argv[1]);
    // fclose(fp);
    char buf[BUF_SIZE];
    ssize_t numRead;
    while ((numRead = read(STDIN_FILENO, buf, BUF_SIZE)) > 0) {
        // printf("%s\n", buf);
        if (write(STDOUT_FILENO, buf, numRead) != numRead) {
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