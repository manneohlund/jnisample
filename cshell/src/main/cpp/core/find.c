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
#include <libgen.h>
#include <strings.h>
#include "find.h"
#include "../utils/fileutils.c"
#include "../utils/stat-time.h"

int find(char *pPath) {
    currentDepth++;
    DIR *pDirectory;

    if ((pDirectory = opendir(pPath)) == NULL) {
        perror(pPath);
        return EXIT_FAILURE;
    } else if (currentDepth == 1 && isWithinCurrentDepth()) {
        statPath(pPath);
        if (validateFind()) {
            printFind();
        }
    }

    while ((pDirent = readdir(pDirectory)) != NULL) {
        if (IS_FOLDER_POINTER(pDirent->d_name)) {
            continue;
        }

        if (!is_option_set.depth && isWithinCurrentDepth()) {

            asprintf(&find_stat.pFullPath, "%s/%s", pPath, pDirent->d_name);
            find_stat.fileName = pDirent->d_name;
            statPath(find_stat.pFullPath);

            if (validateFind() == true) {
                printFind();
            }

            if (S_ISDIR(find_stat.fileStat.st_mode) && !IS_FOLDER_POINTER(find_stat.fileName)) {
                find(find_stat.pFullPath);
            }
        }
    }

    closedir(pDirectory);
    currentDepth--;

    return EXIT_SUCCESS;
}

bool validateFind() {
    // Type
    if (is_option_set.type && (find_stat.fileStat.st_mode & S_IFMT) != option_values.type)
        return false;

    // Size
    long size = find_stat.fileStat.st_size;
    if (is_option_set.size && !isSizeMatch(&size))
        return false;

    // User
    if (is_option_set.user && find_stat.fileStat.st_uid != option_values.user)
        return false;

    // Group
    if (is_option_set.group && find_stat.fileStat.st_gid != option_values.group)
        return false;

    // Pattern
    if (is_option_set.pattern && regexec(&regex, find_stat.fileName, 0, NULL, 0))
        return false;

    return true;
}

void statPath(char *pPath) {
    find_stat.pCurrentDir = dirname(pPath);

    if (lstat(find_stat.pFullPath, &find_stat.fileStat) == -1) {
        perror(find_stat.pFullPath);
        return;
    }

    if (S_ISLNK(find_stat.fileStat.st_mode)) {
        bzero(find_stat.linkPath, sizeof(find_stat.linkPath));
        readlink(find_stat.pFullPath, find_stat.linkPath, sizeof(find_stat.linkPath)-1);
    }
}

void printFind() {
    if (is_option_set.stat_format) {
        printStatFormat();
    } else {
        printf("%s%c", find_stat.pFullPath, is_option_set.print0 ? '\0' : '\n');
    }
}

bool isSizeMatch(long *pSize) {
    switch (size_values.size_type) {
        case EQ:
            divideSize(pSize, size_values.size_multiplier);
            multiplySize(pSize, size_values.size_multiplier);
            return *pSize == size_values.size;
        case GT:
            return *pSize >= size_values.size;
        case LT:
            return *pSize <= size_values.size;
        case CT:
        default:
            return *pSize <= (long) (size_values.size * 1.15) && *pSize >= (long) (size_values.size * 0.95);
    }
}

/****************************************************
 *  Stat format
 *****************************************************/

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

void printStatFormat() {
    char const *b;
    for (b = option_values.stat_format; *b; b++) {
        switch (*b) {
            case '%':
                b++;
                switch (*b) {
                    case 'a':
                        printf("%o", find_stat.fileStat.st_mode);                     // Access bits (octal)
                        break;
                    case 'A':
                        mode_to_letter(find_stat.fileStat.st_mode, find_stat.fileModes);
                        printf("%s", find_stat.fileModes);                            // File modes
                        break;
                    case 's':
                        printf("%lli", find_stat.fileStat.st_size);                   // Size
                        break;
                    case 'f':
                        printf("%x", find_stat.fileStat.st_mode);                     // All mode bits (hex)
                        break;
                    case 'F':
                        printf("%d", getFileType(find_stat.fileStat.st_mode & S_IFMT)); // File type
                        break;
                    case 'g':
                        printf("%u", find_stat.fileStat.st_gid);                      // Group id
                        break;
                    case 'G':
                        printf("%s", getGroupNameById(find_stat.fileStat.st_gid));    // Group name
                        break;
                    case 'i':
                        printf("%llu", find_stat.fileStat.st_ino);                    // Inode
                        break;
                    case 'n':
                        printf("%s", find_stat.fileName);                             // Filename
                        if (S_ISLNK(find_stat.fileStat.st_mode)) {
                            printf(" -> %s", find_stat.linkPath);
                        }
                        break;
                    case 'N':
                        printf("%s", find_stat.pFullPath);                            // Long filename
                        break;
                    case 'o':
                        printf("%lli", find_stat.fileStat.st_blocks);                 // I/O block size
                        break;
                    case 'p':
                        printf("%s", find_stat.pCurrentDir);                          // File path
                        break;
                    case 'u':
                        printf("%u", find_stat.fileStat.st_uid);                      // User id
                        break;
                    case 'U':
                        printf("%s", getUserNameById(find_stat.fileStat.st_uid));     // User name
                        break;
                    case 'x':
                        printDateFormat(get_stat_atime(&find_stat.fileStat).tv_sec);  // Access time
                        break;
                    case 'X':
                        printf("%li", get_stat_atime(&find_stat.fileStat).tv_sec);    // Access time unix
                        break;
                    case 'y':
                        printDateFormat(get_stat_mtime(&find_stat.fileStat).tv_sec);  // File mod/write time
                        break;
                    case 'Y':
                        printf("%li", get_stat_mtime(&find_stat.fileStat).tv_sec);    // File mod/write time unix
                        break;
                    case 'z':
                        printDateFormat(get_stat_ctime(&find_stat.fileStat).tv_sec);  // File change time
                        break;
                    case 'Z':
                        printf("%li", get_stat_ctime(&find_stat.fileStat).tv_sec);    // File change time unix
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

/****************************************************
 *  Date format
 *****************************************************/

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

void printDateFormat(time_t timer) {
    tm_info = localtime(&timer);
    strftime(buffer, 100, option_values.date_format, tm_info);
    printf("%s", buffer);
}

/****************************************************
 *  Parse
 *****************************************************/

int parseNumericArg(char *pValue) {
    char *ptr;

    /* Convert the provided value to a decimal long */
    long result = strtol(pValue, &ptr, 10);

    /* If the result is 0, test for an error */
    if (result == 0) {
        /* If a conversion error occurred, display a message and exit */
        if (errno == EINVAL) {
            perror(pValue);
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
 *  TYPE
 *****************************************************/

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

int parseFileTypeArg2(const char *pValue) {
    switch (pValue[0]) {
        case 'f':
            return S_IFREG; // regular file
        case 'd':
            return S_IFDIR; // directory
        case 'l':
            return S_IFLNK; // symlink
        case 'p':
            return S_IFIFO; // FIFO/pipe
        case 'b':
            return S_IFBLK; // block device
        case 'c':
            return S_IFCHR; // character device
        case 's':
            return S_IFSOCK; // socket
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
    switch (pValue[0]) {
        case '=':
            pValue[0] = '0';
            return EQ;
        case '+':
            pValue[0] = '0';
            return GT;
        case '-':
            pValue[0] = '0';
            return LT;
        default:
            return CT;
    }
}

enum size_multiplier parseSizeSuffixArg(char *pValue) {
    int last = tolower(pValue[strlen(pValue) - 1]);
    switch (last) {
        case 'g':
            return GB;
        case 'm':
            return MB;
        case 'k':
            return KB;
        default:
            return B;
    }
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

/****************************************************
 *  DEPTH
 *****************************************************/

bool isWithinCurrentDepth() {
    return (!is_option_set.mindepth && !is_option_set.maxdepth) ||
           (option_values.mindepth <= currentDepth && option_values.maxdepth >= currentDepth) ||
           (option_values.mindepth <= currentDepth && !is_option_set.maxdepth) ||
           (option_values.maxdepth >= currentDepth && !is_option_set.mindepth);
}

/****************************************************
 *  MAIN
 *****************************************************/

int main(int argc, char *argv[]) {
    int c;
    int option_index = 0;

    while ((c = getopt_long(argc, argv, "oidp:t:u:g:U:G:x:y:z:s:m:M:f:FD:", long_options, &option_index)) != -1) {
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
                option_values.type = parseFileTypeArg2(optarg);
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
                is_option_set.stat_format = true;
                option_values.stat_format = "%A %i %U %G %s %y %N";
                validateStatFormat();
                break;

            case 'D':
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

    // Check regex pattern
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

    // Check date format
    if (!is_option_set.date_format) {
        option_values.date_format = "%Y-%m-%d %X";
    }

    if (is_option_set.options) {
        printParams();
        exit(EXIT_SUCCESS);
    }

    // Check min and maxdepth
    if (!is_option_set.mindepth && !is_option_set.maxdepth && option_values.maxdepth < option_values.mindepth) {
        fprintf(stderr, "mindepth %d is larger than maxdepth %d", option_values.mindepth, option_values.maxdepth);
        exit(EXIT_FAILURE);
    }

    int result;

    // Execute find on path... or current working dir
    if (optind < argc) {
        while (optind < argc) {
            find_stat.pFullPath = argv[optind++];
            statPath(find_stat.pFullPath);
            find_stat.fileName = basename(find_stat.pFullPath);
            if (is_option_set.depth || !S_ISDIR(find_stat.fileStat.st_mode)) {
                printFind();
            } else {
                result = find(removeTrailingSlash(find_stat.pFullPath));
            }
        }
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

void printParams() {
    fprintf(stderr, "Is Option set:\n");
    fprintf(stderr, "icase = %d\n", is_option_set.case_insensitive);
    fprintf(stderr, "pattern = %d\n", is_option_set.pattern);
    fprintf(stderr, "type = %d\n", is_option_set.type);
    fprintf(stderr, "user = %d\n", is_option_set.user);
    fprintf(stderr, "group = %d\n", is_option_set.group);
    fprintf(stderr, "size = %d\n", is_option_set.size);
    fprintf(stderr, "stat-format = %d\n", is_option_set.stat_format);
    fprintf(stderr, "date-format = %d\n", is_option_set.date_format);
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
    if (is_option_set.stat_format) fprintf(stderr, "stat_format = %s\n", option_values.stat_format);
    if (is_option_set.date_format) fprintf(stderr, "date_format = %s\n", option_values.date_format);
    if (is_option_set.mindepth) fprintf(stderr, "mindepth = %d\n", option_values.mindepth);
    if (is_option_set.maxdepth) fprintf(stderr, "maxdepth = %d\n", option_values.maxdepth);
}

void usage(int status) {
    if (status != EXIT_SUCCESS)
        fprintf(stderr, "Try `%s --help' for more information.\n", "find");
    else {
        printf("Usage: [idptuUgGsmMxyzfFD] [--print0] [--delete] [--help] [path...]\n\n");
        fputs("\
Search directories for matching files.\n\
Default: search \".\" match all -print all matches.\n\
\n\
", stdout);
        fputs("\
Filters:\n\
-i --icase                   Case insensitive pattern\n\
-d --depth                   Ignore content of dir\n\
-p --pattern [PATTERN]       Regex patterns with wildcards\n\
-t --type [bcdflps]          File type (block, char, dir, file, symlink, pipe, socket)\n\
-u --user [UID]              Belongs to user UID \n\
-U --user-name [USER-NAME]   Belongs to user UNAME \n\
-g --group [GID]             Belongs to user GID \n\
-G --group-name [GROUP-NAME] Belongs to user UNAME \n\
-s --size [=+-][N][kmg]      [equals, bigger, smaller or around][N=size][k=KB, m=MB, g=GB multiplier]\n\
-m --mindepth [N]            At least N dirs down\n\
-M --maxdepth [N]            At most N dirs down\n\
-x --atime [N][smhdwmy]      Accessed N seconds, minutes, hours, days, weeks, months, years ago\n\
-y --ctime [N][smhdwmy]      Created N seconds, minutes, hours, days, weeks, months, years ago\n\
-z --mtime [N][smhdwmy]      Modified N seconds, minutes, hours, days, weeks, months, years ago\n\
-f --stat-format [FORMAT]    Output specified FORMAT string instead of default file stat\n\
-F --stat-default            Output default '%A %i %U %G %s %y %N' file stat format\n\
-D --date-format [FORMAT]    Output specified FORMAT string instead of default file date\n\
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
-f --stat-format [FORMAT]      Output specified FORMAT string for file stat\n\
-F --stat-default              Output default '%A %i %U %G %s %y %N' file stat format\n\
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
-D --date-format [FORMAT]     Output specified FORMAT string instead of default file date\n\
Default format is '%Y-%m-%d %X' (2019-05-02 14:35:00) \n\
\n\
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
-inum  N        inode number N \n\
-empty          empty files and dirs\n\
", stdout);
    }
    exit(status);
}