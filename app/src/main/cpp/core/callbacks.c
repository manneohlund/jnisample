//
// Created by Manne Öhlund on 2019-04-10.
//

#ifndef _CALLBACKS_
#define _CALLBACKS_

typedef void (*result_callback) (char *result);

typedef struct _callbacks {
    result_callback resultCallback;
    result_callback errorCallback;
} callbacks;

void stdoutput(char *result) {
    fprintf(stdout, "%s", result);
}

void stderror(char *error) {
    fprintf(stderr, "%s", error);
}

#endif