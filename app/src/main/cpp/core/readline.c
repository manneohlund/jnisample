//
// Created by Manne Öhlund on 2019-04-16.
//

/* Copyright (C) 1991 Free Software Foundation, Inc.
This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

/* CHANGED FOR VMS */

/*
 * <getline.c>
**
** HISTORY:
**	 8 Jul 94  FM	Include "HTUtils.h" for memory allocation and free()
**			substitutions with VAXC on VMS.
**
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/* Read up to (and including) a newline from STREAM into *LINEPTR
   (and null-terminate it). *LINEPTR is a pointer returned from malloc (or
   NULL), pointing to *N characters of space.  It is realloc'd as
   necessary.  Returns the number of characters read (not including the
   null terminator), or -1 on error or EOF.  */

int readline2(char **lineptr, size_t *n, int *fd) {
    static char line[256];
    char *ptr;
    unsigned int len;

    if (lineptr == NULL || n == NULL) {
        errno = EINVAL;
        return -1;
    }

    read(*fd, line, 256);

    //ptr = strchr(line,'\n');
    //if (ptr) *ptr = '\0';

    len = strlen(line);

    if ((len + 1) < 256) {
        ptr = realloc(*lineptr, 256);
        if (ptr == NULL)
            return (-1);
        *lineptr = ptr;
        *n = 256;
    }

    strcpy(*lineptr, line);
    return (len);
}

ssize_t getdeliminator(char **buf, size_t *bufsiz, int delimiter, int *fd) {
    char *ptr, *eptr;


    if (*buf == NULL || *bufsiz == 0) {
        *bufsiz = BUFSIZ;
        if ((*buf = malloc(*bufsiz)) == NULL)
            return -1;
    }

    for (ptr = *buf, eptr = *buf + *bufsiz;;) {
        int c = read(fd, 1, 1);
        if (c == '\r') {
                return -1;
        }
        *ptr++ = c;
        if (c == delimiter) {
            *ptr = '\0';
            return ptr - *buf;
        }
        if (ptr + 2 >= eptr) {
            char *nbuf;
            size_t nbufsiz = *bufsiz * 2;
            ssize_t d = ptr - *buf;
            if ((nbuf = realloc(*buf, nbufsiz)) == NULL)
                return -1;
            *buf = nbuf;
            *bufsiz = nbufsiz;
            eptr = nbuf + nbufsiz;
            ptr = nbuf + d;
        }
    }
}

ssize_t readline(char **buf, size_t *bufsiz, int *fd) {
    return getdeliminator(buf, bufsiz, '\n', fd);
}