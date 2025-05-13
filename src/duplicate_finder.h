/*
 * duplicate_finder.h
 * Purpose: Defines functions for finding and reporting duplicate files.
 */
#ifndef DUPLICATE_FINDER_H
#define DUPLICATE_FINDER_H

#include "file_list.h"

/*
 * Purpose: Compares two files byte-by-byte to check for identical content.
 * Parameters:
 *   path1 - Path to the first file.
 *   path2 - Path to the second file.
 * Returns: 1 if files are identical, 0 if not, -1 on error.
 * Assumes files are of the same size.
 */
int compare_files_content(const char *path1, const char *path2);

/*
 * Purpose: Finds and prints duplicate files from the given list.
 *          The list is expected to be sorted by size.
 * Parameters:
 *   list - A pointer to a file_list_t containing file information.
 *          The list should be sorted by size before calling this function.
 */
void find_and_print_duplicates(file_list_t *list);

#endif // DUPLICATE_FINDER_H
