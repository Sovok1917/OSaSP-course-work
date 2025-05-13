/*
 * duplicate_finder.c
 * Purpose: Implements functions for finding and reporting duplicate files.
 */
#include "duplicate_finder.h"
#include <stdio.h>
#include <string.h> // For strerror, memcmp
#include <sys/stat.h>
#include <fcntl.h>    // For O_RDONLY
#include <unistd.h>   // For read, close
#include <errno.h>    // For errno

// Helper to print error messages with path context
// Moved definition before its first use.
static void perror_msg(const char *prefix, const char *path) {
    fprintf(stderr, "%s: %s: %s\n", prefix, path, strerror(errno));
}

int compare_files_content(const char *path1, const char *path2) {
    int fd1 = -1, fd2 = -1;
    // Using a static buffer for read can be problematic in multi-threaded contexts,
    // but for this single-threaded program, stack allocation is fine.
    char buffer1[READ_BUFFER_SIZE];
    char buffer2[READ_BUFFER_SIZE];
    ssize_t bytes_read1, bytes_read2;
    int result = 0; // 0 for different, 1 for identical

    fd1 = open(path1, O_RDONLY);
    if (fd1 < 0) {
        perror_msg("Error opening file for comparison", path1); // Now correctly declared
        return -1; // Error
    }

    fd2 = open(path2, O_RDONLY);
    if (fd2 < 0) {
        perror_msg("Error opening file for comparison", path2); // Now correctly declared
        if (close(fd1) < 0) { // Ensure fd1 is closed on error path
            perror_msg("Error closing file (on error path)", path1);
        }
        return -1; // Error
    }

    // Files are assumed to be of the same size by the calling logic
    while (1) {
        bytes_read1 = read(fd1, buffer1, READ_BUFFER_SIZE);
        if (bytes_read1 < 0) {
            perror_msg("Error reading from file", path1); // Now correctly declared
            result = -1; // Error
            break;
        }

        bytes_read2 = read(fd2, buffer2, READ_BUFFER_SIZE);
        if (bytes_read2 < 0) {
            perror_msg("Error reading from file", path2); // Now correctly declared
            result = -1; // Error
            break;
        }

        if (bytes_read1 != bytes_read2) {
            // This case implies an issue if sizes were pre-checked and identical,
            // or one file was modified during the check.
            fprintf(stderr, "Warning: Mismatch in bytes read for supposedly same-sized files: %s (%zd bytes) vs %s (%zd bytes)\n",
                    path1, bytes_read1, path2, bytes_read2);
            result = 0; // Different
            break;
        }

        if (bytes_read1 == 0) { // Both reached EOF simultaneously
            result = 1; // Identical
            break;
        }

        if (memcmp(buffer1, buffer2, bytes_read1) != 0) {
            result = 0; // Different
            break;
        }
    }

    // Cleanup file descriptors
    int close1_err = 0;
    int close2_err = 0;

    if (close(fd1) < 0) {
        perror_msg("Error closing file", path1); // Now correctly declared
        close1_err = 1;
    }
    if (close(fd2) < 0) {
        perror_msg("Error closing file", path2); // Now correctly declared
        close2_err = 1;
    }

    if (close1_err || close2_err) {
        // If files were deemed identical but a close operation failed,
        // it's safer to report an overall error for the comparison.
        if (result == 1) result = -1;
        // If result was already -1 (read error) or 0 (different), keep that.
    }

    return result;
}


void find_and_print_duplicates(file_list_t *list) {
    if (!list || list->count < 2) {
        // This message is fine, but main also prints a similar one. Consider consolidating.
        // printf("Not enough files to find duplicates.\n");
        return;
    }

    // The list is expected to be sorted by size by the caller (main).

    // printf("Searching for duplicates...\n"); // Message moved to main for better flow

    int duplicate_sets_found = 0;

    for (size_t i = 0; i < list->count; ++i) {
        if (list->items[i]->processed_for_duplicates) {
            continue;
        }

        // Find a block of files with the same size
        size_t block_start = i;
        size_t block_end = i; // Initialize block_end to block_start
        // Corrected loop condition: check block_end + 1 < list->count
        while (block_end + 1 < list->count && list->items[block_end + 1]->size == list->items[block_start]->size) {
            block_end++;
        }

        // Only proceed if there's more than one file in the size block
        if (block_end > block_start) {
            for (size_t j = block_start; j <= block_end; ++j) {
                if (list->items[j]->processed_for_duplicates) {
                    continue;
                }

                file_list_t *current_duplicate_set = create_file_list();
                if (!current_duplicate_set) {
                    fprintf(stderr, "Critical error: Could not create list for duplicate set. Aborting duplicate search.\n");
                    return;
                }

                // Add the base file for comparison to this potential set
                // No need to check return of add_file_to_list here as it aborts on critical alloc failure
                add_file_to_list(current_duplicate_set, list->items[j]->path, list->items[j]->size, list->items[j]->mime_type);
                list->items[j]->processed_for_duplicates = 1;


                for (size_t k = j + 1; k <= block_end; ++k) {
                    if (list->items[k]->processed_for_duplicates) {
                        continue;
                    }

                    // Ensure files are indeed the same size before content comparison (should be true due to outer block logic)
                    if (list->items[j]->size != list->items[k]->size) {
                        // This should ideally not happen if the outer block logic is correct
                        fprintf(stderr, "Internal logic error: File sizes differ within supposed same-size block: %s (%lld) vs %s (%lld)\n",
                                list->items[j]->path, (long long)list->items[j]->size,
                                list->items[k]->path, (long long)list->items[k]->size);
                        continue;
                    }

                    int comparison_result = compare_files_content(list->items[j]->path, list->items[k]->path);

                    if (comparison_result == 1) { // Files are identical
                        add_file_to_list(current_duplicate_set, list->items[k]->path, list->items[k]->size, list->items[k]->mime_type);
                        list->items[k]->processed_for_duplicates = 1;
                    } else if (comparison_result == -1) {
                        // Error message already printed by compare_files_content or its helper perror_msg
                        fprintf(stderr, "Skipping comparison between %s and %s due to error.\n", list->items[j]->path, list->items[k]->path);
                    }
                    // If comparison_result is 0, files are different, do nothing.
                }

                if (current_duplicate_set->count > 1) {
                    if (duplicate_sets_found == 0) { // Print header only once before first set
                        printf("\n--- Duplicate Sets Found ---\n");
                    }
                    duplicate_sets_found++;
                    printf("\nSet %d (Size: %lld bytes):\n", duplicate_sets_found, (long long)list->items[j]->size);
                    for (size_t l = 0; l < current_duplicate_set->count; ++l) {
                        printf("  %s\n", current_duplicate_set->items[l]->path);
                    }
                }
                free_file_list(current_duplicate_set); // Free this set's list
            }
        }
        // Move i to the end of the processed block to avoid redundant checks
        i = block_end;
    }

    if (duplicate_sets_found == 0 && list->count > 0) { // Only print if files were processed
        printf("No duplicate files found among the processed files.\n");
    } else if (duplicate_sets_found > 0) {
        printf("\n--- End of Duplicate Sets ---\n");
    }
    // If list->count was 0, main handles the "no files found" message.
}
