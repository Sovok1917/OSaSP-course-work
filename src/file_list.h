/*
 * file_list.h
 * Purpose: Defines structures and functions for managing a list of file information.
 */
#ifndef FILE_LIST_H
#define FILE_LIST_H

#include "defs.h"

typedef struct file_info_s {
    char *path;
    off_t size;
    char *mime_type;
    int is_duplicate_of_prev; // Flag used during duplicate finding
    int processed_for_duplicates; // Flag to avoid re-processing
} file_info_t;

typedef struct file_list_s {
    file_info_t **items;
    size_t count;
    size_t capacity;
} file_list_t;

/*
 * Purpose: Creates and initializes a new file list.
 * Parameters: None.
 * Returns: A pointer to the newly created file_list_t, or NULL on failure.
 *          The caller is responsible for freeing the list using free_file_list.
 */
file_list_t *create_file_list(void);

/*
 * Purpose: Adds a file_info_t item to the file list.
 *          The list will resize if necessary.
 * Parameters:
 *   list - The file list to add to.
 *   path - The path of the file.
 *   size - The size of the file.
 *   mime_type - The MIME type of the file.
 * Returns: 0 on success, -1 on failure (e.g., memory allocation).
 */
int add_file_to_list(file_list_t *list, const char *path, off_t size, const char *mime_type);

/*
 * Purpose: Frees all memory associated with the file list, including
 *          all file_info_t items and their string members.
 * Parameters:
 *   list - The file list to free.
 */
void free_file_list(file_list_t *list);

/*
 * Purpose: Sorts the file list by size (primary key) and then by path (secondary key).
 * Parameters:
 *  list - The file list to sort.
 */
void sort_file_list(file_list_t *list);


#endif // FILE_LIST_H
