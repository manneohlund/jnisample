//
// Created by Manne Öhlund on 2019-04-20.
//

#ifndef JNISAMPLE_CSHELL_H
#define JNISAMPLE_CSHELL_H

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <syslog.h>
#include <fcntl.h>
#include "getline.c"
#include "../utils/callbacks.c"

struct subprocess pShell;

struct subprocess {
    pid_t pid;
    int in;
    int out;
    int err;
};

int initShell(struct subprocess * pSubprocess) ;

void mvFd(int fd1, int fd2);

void readPipe(int fd, result_callback resultCallback);

int exitShell();

int interrupt();

#endif //JNISAMPLE_CSHELL_H

/*
//setvbuf(stdout, 0, _IONBF, 0); // linebuffered = _IOLBF
//setvbuf(stderr, 0, _IONBF, 0); // no buffer = _IONBF
 */

/*ssize_t rdsz;
    char buf[128];
    while((rdsz = read(pShell.out, buf, sizeof buf - 1)) > 0) {
        if(buf[rdsz - 2] == '\r' && buf[rdsz - 1] == '\n') {
            syslog(LOG_CRIT, "EXIT block read %zu: %s\n", rdsz, buf);
            return 0;
        }
        buf[rdsz] = 0;  // add null-terminator
        syslog(LOG_CRIT, "block read %zu: %s\n", rdsz, buf);
        callbacks->resultCallback(buf);
    }*/

/*
 char* line=0;
    size_t line_buf_len=0;
    ssize_t curr_line_len;

    while ((curr_line_len = readline(&line, &line_buf_len, &pShell.out)) > 0) {
        if(line[curr_line_len - 2] == '\r' && line[curr_line_len - 1] == '\n') {
            syslog(LOG_CRIT, "EXIT block read %zu: %s\n", curr_line_len, line);
            return 0;
        }
        //line[curr_line_len] = 0;  // add null-terminator
        syslog(LOG_CRIT, "block read %zu: %s\n", curr_line_len, line);
        callbacks->resultCallback(line);
    }*/

/*while ((read(pShell.out, line, sizeof(line)-1)) > 0) {
    syslog(LOG_CRIT, "block read: \n<%s>\n", line);
    callbacks->resultCallback(line);
}

while ((read(pShell.err, line, sizeof(line)-1)) > 0) {
    syslog(LOG_CRIT, "block read: \n<%s>\n", line);
    callbacks->resultCallback(line);
}*/

/*
char buf[BUFSIZ];
read(out[0], buf, BUFSIZ);
callbacks->resultCallback(buf);
char buf2[BUFSIZ];
read(err[0], buf2, BUFSIZ);
callbacks->resultCallback(buf);
 */