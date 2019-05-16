#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <regex.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <limits.h>
#include <zconf.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <sys/param.h>
#include "../utils/fileutils.c"
#include "../utils/stat-time.h"

struct dirent *pDirent, pFile;
struct stat filestat;
char file_modes[12];
char *result;
char *pFileName;
char *pDirName;
regex_t regex;
int regex_result;

enum {
    HELP = CHAR_MAX + 1,
    DELETE,
    PRINT0
};

static struct option long_options[] = {
        {"icase",       no_argument,       0, 'i'},
        {"depth",       no_argument,       0, 'd'},
        {"pattern",     required_argument, 0, 'p'},
        {"type",        required_argument, 0, 't'}, // -type [bcdflps] (block, char, dir, file, symlink, pipe, socket)
        {"user",        required_argument, 0, 'u'}, // -user  UNAME    belongs to user UNAME
        {"group",       required_argument, 0, 'g'}, // -group GROUP    belongs to group GROUP
        {"user-name",   required_argument, 0, 'U'}, // -user  UNAME    belongs to user UNAME
        {"group-name",  required_argument, 0, 'G'}, // -group GROUP    belongs to group GROUP
        {"atime",       required_argument, 0, 'x'}, // -atime N[u]     accessed N units ago
        {"ctime",       required_argument, 0, 'y'}, // -ctime N[u]     created/write N units ago
        {"mtime",       required_argument, 0, 'z'}, // -mtime N[u]     modified N units ago
        {"size",        required_argument, 0, 's'}, // -size  [=><]N[kmg]     512 byte blocks (c=bytes)
        {"mindepth",    required_argument, 0, 'm'}, // -mindepth       # at least # dirs down
        {"maxdepth",    required_argument, 0, 'M'}, // -maxdepth       # at most # dirs down
        {"stat-format", required_argument, 0, 'f'},
        {"date-format", required_argument, 0, 'F'},
        {"options",     no_argument,       0, 'o'},
        {"help",        no_argument,       0, HELP},
        {"delete",      no_argument,       0, DELETE},
        {"print0",      no_argument,       0, PRINT0},
        {NULL, 0, NULL,                       0}
};

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

int currentDepth = 0;

bool isWithinCurrentDepth();

int parseNumericArg(char *value);

int parseFileTypeArg(const char *pValue);

enum size_type parseSizePrefixArg(char *pValue);

enum size_multiplier parseSizeSuffixArg(char *pValue);

bool isSizeMatch(long *pSize);

void multiplySize(long *pSize, enum size_multiplier multiplier);

void divideSize(long *pSize, enum size_multiplier multiplier);

gid_t getGroupIdByName(const char *pName);

uid_t getUserIdByName(const char *pName);

char *getGroupNameById(uid_t uid);

char *getUserNameById(uid_t uid);

static void printStat(char *pPath);

void printDateFormat(time_t timer);

void printParams();

void usage(int status);

void validateStatFormat();

void validateDateFormat();

void printStatFormat(char *pPath);

int find(char *pPath) {
    currentDepth++;
    DIR *pDirectory;

    if ((pDirectory = opendir(pPath)) == NULL) {
        perror(pPath);
        return EXIT_FAILURE;
    } else if (currentDepth == 1) {
        printStat(pPath);
    }

    while ((pDirent = readdir(pDirectory)) != NULL) {
        if (!is_option_set.depth && isWithinCurrentDepth()) {
            if (!is_option_set.type || pDirent->d_type == option_values.type) {
                if ((!IS_FOLDER_POINTER(pDirent->d_name) && option_values.pattern == NULL)
                    || (!IS_FOLDER_POINTER(pDirent->d_name) && !regexec(&regex, pDirent->d_name, 0, NULL, 0))) {

                    asprintf(&result, "%s/%s", pPath, pDirent->d_name);
                    asprintf(&pFileName, "%s", pDirent->d_name);
                    asprintf(&pDirName, "%s", pPath);
                    pFile = *pDirent;
                    printStat(result);
                }
            }

            if (pDirent->d_type == DT_DIR && !IS_FOLDER_POINTER(pDirent->d_name)) {
                asprintf(&result, "%s/%s", pPath, pDirent->d_name);
                find(result);
            }
        }
    }

    currentDepth--;
    closedir(pDirectory);

    return EXIT_SUCCESS;
}

static void printStat(char *pPath) {
    if (stat(pPath, &filestat)) {
        perror(pPath);
        return;
    }

    // Size
    long size = filestat.st_size;
    if (is_option_set.size && !isSizeMatch(&size))
        return;

    // User
    if (is_option_set.user && filestat.st_uid != option_values.user)
        return;

    // Group
    if (is_option_set.group && filestat.st_gid != option_values.group)
        return;

    if (is_option_set.stat_format) {
        printStatFormat(pPath);
    } else {
        printf("%s%c", pPath, is_option_set.print0 ? '\0' : '\n');
    }
}

bool isSizeMatch(long *pSize) {
    /*printf("\nPSIZE = %ld\n", *pSize);
    printf("SIZE  = %ld\n", size_values.size);
    printf("SIZE min  = %ld\n", (long) (size_values.size * 0.95));
    printf("SIZE max  = %ld\n", (long) (size_values.size * 1.05));*/
    if (size_values.size_type == EQ) {
        divideSize(pSize, size_values.size_multiplier);
        multiplySize(pSize, size_values.size_multiplier);
        return *pSize == size_values.size;
    } else if (size_values.size_type == GT) {
        return *pSize >= size_values.size;
    } else if (size_values.size_type == LT) {
        return *pSize <= size_values.size;
    } else { // CT
        return *pSize <= (long) (size_values.size * 1.15) && *pSize >= (long) (size_values.size * 0.95);
    }
}

int main(int argc, char *argv[]) {
    int c;
    int option_index = 0;

    while ((c = getopt_long(argc, argv, "oidp:t:u:g:U:G:x:y:z:s:m:M:f:F:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'o':
                is_option_set.options = true;
                break;

            case 'i':
                is_option_set.case_insensitive = true;
                break;

            case 'd':
                is_option_set.depth = true;
                break;

            case 'p':
                is_option_set.pattern = true;
                option_values.pattern = optarg;
                break;

            case 't':
                is_option_set.type = true;
                option_values.type = parseFileTypeArg(optarg);
                break;

            case 'u':
                is_option_set.user = true;
                option_values.user = parseNumericArg(optarg);
                break;

            case 'g':
                is_option_set.group = true;
                option_values.group = parseNumericArg(optarg);
                break;

            case 'U':
                is_option_set.user = true;
                option_values.user = getUserIdByName(optarg);
                break;

            case 'G':
                is_option_set.group = true;
                option_values.group = getGroupIdByName(optarg);
                break;

            case 's':
                is_option_set.size = true;
                size_values.size_type = parseSizePrefixArg(optarg);
                size_values.size = parseNumericArg(optarg);
                size_values.size_multiplier = parseSizeSuffixArg(optarg);
                multiplySize(&size_values.size, size_values.size_multiplier);
                break;

            case 'm':
                is_option_set.mindepth = true;
                option_values.mindepth = parseNumericArg(optarg);
                break;

            case 'M':
                is_option_set.maxdepth = true;
                option_values.maxdepth = parseNumericArg(optarg);
                break;

            case 'f':
                is_option_set.stat_format = true;
                option_values.stat_format = optarg;
                validateStatFormat();
                break;

            case 'F':
                is_option_set.date_format = true;
                option_values.date_format = optarg;
                validateDateFormat();
                break;

            case HELP:
                usage(EXIT_SUCCESS);

            case DELETE:
                is_option_set.delete = true;
                break;

            case PRINT0:
                is_option_set.print0 = true;
                break;

            case '?':
                fprintf(stderr, "usage: [-oid] [ptugUGsmM] [--print0] [--delete] [--help] [path...]\n");
                exit(EXIT_FAILURE);

            default:
                fprintf(stderr, "usage: [-oid] [ptugUGsmM] [--print0] [--delete] [--help] [path...]\n");
                exit(EXIT_FAILURE);
        }
    }

    if (option_values.pattern != NULL) {
        regex_result = regcomp(&regex, option_values.pattern,
                               REG_EXTENDED | (is_option_set.case_insensitive ? REG_ICASE : 0));
        if (regex_result) {
            // Any value different from 0 means it was not possible to
            // compile the regular expression, either for memory problems
            // or problems with the regular expression syntax.
            if (regex_result == REG_ESPACE)
                fprintf(stderr, "%s\n", strerror(ENOMEM));
            else
                fprintf(stderr, "Invalid expression: %s\n", option_values.pattern);
            return EXIT_FAILURE;
        }
    }

    if (is_option_set.options) {
        printParams();
        exit(EXIT_SUCCESS);
    }

    // Check min and maxdepth
    if (!is_option_set.mindepth && !is_option_set.maxdepth && option_values.maxdepth < option_values.mindepth) {
        fprintf(stderr, "mindeoth %d is larger than maxdepth %d", option_values.mindepth, option_values.maxdepth);
        exit(EXIT_FAILURE);
    }

    int result = EXIT_SUCCESS;

    // Execute find on path... or current working dir
    if (optind < argc) {
        while (optind < argc)
            //printf("Path = %s\n", argv[optind++]);
            result = find(removeTrailingSlash(argv[optind++]));
    } else if (getcwd(option_values.cwd, sizeof(option_values.cwd)) != NULL) {
        //printf("Current working dir: %s\n", option_values.cwd);
        result = find(removeTrailingSlash(option_values.cwd));
    } else {
        perror("getcwd() error");
        exit(EXIT_FAILURE);
    }

    regfree(&regex);
    exit(result);
}

void validateStatFormat() {
    char const *b;
    for (b = option_values.stat_format; *b; b++) {
        switch (*b) {
            case '%':
                b++;
                switch (*b) {
                    case 'a':
                    case 'A':
                    case 's':
                    case 'f':
                    case 'F':
                    case 'g':
                    case 'G':
                    case 'i':
                    case 'n':
                    case 'N':
                    case 'o':
                    case 'p':
                    case 'u':
                    case 'U':
                    case 'x':
                    case 'X':
                    case 'y':
                    case 'Y':
                    case 'z':
                    case 'Z':
                        break;
                    default:
                        errno = EINVAL;
                        fprintf(stderr, "Format char '%c': ", *b);
                        perror("");
                        exit(EXIT_FAILURE);
                }
                break;
            default:
                break;
        }
    }
}

void printStatFormat(char *pPath) {
    char const *b;
    for (b = option_values.stat_format; *b; b++) {
        switch (*b) {
            case '%':
                b++;
                switch (*b) {
                    case 'a':
                        printf("%o", filestat.st_mode);                     // Access bits (octal)
                        break;
                    case 'A':
                        mode_to_letter(filestat.st_mode, file_modes);
                        printf("%s", file_modes);                           // File modes
                        break;
                    case 's':
                        printf("%lli", filestat.st_size);                   // Size
                        break;
                    case 'f':
                        printf("%x", filestat.st_mode);                     // All mode bits (hex)
                        break;
                    case 'F':
                        printf("%d", pFile.d_type);                         // File type
                        break;
                    case 'g':
                        printf("%u", filestat.st_gid);                      // Group id
                        break;
                    case 'G':
                        printf("%s", getGroupNameById(filestat.st_gid));    // Group name
                        break;
                    case 'i':
                        printf("%llu", filestat.st_ino);                    // Inode
                        break;
                    case 'n':
                        printf("%s", pFile.d_name);                         // Filename
                        break;
                    case 'N':
                        printf("%s", pPath);                                // Long filename
                        break;
                    case 'o':
                        printf("%lli", filestat.st_blocks);                 // I/O block size
                        break;
                    case 'p':
                        printf("%s", pDirName);                             // File path
                        break;
                    case 'u':
                        printf("%u", filestat.st_uid);                      // User id
                        break;
                    case 'U':
                        printf("%s", getUserNameById(filestat.st_uid));     // User name
                        break;
                    case 'x':
                        printDateFormat(get_stat_atime(&filestat).tv_sec);  // Access time
                        break;
                    case 'X':
                        printf("%li", get_stat_atime(&filestat).tv_sec);    // Access time unix
                        break;
                    case 'y':
                        printDateFormat(get_stat_mtime(&filestat).tv_sec);  // File mod/write time
                        break;
                    case 'Y':
                        printf("%li", get_stat_mtime(&filestat).tv_sec);    // File mod/write time unix
                        break;
                    case 'z':
                        printDateFormat(get_stat_ctime(&filestat).tv_sec);  // File change time
                        break;
                    case 'Z':
                        printf("%li", get_stat_ctime(&filestat).tv_sec);    // File change time unix
                        break;
                    default:
                        errno = EINVAL;
                        fprintf(stderr, "Format char '%c': ", *b);
                        perror("");
                        exit(EXIT_FAILURE);
                }
                break;
            default:
                putchar(*b);
                break;
        }
    }
    putchar('\n');
}

char buffer[100];
struct tm *tm_info;

void printDateFormat(time_t timer) {
    tm_info = localtime(&timer);
    strftime(buffer, 100, option_values.date_format, tm_info);
    printf("%s", buffer);
}

void validateDateFormat() {
    char const *b;
    for (b = option_values.date_format; *b; b++) {
        switch (*b) {
            case '%':
                b++;
                switch (*b) {
                    case 'a':
                    case 'A':
                    case 'b':
                    case 'B':
                    case 'c':
                    case 'C':
                    case 'd':
                    case 'D':
                    case 'e':
                    case 'E':
                    case 'F':
                    case 'g':
                    case 'G':
                    case 'h':
                    case 'H':
                    case 'I':
                    case 'j':
                    case 'k':
                    case 'l':
                    case 'm':
                    case 'M':
                    case 'n':
                    case 'O':
                    case 'p':
                    case 'P':
                    case 'r':
                    case 'R':
                    case 's':
                    case 'S':
                    case 't':
                    case 'T':
                    case 'u':
                    case 'U':
                    case 'V':
                    case 'w':
                    case 'W':
                    case 'x':
                    case 'X':
                    case 'y':
                    case 'Y':
                    case 'z':
                    case 'Z':
                    case '+':
                    case '%':
                        break;
                    default:
                        errno = EINVAL;
                        fprintf(stderr, "Date format char '%c': ", *b);
                        perror("");
                        exit(EXIT_FAILURE);
                }
                break;
            default:
                break;
        }
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
 *  User / Group
 *****************************************************/

gid_t getGroupIdByName(const char *pName) {
    struct group *grp = getgrnam(pName); /* don't free, see getgrnam() for details */
    if (grp == NULL) {
        errno = EINVAL;
        perror(pName);
        exit(EXIT_FAILURE);
    }
    return grp->gr_gid;
}

uid_t getUserIdByName(const char *pName) {
    struct passwd *pwd = getpwnam(pName); /* don't free, see getpwnam() for details */
    if (pwd == NULL) {
        errno = EINVAL;
        perror(pName);
        exit(EXIT_FAILURE);
    }
    return pwd->pw_uid;
}

char *getGroupNameById(const uid_t uid) {
    struct group *grp = getgrgid(uid); /* don't free, see getgrnam() for details */
    if (grp == NULL) {
        perror(uid);
        exit(EXIT_FAILURE);
    }
    return grp->gr_name;
}

char *getUserNameById(const uid_t uid) {
    struct passwd *pwd = getpwuid(uid); /* don't free, see getpwnam() for details */
    if (pwd == NULL) {
        perror(uid);
        exit(EXIT_FAILURE);
    }
    return pwd->pw_name;
}

/****************************************************
 *  DEPTH
 *****************************************************/

bool isWithinCurrentDepth() {
    return (!is_option_set.mindepth && !is_option_set.maxdepth) ||
           (option_values.mindepth <= currentDepth && option_values.maxdepth >= currentDepth) ||
           (option_values.mindepth <= currentDepth && !is_option_set.maxdepth) ||
           (option_values.maxdepth >= currentDepth && !is_option_set.mindepth);
}

/**
 * block, char, dir, file, symlink, pipe, socket
 *
 * switch (sb.st_mode & S_IFMT) {
           case S_IFBLK:  printf("block device\n");            break;
           case S_IFCHR:  printf("character device\n");        break;
           case S_IFDIR:  printf("directory\n");               break;
           case S_IFIFO:  printf("FIFO/pipe\n");               break;
           case S_IFLNK:  printf("symlink\n");                 break;
           case S_IFREG:  printf("regular file\n");            break;
           case S_IFSOCK: printf("socket\n");                  break;
           default:       printf("unknown?\n");                break;
           }
 * @param pValue
 * @return
 */
int parseFileTypeArg(const char *pValue) {
    switch (pValue[0]) {
        case 'f':
            return DT_REG; // S_IFREG; // regular file
        case 'd':
            return DT_DIR; // S_IFDIR; // directory
        case 'l':
            return DT_LNK; // S_IFLNK; // symlink
        case 'p':
            return DT_FIFO; // S_IFIFO; // FIFO/pipe
        case 'b':
            return DT_BLK; // S_IFBLK; // block device
        case 'c':
            return DT_CHR; // S_IFCHR; // character device
        case 's':
            return DT_SOCK; // S_IFSOCK; // socket
        default:
            errno = EINVAL;
            perror(&pValue[0]);
            exit(EXIT_FAILURE);
    }
}

/****************************************************
 *  SIZE
 *****************************************************/

enum size_type parseSizePrefixArg(char *pValue) {
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

enum size_multiplier parseSizeSuffixArg(char *pValue) {
    int last = tolower(pValue[strlen(pValue) - 1]);
    if (last == 'g') {
        return GB;
    } else if (last == 'm') {
        return MB;
    } else if (last == 'k') {
        return KB;
    }
    return B;
}

void multiplySize(long *pSize, enum size_multiplier multiplier) {
    if (multiplier == GB) {
        *pSize = *pSize * 1024 * 1024 * 1024;
    } else if (multiplier == MB) {
        *pSize = *pSize * 1024 * 1024;
    } else if (multiplier == KB) {
        *pSize = *pSize * 1024;
    }
}

void divideSize(long *pSize, enum size_multiplier multiplier) {
    if (multiplier == GB) {
        *pSize = *pSize / 1024 / 1024 / 1024;
    } else if (multiplier == MB) {
        *pSize = *pSize / 1024 / 1024;
    } else if (multiplier == KB) {
        *pSize = *pSize / 1024;
    }
}

void printParams() {
    fprintf(stderr, "Is Option set:\n");
    fprintf(stderr, "icase = %d\n", is_option_set.case_insensitive);
    fprintf(stderr, "pattern = %d\n", is_option_set.pattern);
    fprintf(stderr, "type = %d\n", is_option_set.type);
    fprintf(stderr, "user = %d\n", is_option_set.user);
    fprintf(stderr, "group = %d\n", is_option_set.group);
    fprintf(stderr, "size = %d\n", is_option_set.size);
    fprintf(stderr, "mindepth = %d\n", is_option_set.mindepth);
    fprintf(stderr, "maxdepth = %d\n", is_option_set.maxdepth);
    fprintf(stderr, "delete = %d\n", is_option_set.delete);
    fprintf(stderr, "print0 = %d\n", is_option_set.print0);

    fprintf(stderr, "\nOption Values:\n");
    if (is_option_set.pattern) fprintf(stderr, "pattern = %s\n", option_values.pattern);
    if (is_option_set.type) fprintf(stderr, "type = %i\n", option_values.type);
    if (is_option_set.user) fprintf(stderr, "user = %d\n", option_values.user);
    if (is_option_set.group) fprintf(stderr, "group = %d\n", option_values.group);
    if (is_option_set.depth) fprintf(stderr, "depth = %d\n", option_values.depth);
    if (is_option_set.size) {
        fprintf(stderr, "size_type = %d\n", size_values.size_type);
        fprintf(stderr, "size = %ld\n", size_values.size);
        fprintf(stderr, "size_multiplier = %d\n", size_values.size_multiplier);
    }
    if (is_option_set.mindepth) fprintf(stderr, "mindepth = %d\n", option_values.mindepth);
    if (is_option_set.maxdepth) fprintf(stderr, "maxdepth = %d\n", option_values.maxdepth);
}

void usage(int status) {
    if (status != EXIT_SUCCESS)
        fprintf(stderr, "Try `%s --help' for more information.\n", "find");
    else {
        printf("Usage: [-id] [ptugUGsmMxyzf] [--print0] [--delete] [--help] [path...]\n\n");
        fputs("\
Search directories for matching files.\n\
Default: search \".\" match all -print all matches.\n\
\n\
", stdout);
        fputs("\
Filters:\n\
-i --icase       Case insensitive pattern\n\
-d --depth       Ignore content of dir\n\
-p --pattern     [PATTERN]  filename with wildcards\n\
-t --type        [bcdflps] (block, char, dir, file, symlink, pipe, socket)\n\
-u --user        [uid]    belongs to user UID \n\
-g --group       [uid]    belongs to user GID \n\
-U --user-name   [user-name]   belongs to user UNAME \n\
-G --group-name  [group-name]    belongs to user UNAME \n\
-s --size        [=><]N[kmg]    [equals, bigger, smaller or around][N=size][k=KB, m=MB, g=GB multiplier]\n\
-m --mindepth    # at least # dirs down\n\
-M --maxdepth    # at most # dirs down\n\
-x --atime N     Accessed N days ago\n\
-y --ctime N     Created N days ago\n\
-z --mtime N     Modified N days ago\n\
-f --format      Output specified FORMAT string instead of default file path\n\
\n\
", stdout);
        fputs("\
Actions:\n\
--print0    Print match with null\n\
--delete    Remove matching file/dir\n\
--help      Displays this message\n\
\n\
", stdout);
        fputs("\
Display status of files or filesystems:\n\
-f --format      Output specified FORMAT string instead of default file path\n\
\n\
The valid format escape sequences for files:\n\
%a  Access bits (octal) |%A  Access bits (flags)|%b  Size/512\n\
%B  Bytes per %b (512)  |%d  Device ID (dec)    |%D  Device ID (hex)\n\
%f  All mode bits (hex) |%F  File type          |%g  Group ID\n\
%G  Group name          |%h  Hard links         |%i  Inode\n\
%m  Mount point         |%n  Filename           |%N  Long filename\n\
%o  I/O block size      |%s  Size (bytes)       |%t  Devtype major (hex)\n\
%T  Devtype minor (hex) |%u  User ID            |%U  User name\n\
%x  Access time         |%X  Access unix time   |%y  File write time\n\
%Y  File write unix time|%z  Dir change time    |%Z  Dir change unix time\n\
\n\
", stdout);
        fputs("\
Date format:\n\
%a\tAbbreviated weekday name             Sun\n\
%A\tFull weekday name                    Sunday\n\
%b\tAbbreviated month name               Mar\n\
%B\tFull month name                      March\n\
%c\tDate and time representation         Sun Aug 19 02:56:02 2012\n\
%d\tDay of the month (01-31)             19\n\
%H\tHour in 24h format (00-23)           14\n\
%I\tHour in 12h format (01-12)           05\n\
%j\tDay of the year (001-366)            231\n\
%m\tMonth as a decimal number (01-12)    08\n\
%M\tMinute (00-59)                       55\n\
%p\tAM or PM designation                 PM\n\
%S\tSecond (00-61)                       02\n\
%U\tWeek number with the first Sunday \n\
  \tas the first day of week one (00-53) 33\n\
%w\tWeekday as a decimal number \n\
  \twith Sunday as 0 (0-6)               4\n\
%W\tWeek number with the first Monday \n\
  \tas the first day of week one (00-53) 34\n\
%x\tDate representation                  08/19/12\n\
%X\tTime representation                  02:50:06\n\
%y\tYear, last two digits (00-99)        01\n\
%Y\tYear                                 2012\n\
%Z\tTimezone name or abbreviation        CDT\n\
%%\tA % sign                             %\n\
\n\
", stdout);
        fputs("\
MORE TBD:\n\
-perm           [-/]MODE permissions (-=min /=any) \n\
-prune          ignore contents of dir\n\
-xdev           only this filesystem\n\
-newer FILE     newer mtime than FILE\n\
-depth          ignore contents of dir\n\
-inum  N        inode number N \n\
-empty          empty files and dirs\n\
", stdout);
    }
    exit(status);
}