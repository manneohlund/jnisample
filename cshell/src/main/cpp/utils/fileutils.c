#include <sys/stat.h>
#include <memory.h>
#include "fileutils.h"

#ifndef _FILEUTILS_
#define _FILEUTILS_

void mode_to_letter(int mode, char *pOut) {
    pOut[0] = S_ISDIR(mode) ? 'd'
            : S_ISLNK(mode) ? 'l'
            : S_ISCHR(mode) ? 'b'
            : S_ISBLK(mode) ? 'c'
            : S_ISSOCK(mode) ? 's'
            : S_ISFIFO(mode) ? 'f'
            : '-';

    // rwx user
    pOut[1] = IS_R_USR(mode) ? 'r' : '-';
    pOut[2] = IS_W_USR(mode) ? 'w' : '-';
    pOut[3] = IS_X_USR(mode) ? 'x' : '-';

    // rwx group
    pOut[4] = IS_R_GRP(mode) ? 'r' : '-';
    pOut[5] = IS_W_GRP(mode) ? 'w' : '-';
    pOut[6] = IS_X_GRP(mode) ? 'x' : '-';

    // rwx other
    pOut[7] = IS_R_OTH(mode) ? 'r' : '-';
    pOut[8] = IS_W_OTH(mode) ? 'w' : '-';
    pOut[9] = IS_X_OTH(mode) ? 'x' : '-';

    pOut[10] = '\0';
}

char parseFileTypeShort(int mode) {
    return (char) (mode == DT_DIR ? 'd'      // S_IFDIR; // directory
        : mode == DT_LNK ? 'l'          // S_IFLNK; // symlink
        : mode == DT_FIFO ? 'f'   // S_IFIFO; // FIFO/pipe
        : mode == DT_BLK ? 'b'    // S_IFBLK; // block device
        : mode == DT_CHR ? 'c'    // S_IFCHR; // character device
        : mode == DT_SOCK ? 's'   // S_IFSOCK; // socket
        : '-');                  // S_IFREG; // regular file
}

short getFileType(int pValue) {
    switch (pValue) {
        case S_IFDIR:
            return DT_DIR; // directory
        case S_IFLNK:
            return DT_LNK; // symlink
        case S_IFIFO:
            return DT_FIFO; // FIFO/pipe
        case S_IFBLK:
            return DT_BLK; // block device
        case S_IFCHR:
            return DT_CHR; // character device
        case S_IFSOCK:
            return DT_SOCK; // socket
        case S_IFREG:
        default:
            return DT_REG; // regular file
    }
}

char *removeTrailingSlash(char *source) {
    size_t len = strlen(source);
    if ((len > 0) && (source[len - 1] == '/'))
        source[len - 1] = '\0';
    return source;
}

#endif