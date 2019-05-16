//
// Created by Manne Öhlund on 2019-04-10.
//

#include <stdio.h>
#include <jni.h>
#include "core/fileutils.h"
#include "core/ls.c"
#include "core/whoami.c"
#include "core/find.c"
#include "core/exec.c"
#include "core/shell.c"

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
}

void onError(char *error) {
    jstring jStringParam = (*env)->NewStringUTF(env, error);
    if (!jStringParam) {
        printf("failed to alloc param string in java.");
        return;
    };

    (*env)->CallVoidMethod(env, obj, jOnError, jStringParam);
}

void init(JNIEnv *_env, jobject _obj) {
    env = _env;
    obj = _obj;

    jclass jniExampleCls = (*env)->GetObjectClass(env, obj);

    jOnResult = (*env)->GetMethodID(env, jniExampleCls, "onResult", "(Ljava/lang/String;)V");
    jOnError = (*env)->GetMethodID(env, jniExampleCls, "onError", "(Ljava/lang/String;)V");
}

JNIEXPORT jint JNICALL
Java_com_example_jnisample_toolbox_RootTools_initShell(JNIEnv *env, jobject instance) {
    struct subprocess shell;
    return initShell(&shell);
}

JNIEXPORT jint JNICALL
Java_com_example_jnisample_toolbox_RootTools_exitShell(JNIEnv *env, jobject instance) {
    return exitShell();
}

JNIEXPORT jint JNICALL
Java_com_example_jnisample_toolbox_RootTools_interrupt(JNIEnv *env, jobject instance) {
    return interrupt();
}

JNIEXPORT jint JNICALL
Java_com_example_jnisample_toolbox_RootTools_ls(JNIEnv *env, jobject instance, jstring path_) {
    init(env, instance);
    callbacks callbacks = { onResult, onError };

    print_headers();

    const char *path = (*env)->GetStringUTFChars(env, path_, 0);
    int result = ls((char *) path, &callbacks);
    (*env)->ReleaseStringUTFChars(env, path_, path);

    return result;
}

JNIEXPORT jint JNICALL
Java_com_example_jnisample_toolbox_RootTools_whoami(JNIEnv *env, jobject instance) {
    init(env, instance);
    callbacks callbacks = { onResult, onError };

    return whoami(&callbacks);
}

JNIEXPORT jint JNICALL
Java_com_example_jnisample_toolbox_RootTools_find(JNIEnv *env, jobject instance, jstring path_, jstring name_) {
    init(env, instance);
    const char *path = (*env)->GetStringUTFChars(env, path_, 0);
    const char *name = (*env)->GetStringUTFChars(env, name_, 0);

    callbacks callbacks = { onResult, onError };
    int result = find((char *) path, (char *) name, &callbacks);

    (*env)->ReleaseStringUTFChars(env, path_, path);
    (*env)->ReleaseStringUTFChars(env, name_, name);

    return result;
}

JNIEXPORT jint JNICALL
Java_com_example_jnisample_toolbox_RootTools_exec(JNIEnv *env, jobject instance, jstring command_) {
    init(env, instance);
    const char *command = (*env)->GetStringUTFChars(env, command_, 0);

    callbacks callbacks = { onResult, onError };
    int result = execute((char *) command, &callbacks);

    (*env)->ReleaseStringUTFChars(env, command_, command);

    return result;
}

JNIEXPORT jint JNICALL
Java_com_example_jnisample_toolbox_RootTools_execute(JNIEnv *env, jobject instance, jstring command_) {
    init(env, instance);
    const char *command = (*env)->GetStringUTFChars(env, command_, 0);

    callbacks callbacks = { onResult, onError };
    int result = exec((char *) command, &callbacks);

    (*env)->ReleaseStringUTFChars(env, command_, command);

    return result;
}