/*
 * defs.h
 * Purpose: Common definitions and includes for the fdupes_mime project.
 */
#ifndef DEFS_H
#define DEFS_H

#include <stdio.h>
#include <stdlib.h>    // For malloc, free, abort, exit, realpath
#include <string.h>    // For strcmp, strdup, strerror, snprintf, memcmp, etc.
#include <errno.h>     // For errno
#include <ctype.h>     // For isprint

// POSIX includes
#include <unistd.h>    // For getopt, read, close, etc.
#include <sys/types.h> // For off_t, dev_t, ino_t, etc.
#include <sys/stat.h>  // For struct stat, S_ISDIR, S_ISREG
#include <dirent.h>    // For DIR, struct dirent, opendir, readdir, closedir
// getopt.h is often included by unistd.h on POSIX systems, but explicit include is safer.
#include <getopt.h>    // For getopt (short options only for POSIX)
#include <limits.h>    // For PATH_MAX

#define MAX_PATH_LEN PATH_MAX
#define READ_BUFFER_SIZE 8192
#define MIME_CMD_BUFFER_SIZE (MAX_PATH_LEN + 128) // Increased slightly for "file -b --mime-type ''" + path

// Macro for handling allocation errors in non-interactive parts
#define CHECK_ALLOC(ptr) \
do { \
    if (!(ptr)) { \
        perror("Memory allocation failed"); \
        abort(); \
    } \
} while (0)

#endif // DEFS_H
