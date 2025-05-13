/*
 * mime_utils.h
 * Purpose: Defines functions for POSIX-compliant MIME type detection
 *          by calling the 'file' command-line utility.
 */
#ifndef MIME_UTILS_H
#define MIME_UTILS_H

#include "defs.h" // For size_t, MAX_PATH_LEN

/*
 * Purpose: Gets the MIME type of a specified file using the 'file' command.
 * Parameters:
 *   filepath - Path to the file.
 *   mime_buffer - Buffer to store the resulting MIME type string.
 *   buffer_size - Size of the mime_buffer.
 * Returns: 0 on success (MIME type in mime_buffer), -1 on error or if 'file' command fails.
 *          On failure, mime_buffer might contain a default or be empty.
 */
int get_file_mime_type_posix(const char *filepath, char *mime_buffer, size_t buffer_size);

#endif // MIME_UTILS_H
