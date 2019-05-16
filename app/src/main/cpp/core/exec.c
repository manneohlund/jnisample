//
// Created by Manne Öhlund on 2019-04-11.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fileutils.c"
#include "callbacks.c"
#include "su.c"

int exec2(char *pCommand, callbacks *callbacks) {
    //system(pCommand);
    //fork();
    //system("whoami");
    FILE *fpipe;
    char line[256];

    if (!(fpipe = popen(pCommand, "r"))) {  // If fpipe is NULL
        callbacks->errorCallback("Problems with pipe");
        return EXIT_FAILURE;
    }

    callbacks->resultCallback("Reading");
    while (fgets(line, sizeof line, fpipe)) {
        callbacks->resultCallback(line);
    }

    pclose(fpipe);
    callbacks->resultCallback(pCommand);
    return EXIT_SUCCESS;
}

int execute(char *pCommand, callbacks *callbacks) {
    FILE *fpipe;
    char line[256];

    callbacks->resultCallback("Reading");
    if (!(fpipe = popen(pCommand, "r"))) {  // If fpipe is NULL
        callbacks->errorCallback("Problems with pipe");
        return EXIT_FAILURE;
    }

    char * result;
    while (fgets(line, sizeof line, fpipe)) {
        asprintf(&result, "%s", line);
        callbacks->resultCallback(result);
    }

    return pclose(fpipe) / 256;
}