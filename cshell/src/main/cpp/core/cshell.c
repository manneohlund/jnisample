//
// Created by Manne Öhlund on 2019-04-15.
//

#include "cshell.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <syslog.h>
#include <fcntl.h>
#include "../utils/callbacks.c"

#define PRMTSIZ 255
#define MAXARGS 63
#define EXITCMD "exit"

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
    signal(SIGSEGV, SIG_DFL);

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

        //char *shell[] = { "/system/bin/sh", NULL };
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
    size_t len;
    ssize_t read;

    if (out == NULL)
        exit(EXIT_FAILURE);

    while ((read = getdelim(&line, &len, '\n', out)) != -1) {
        if(read > 1 && line[read - 1] == '\n' && line[read - 2] == '\r') {
            syslog(LOG_CRIT, "RETURN");
            return;
        } else if (read == 1 && line[read - 1] == '\r') {
            syslog(LOG_CRIT, "CONTINUE");
            continue;
        }
        syslog(LOG_CRIT, "block read %zu:%zu: %s\n", read, strlen(line), line);
        if (read > 1 && line[read - 1] == '\n')
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