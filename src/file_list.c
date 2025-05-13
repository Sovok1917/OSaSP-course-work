/*
 * file_list.c
 * Purpose: Implements functions for managing a dynamic list of file_info_t structures.
 */
#include "file_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16

file_list_t *create_file_list(void) {
    file_list_t *list = malloc(sizeof(file_list_t));
    CHECK_ALLOC(list);

    list->items = malloc(INITIAL_CAPACITY * sizeof(file_info_t *));
    CHECK_ALLOC(list->items);

    list->count = 0;
    list->capacity = INITIAL_CAPACITY;
    return list;
}

int add_file_to_list(file_list_t *list, const char *path, off_t size, const char *mime_type) {
    if (!list) return -1;

    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity == 0 ? INITIAL_CAPACITY : list->capacity * 2;
        file_info_t **new_items = realloc(list->items, new_capacity * sizeof(file_info_t *));
        if (!new_items) { // realloc can return NULL even when reducing size
            perror("Failed to resize file list");
            // Original list->items is still valid if realloc fails
            return -1;
        }
        list->items = new_items;
        list->capacity = new_capacity;
    }

    file_info_t *new_file_info = malloc(sizeof(file_info_t));
    if (!new_file_info) {
        perror("Failed to allocate file_info_t");
        return -1;
    }

    new_file_info->path = strdup(path);
    if (!new_file_info->path) {
        perror("Failed to duplicate path string");
        free(new_file_info);
        return -1;
    }

    new_file_info->mime_type = strdup(mime_type);
    if (!new_file_info->mime_type) {
        perror("Failed to duplicate mime_type string");
        free(new_file_info->path);
        free(new_file_info);
        return -1;
    }

    new_file_info->size = size;
    new_file_info->is_duplicate_of_prev = 0;
    new_file_info->processed_for_duplicates = 0;

    list->items[list->count++] = new_file_info;
    return 0;
}

void free_file_list(file_list_t *list) {
    if (!list) return;

    for (size_t i = 0; i < list->count; ++i) {
        if (list->items[i]) {
            free(list->items[i]->path); // Path was strdup'd
            list->items[i]->path = NULL;
            free(list->items[i]->mime_type); // Mime type was strdup'd
            list->items[i]->mime_type = NULL;
            free(list->items[i]);
            list->items[i] = NULL;
        }
    }
    free(list->items);
    list->items = NULL;
    free(list);
}

// Comparison function for qsort
static int compare_file_info(const void *a, const void *b) {
    const file_info_t *file_a = *(const file_info_t **)a;
    const file_info_t *file_b = *(const file_info_t **)b;

    // Primary sort by size
    if (file_a->size < file_b->size) return -1;
    if (file_a->size > file_b->size) return 1;

    // Secondary sort by path (for stable grouping, optional but good)
    return strcmp(file_a->path, file_b->path);
}

void sort_file_list(file_list_t *list) {
    if (!list || list->count < 2) return;
    qsort(list->items, list->count, sizeof(file_info_t *), compare_file_info);
}
