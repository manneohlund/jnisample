//
// Created by Manne Öhlund on 2019-05-16.
//

#ifndef JNISAMPLE_FIND_H
#define JNISAMPLE_FIND_H

#include <sys/types.h>
#include <getopt.h>
#include <limits.h>

struct dirent *pDirent;
int currentDepth = 0;

// File stat
struct find_stat {
    char *pFullPath;
    char *pCurrentDir;
    char fileModes[12];
    char *fileName;
    struct stat fileStat;
    char linkPath[1024];
} find_stat;

// Regex
regex_t regex;
int regex_result;

// Date format
char buffer[100];
struct tm *tm_info;

// Option checker
struct option_check {
    bool case_insensitive;
    bool depth;
    bool pattern;
    bool type;
    bool user;
    bool group;
    bool size;
    bool mindepth;
    bool maxdepth;
    bool stat_format;
    bool date_format;
    bool delete;
    bool print0;
    bool options;
} is_option_set;

// Option value holder
struct option_values {
    int depth;
    char *pattern;
    int type;
    int user;
    int group;
    int mindepth;
    int maxdepth;
    char *stat_format;
    char *date_format;
    char cwd[PATH_MAX]; // Current working dir
} option_values;

enum size_type {
    EQ, GT, LT, CT
};

enum size_multiplier {
    B, KB, MB, GB
};

struct size_struct {
    enum size_type size_type;
    long size;
    enum size_multiplier size_multiplier;
} size_values;

enum {
    HELP = CHAR_MAX + 1,
    DELETE,
    PRINT0
};

static struct option long_options[] = {
        {"icase",        no_argument,       0, 'i'},
        {"depth",        no_argument,       0, 'd'},
        {"pattern",      required_argument, 0, 'p'},
        {"type",         required_argument, 0, 't'}, // -type [bcdflps] (block, char, dir, file, symlink, pipe, socket)
        {"user",         required_argument, 0, 'u'}, // -user  UNAME    belongs to user UNAME
        {"group",        required_argument, 0, 'g'}, // -group GROUP    belongs to group GROUP
        {"user-name",    required_argument, 0, 'U'}, // -user  UNAME    belongs to user UNAME
        {"group-name",   required_argument, 0, 'G'}, // -group GROUP    belongs to group GROUP
        {"atime",        required_argument, 0, 'x'}, // -atime N[u]     accessed N units ago
        {"ctime",        required_argument, 0, 'y'}, // -ctime N[u]     created/write N units ago
        {"mtime",        required_argument, 0, 'z'}, // -mtime N[u]     modified N units ago
        {"size",         required_argument, 0, 's'}, // -size  [=><]N[kmg]     512 byte blocks (c=bytes)
        {"mindepth",     required_argument, 0, 'm'}, // -mindepth       # at least # dirs down
        {"maxdepth",     required_argument, 0, 'M'}, // -maxdepth       # at most # dirs down
        {"stat-format",  required_argument, 0, 'f'},
        {"stat-default", no_argument,       0, 'F'},
        {"date-format",  required_argument, 0, 'D'},
        {"options",      no_argument,       0, 'o'},
        {"help",         no_argument,       0, HELP},
        {"delete",       no_argument,       0, DELETE},
        {"print0",       no_argument,       0, PRINT0},
        {NULL, 0, NULL,                        0}
};

bool validateFind();

void printFind();

void statPath(char *pPath);

void printDateFormat(time_t timer);

void printStatFormat();

void multiplySize(long *pSize, enum size_multiplier multiplier);

void divideSize(long *pSize, enum size_multiplier multiplier);

gid_t getGroupIdByName(const char *pName);

uid_t getUserIdByName(const char *pName);

char *getGroupNameById(uid_t uid);

char *getUserNameById(uid_t uid);

bool isSizeMatch(long *pSize);

bool isWithinCurrentDepth();

int parseNumericArg(char *pValue);

int parseFileTypeArg(const char *pValue);

enum size_type parseSizePrefixArg(char *pValue);

enum size_multiplier parseSizeSuffixArg(char *pValue);

void validateStatFormat();

void validateDateFormat();

void printParams();

void usage(int status);

#endif //JNISAMPLE_FIND_H
