//
// Created by Manne Öhlund on 2019-05-04.
//

#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <getopt.h>
#include <memory.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include "stdbool.h"

enum size_type { EQ, GT, LT, CT };

enum size_multiplier { B, KB, MB, GB };

struct size_struct {
    enum size_type size_type;
    long size;
    enum size_multiplier size_multiplier;
};

bool case_insensitive = false;
char *pattern;
char *type;
int user = -1;
int group = -1;
int depth = -1;
struct size_values size_values = { CT, -1, B };
int mindepth = -1;
int maxdepth = -1;
bool delete = false;
bool print0 = false;
char cwd[PATH_MAX];

int parseNumericArg(char *value);

enum size_type parseSizePrefixArg(char *pValue);

enum size_multiplier parseSizeSuffixArg(char *pValue);

void multiplySize(long *size, enum size_multiplier multiplier);

void print();

int main(int argc, char **argv) {
    int c;
    int option_index = 0;
    static struct option long_options[] = {
            {"icase",    no_argument,       0, 'i'},
            {"pattern",  required_argument, 0, 'p'},
            {"type",     required_argument, 0, 't'}, // -type [bcdflps] (block, char, dir, file, symlink, pipe, socket)
            {"user",     required_argument, 0, 'u'}, // -user  UNAME    belongs to user UNAME
            {"group",    required_argument, 0, 'g'}, // -group GROUP    belongs to group GROUP
            {"atime",    required_argument, 0, 'x'}, // -atime N[u]     accessed N units ago
            {"ctime",    required_argument, 0, 'y'}, // -ctime N[u]     created/write N units ago
            {"mtime",    required_argument, 0, 'z'}, // -mtime N[u]     modified N units ago
            {"size",     required_argument, 0, 's'}, // -size  [=><]N[kmg]     512 byte blocks (c=bytes)
            {"mindepth", required_argument, 0, 'm'}, // -mindepth       # at least # dirs down
            {"maxdepth", required_argument, 0, 'M'}, // -maxdepth       # at most # dirs down
            {"format",   required_argument, 0, 'f'},
            {"help",     no_argument,       0, 0},
            {"delete",   no_argument,       0, 1},
            {"print0",   no_argument,       0, 2},
            {NULL, 0, NULL,                    0}
    };

    while ((c = getopt_long(argc, argv, "ip:t:u:g:x:y:z:s:m:M:", long_options, &option_index)) != -1) {
        switch (c) {
            case 0:
                printf("usage: [ip] [tugxyzsmM] [--print0] [--delete] [path...]\n");
                exit(EXIT_SUCCESS);

            case 'i':
                case_insensitive = true;
                break;

            case 'p':
                pattern = optarg;
                break;

            case 't':
                type = optarg;
                break;

            case 'u':
                user = parseNumericArg(optarg);
                break;

            case 'g':
                group = parseNumericArg(optarg);
                break;

            case 'd':
                depth = parseNumericArg(optarg);
                break;

            case 's':
                printf("%s\n", optarg);
                size_values.size_type = parseSizePrefixArg(optarg);
                size_values.size = parseNumericArg(optarg);
                size_values.size_multiplier = parseSizeSuffixArg(optarg);
                multiplySize(&size_values.size, size_values.size_multiplier);
                break;

            case 'm':
                mindepth = parseNumericArg(optarg);
                break;

            case 'M':
                maxdepth = parseNumericArg(optarg);
                break;

            case 'f':
                printf(optarg);
                break;

            case 1:
                delete = true;
                break;

            case 2:
                print0 = true;
                break;

            case '?':
                fprintf(stderr, "? usage: [ip] [tugxyzsmM] [--print0] [--delete] [path...]\n");
                exit(EXIT_FAILURE);

            default:
                fprintf(stderr, "default usage: [ip] [tugxyzsmM] [--print0] [--delete] [path...]\n");
                exit(EXIT_FAILURE);
        }
    }

    // Check min and maxdepth
    if (mindepth != -1 && maxdepth != -1 && maxdepth < mindepth) {
        fprintf(stderr, "mindeoth %d is larger than maxdepth %d", mindepth, maxdepth);
        exit(EXIT_FAILURE);
    }

    print();

    // Path or Current working dir
    if (optind < argc) {
        //strcpy(path, argv[optind++]);
        while (optind < argc)
            printf("Path = %s\n", argv[optind++]);
    } else if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
    } else {
        perror("getcwd() error");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

/****************************************************
 *  SIZE
 *****************************************************/

enum size_type parseSizePrefixArg(char *value) {
    if (optarg[0] == '=') {
        optarg[0] = '0';
        return EQ;
    } else if (optarg[0] == '>') {
        optarg[0] = '0';
        return GT;
    } else if (optarg[0] == '<') {
        optarg[0] = '0';
        return LT;
    }
    return CT;
}

enum size_multiplier parseSizeSuffixArg(char *value) {
    int last = tolower(value[strlen(value) - 1]);
    if (last == 'g') {
        return GB;
    } else if (last == 'm') {
        return MB;
    } else if (last == 'k') {
        return KB;
    }
    return B;
}

void multiplySize(long *size, enum size_multiplier multiplier) {
    if (multiplier == GB) {
        *size = *size * 1024 * 1024 * 1024;
    } else if (multiplier == MB) {
        *size = *size * 1024 * 1024;
    } else if (multiplier == KB) {
        *size = *size * 1024;
    }
}

int parseNumericArg(char *value) {
    char *ptr;

    /* Convert the provided value to a decimal long */
    long result = strtol(value, &ptr, 10);

    /* If the result is 0, test for an error */
    if (result == 0) {
        /* If a conversion error occurred, display a message and exit */
        if (errno == EINVAL) {
            perror(value);
            exit(EXIT_FAILURE);
        }

        /* If the value provided was out of range, display a warning message */
        if (errno == ERANGE)
            printf("The value provided was out of range\n");
    }

    return (int) result;
}


/****************************************************
 *  PRINT
 *****************************************************/

void print() {
    if (case_insensitive) printf("icase = %d\n", case_insensitive);
    if (pattern) printf("pattern = %s\n", pattern);
    if (type) printf("type = %s\n", type);
    if (user != -1) printf("user = %d\n", user);
    if (group != -1) printf("group = %d\n", group);
    if (depth != -1) printf("depth = %d\n", depth);
    if (size_values.size != -1) {
        printf("size_type = %d\n", size_values.size_type);
        printf("size = %ld\n", size_values.size);
        printf("size_multiplier = %d\n", size_values.size_multiplier);
    }
    if (mindepth != -1) printf("mindepth = %d\n", mindepth);
    if (maxdepth != -1) printf("maxdepth = %d\n", maxdepth);
    if (delete) printf("delete = %d\n", delete);
    if (print0) printf("print0 = %d\n", print0);
}

/*
if (optind < argc) {
    printf("non-option ARGV-elements: ");
    while (optind < argc)
        printf("%s ", argv[optind++]);
    printf("\n");
}
*/