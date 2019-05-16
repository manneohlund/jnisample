#include <stdlib.h>
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>
#include "callbacks.c"

char * result;

int whoami(callbacks *callbacks) {
    struct passwd *pw = getpwuid(geteuid());
    if (!pw) {
        asprintf(&result, "whoami: cannot find username for UID %u\n", (unsigned)pw->pw_uid);
        callbacks->errorCallback(result);
        return EXIT_FAILURE;
    }
    asprintf(&result, "%s\n", pw->pw_name);
    callbacks->resultCallback(result);
    return EXIT_SUCCESS;
}

/*int main(int argc, char *argv[]) {
    callbacks callbacks = { stdoutput, stderror };
    exit(whoami(&callbacks));
}*/