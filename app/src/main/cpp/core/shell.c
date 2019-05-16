//
// Created by Manne Öhlund on 2019-04-15.
//

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <syslog.h>
#include <fcntl.h>
#include "readline.c"
#include "getline.c"

#define PRMTSIZ 255
#define MAXARGS 63
#define EXITCMD "exit"

pid_t pid;

struct subprocess {
    pid_t pid;
    int in;
    int out;
    int err;
};

struct subprocess pShell;

int initShell(struct subprocess * pSubprocess) ;

void mvFd(int fd1, int fd2);

void readPipe(int fd, result_callback resultCallback);

int exitShell();

int interrupt();

int initShell(struct subprocess * pSubprocess) {
    pShell = *pSubprocess;
    int wstatus;

    int in[2];
    int out[2];
    int err[2];

    // TODO prompt shell loop output
    //printf("%s ", getuid() == 0 ? "#" : "$");
    //fgets(input, PRMTSIZ, in);

    // fork child and execute program
    //signal(SIGINT, SIG_DFL);

    if (pipe(in))
        goto pipe_error;
    if (pipe(out))
        goto pipe_error;
    if (pipe(err))
        goto pipe_error;

    syslog(LOG_CRIT, "jnisample: before fork");
    int ppid = getpid();
    int cpid = fork();

    syslog(LOG_CRIT, "jnisample: after fork, ppid=%d, childpid=%d", ppid, cpid);
    if (cpid == -1) {
        syslog(LOG_CRIT, "jnisample: Fork failed");
        exit(EXIT_FAILURE);
    }

    if (cpid == 0) {
        syslog(LOG_CRIT, "jnisample: Exec child");
        close(0); close(1); close(2);
        mvFd(in[0], STDIN_FILENO);
        mvFd(out[1], STDOUT_FILENO);
        mvFd(err[1], STDERR_FILENO);
        close(in[1]); close(out[0]); close(err[0]);

        char *shell[] = { "/system/bin/sh", NULL };
        //int shExit = execv(shell[0], shell);
        exit(execlp("sh", "sh", NULL));
    } else {
        pShell.pid = cpid;
        syslog(LOG_CRIT, "jnisample: Exec parent");
        close(in[0]); close(out[1]); close(err[1]); // unused child pipe ends
        pShell.in = in[1];   // parent wants to write to subprocess child_in
        pShell.out = out[0]; // parent wants to read from subprocess child_out
        pShell.err = err[0]; // parent wants to read from subprocess child_err
    }
    //signal(SIGINT, SIG_IGN);

    // TODO synchronized shell
    /* wait for program to finish and print exit status
    wait(&wstatus);
    if (WIFEXITED(wstatus)) {
        printf("<%d>\n", WEXITSTATUS(wstatus));
    }
    */

    return 0;

    pipe_error:
        printf("Pipe error, system exit");
}

int exec(char *command, callbacks *callbacks) {
    syslog(LOG_CRIT, "jnisample: exec %s", command);

    write(pShell.in, command, strlen(command));
    write(pShell.in, "\n", strlen("\n"));

    char *eof = "echo '\r'\n";
    write(pShell.in, eof, strlen(eof));

    char *erreof = ">&2 echo '\r'\n";
    write(pShell.in, erreof, strlen(erreof));

    readPipe(pShell.out, callbacks->resultCallback);
    readPipe(pShell.err, callbacks->errorCallback);

    return EXIT_SUCCESS;
}

void mvFd(int fd1, int fd2) {
    dup2(fd1, fd2);
    close(fd1);
}

void readPipe(int fd, result_callback callback) {
    FILE *out = fdopen(fd, "r");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    if (out == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline2(&line, &len, out)) != -1) {
        if(line[read - 1] == '\n' && line[read - 2] == '\r') {
            return;
        } else if (read == 1 && line[read - 1] == '\n') {
            continue;
        }
        syslog(LOG_CRIT, "block read %zu: %s\n", read, line);
        line[read - 1] = '\0';
        callback(line);
    }

    fclose(out);

    if (line)
        free(line);
}

int exitShell() {
    char *exit = "exit\n";
    write(pShell.in, exit, strlen(exit));
    close(pShell.in);
    close(pShell.out);
    close(pShell.err);
    return kill(pShell.pid, SIGQUIT);
}

int interrupt() {
    //signal(SIGKILL, SIG_IGN);
    //return kill(pShell.pid, SIGKILL);
    //kill -s SIGCHLD pid
    //pkill -9 -p $PPID
    char * ppid;
    asprintf(&ppid, "pkill -P %d", pShell.pid);
    char * killShellChildren[] = { "pkill", "-P", ppid, NULL };
    //return execlp("pkill", "pkill", "-P", ppid, NULL);
    return system(ppid);
}

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