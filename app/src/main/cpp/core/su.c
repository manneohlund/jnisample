//
// Created by Manne Öhlund on 2019-04-11.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include <unistd.h>
#include <time.h>

#include <pwd.h>

static int executionFailure(char *context) {
    fprintf(stderr, "su: %s. Error:%s\n", context, strerror(errno));
    return -errno;
}

static int permissionDenied() {
    // the superuser activity couldn't be started
    printf("su: permission denied\n Fail to call setuid or setgid\nPls check permission!!\nchmod 6755 su\n");
    return 1;
}

/*
int main(int argc, char **argv) {
    if(setgid(0) || setuid(0)) {
        return permissionDenied();
    }

    char *exec_args[argc + 1];
    exec_args[argc] = NULL;
    exec_args[0] = "sh";
    int i;
    for (i = 1; i < argc; i++) {
        exec_args[i] = argv[i];
    }
    execv("/su/bin/su", exec_args);
    return executionFailure("sh");
}
 */