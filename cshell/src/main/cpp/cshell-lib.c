//
// Created by Manne Öhlund on 2019-04-10.
//

#include <stdio.h>
#include <jni.h>
#include "utils/fileutils.h"
#include "core/cshell.c"
#include "utils/callbacks.c"

JNIEnv *env;
jobject obj;
jmethodID jOnResult, jOnError;

void onResult(char *result) {
    jstring jStringParam = (*env)->NewStringUTF(env, result);
    if (!jStringParam) {
        printf("failed to alloc param string in java.");
        return;
    };

    (*env)->CallVoidMethod(env, obj, jOnResult, jStringParam);
    (*env)->DeleteLocalRef(env, jStringParam);
}

void onError(char *error) {
    jstring jStringParam = (*env)->NewStringUTF(env, error);
    if (!jStringParam) {
        printf("failed to alloc param string in java.");
        return;
    };

    (*env)->CallVoidMethod(env, obj, jOnError, jStringParam);
    (*env)->DeleteLocalRef(env, jStringParam);
}

void init(JNIEnv *_env, jobject _obj) {
    env = _env;
    obj = _obj;

    jclass jniExampleCls = (*env)->GetObjectClass(env, obj);

    jOnResult = (*env)->GetMethodID(env, jniExampleCls, "onResult", "(Ljava/lang/String;)V");
    jOnError = (*env)->GetMethodID(env, jniExampleCls, "onError", "(Ljava/lang/String;)V");
}

JNIEXPORT jint JNICALL
Java_cshell_CShell_initShell(JNIEnv *env, jobject instance) {
    struct subprocess shell;
    return initShell(&shell);
}

JNIEXPORT jint JNICALL
Java_cshell_CShell_exitShell(JNIEnv *env, jobject instance) {
    return exitShell();
}

JNIEXPORT jint JNICALL
Java_cshell_CShell_interrupt(JNIEnv *env, jobject instance) {
    return interrupt();
}

JNIEXPORT jint JNICALL
Java_cshell_CShell_execute(JNIEnv *env, jobject instance, jstring command_) {
    init(env, instance);
    const char *command = (*env)->GetStringUTFChars(env, command_, 0);

    callbacks callbacks = { onResult, onError };
    int result = exec((char *) command, &callbacks);

    (*env)->ReleaseStringUTFChars(env, command_, command);

    return result;
}