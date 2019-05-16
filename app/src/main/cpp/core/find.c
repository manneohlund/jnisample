#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "fileutils.c"
#include "stringutils.c"
#include "callbacks.c"

struct dirent *pDirent;
struct stat filestat;
char file_modes[12];
char *result;

int find(char *pPath, char *name, callbacks *callbacks) {
    DIR *pDirectory;

    if ((pDirectory = opendir(pPath)) == NULL) {
        asprintf(&result, "can't open '%s'\n", pPath);
        callbacks->errorCallback(result);
        return EXIT_FAILURE;
    }

    while ((pDirent = readdir(pDirectory)) != NULL) {
        stat(pDirent->d_name, &filestat);
        mode_to_letter(filestat.st_mode, file_modes);

        asprintf(&result, "%s:%u:%u:%lld:%s/%s",
                 file_modes,
                 filestat.st_uid,
                 filestat.st_gid,
                 filestat.st_size,
                 pPath,
                 pDirent->d_name);

        if (ENDS_WITH_OR_CONTAINS_CASE_INSENSITIVE(pDirent->d_name, name) && !IS_FOLDER_POINTER(pDirent->d_name)) {
            callbacks->resultCallback(result);
        }

        free(result);

        if (pDirent->d_type == DT_DIR && !IS_FOLDER_POINTER(pDirent->d_name)) {
            asprintf(&result, "%s/%s", pPath, pDirent->d_name);
            find(result, name, callbacks);
        }
    }

    closedir(pDirectory);

    return EXIT_SUCCESS;
}

/*int main(int argc, char **argv) {
    char *pPath = argv[1];

    if (argc != 2) {
        fprintf(stderr, "usage: %s dir_name\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    print_headers();
    
    int result = ls(removeTrailingSlash(pPath), onStatResult);

    exit(result);
}*/