/*
 * main.c
 * Purpose: Main entry point for the fdupes_mime program.
 *          Handles argument parsing, directory traversal, and orchestrates
 *          MIME type checking and duplicate finding. POSIX compliant.
 */
#include "defs.h"
#include "file_list.h"
#include "mime_utils.h"
#include "duplicate_finder.h"

#define MAX_MIME_FILTERS 100
#define MIME_TYPE_BUFFER_SIZE 256

// Global options structure
typedef struct app_options_s {
    char **directories;
    int num_directories;
    char **mime_filters;
    int num_mime_filters;
    int recursive;
} app_options_t;

// Static global for options, initialized at runtime
static app_options_t g_options;

/*
 * Purpose: Initializes global static memory (g_options).
 */
static void initialize_global_options(void) {
    g_options.directories = NULL;
    g_options.num_directories = 0;
    g_options.mime_filters = NULL;
    g_options.num_mime_filters = 0;
    g_options.recursive = 0;
}

/*
 * Purpose: Frees memory allocated for global options.
 */
static void free_global_options(void) {
    if (g_options.directories) {
        for (int i = 0; i < g_options.num_directories; ++i) {
            free(g_options.directories[i]);
        }
        free(g_options.directories);
        g_options.directories = NULL;
    }
    if (g_options.mime_filters) {
        for (int i = 0; i < g_options.num_mime_filters; ++i) {
            free(g_options.mime_filters[i]);
        }
        free(g_options.mime_filters);
        g_options.mime_filters = NULL;
    }
}

/*
 * Purpose: Prints usage information for the program.
 */
static void print_usage(const char *program_name) {
    // Updated usage to reflect default directory behavior
    printf("Usage: %s [-r] [-h] [-m mime/type ...] [directory ...]\n", program_name);
    printf("\nFinds duplicate files, optionally filtering by MIME type. POSIX compliant.\n");
    printf("If no directories are specified, the current directory (.) is used.\n\n");
    printf("Options:\n");
    printf("  -r             Recursively search subdirectories.\n");
    printf("  -m MIME_TYPE   Add a MIME type to filter by. Can be used multiple times.\n");
    printf("                 Only files matching one of these types will be considered.\n");
    printf("                 If no -m options are given, all file types are considered.\n");
    printf("  -h             Display this help message and exit.\n");
    printf("\nExamples:\n");
    printf("  %s -r -m image/jpeg ./pics ./backup/images\n", program_name);
    printf("  %s -m text/plain    (scans current directory for text/plain files)\n", program_name);
    printf("  %s dir1 -r dir2 -m application/pdf\n", program_name);
}

/*
 * Purpose: Parses command-line arguments using getopt() and populates the app_options_t structure.
 *          Allows options and directories to be interleaved.
 *          Defaults to current directory if no directories are specified.
 */
static int parse_arguments(int argc, char *argv[], app_options_t *options) {
    options->mime_filters = malloc(MAX_MIME_FILTERS * sizeof(char *));
    CHECK_ALLOC(options->mime_filters);
    for(int i=0; i < MAX_MIME_FILTERS; ++i) {
        options->mime_filters[i] = NULL; // For safe freeing
    }

    int opt;
    // optstring "rm:h" - getopt will permute argv to collect non-options at the end.
    while ((opt = getopt(argc, argv, "rm:h")) != -1) {
        switch (opt) {
            case 'r':
                options->recursive = 1;
                break;
            case 'm':
                if (options->num_mime_filters < MAX_MIME_FILTERS) {
                    options->mime_filters[options->num_mime_filters] = strdup(optarg);
                    CHECK_ALLOC(options->mime_filters[options->num_mime_filters]);
                    options->num_mime_filters++;
                } else {
                    fprintf(stderr, "Error: Exceeded maximum number of MIME type filters (%d).\n", MAX_MIME_FILTERS);
                    return 1; // Error exit
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 2; // Special return code for help displayed
            case '?':
                if (optopt == 'm') {
                    fprintf(stderr, "Error: Option -%c requires an argument.\n", optopt);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "Error: Unknown option `-%c'.\n", optopt);
                } else {
                    fprintf(stderr, "Error: Unknown option character `\\x%x'.\n", optopt);
                }
                print_usage(argv[0]);
                return 1; // Error exit
            default:
                abort();
        }
    }

    // After getopt, optind is the index of the first non-option argument.
    if (optind >= argc) {
        // No directory arguments provided, default to current directory "."
        options->num_directories = 1;
        options->directories = malloc(options->num_directories * sizeof(char *));
        CHECK_ALLOC(options->directories);
        options->directories[0] = strdup(".");
        CHECK_ALLOC(options->directories[0]);
    } else {
        // Directory arguments are present
        options->num_directories = argc - optind;
        options->directories = malloc(options->num_directories * sizeof(char *));
        CHECK_ALLOC(options->directories);
        for (int i = 0; i < options->num_directories; ++i) {
            options->directories[i] = strdup(argv[optind + i]);
            CHECK_ALLOC(options->directories[i]);
        }
    }
    return 0; // Success
}

/*
 * Purpose: Recursively walks a directory, collects file information,
 *          filters by MIME type, and adds to the file list.
 */
static void collect_files_from_directory(const char *dir_path, file_list_t *all_files_list, const app_options_t *options) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char path_buffer[MAX_PATH_LEN];
    char mime_buffer[MIME_TYPE_BUFFER_SIZE];

    dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Error opening directory %s: %s\n", dir_path, strerror(errno));
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct full path using snprintf and check its return value for truncation
        int required_len = snprintf(path_buffer, MAX_PATH_LEN, "%s/%s", dir_path, entry->d_name);

        if (required_len < 0) {
            // An encoding error occurred with snprintf
            fprintf(stderr, "Error: snprintf encoding error while constructing path for %s/%s. Skipping.\n", dir_path, entry->d_name);
            continue;
        }
        if ((size_t)required_len >= MAX_PATH_LEN) {
            // The path was truncated by snprintf
            fprintf(stderr, "Error: Path too long, would truncate, skipping: %s/%s (requires %d, buffer %d)\n",
                    dir_path, entry->d_name, required_len, MAX_PATH_LEN);
            continue;
        }
        // If we reach here, path_buffer contains the full, null-terminated path.

        if (lstat(path_buffer, &statbuf) == -1) {
            fprintf(stderr, "Error stating file %s: %s. Skipping.\n", path_buffer, strerror(errno));
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            if (options->recursive) {
                collect_files_from_directory(path_buffer, all_files_list, options);
            }
        } else if (S_ISREG(statbuf.st_mode)) {
            if (statbuf.st_size == 0) {
                continue;
            }

            if (get_file_mime_type_posix(path_buffer, mime_buffer, MIME_TYPE_BUFFER_SIZE) != 0) {
                // Proceed with default MIME type
            }

            int mime_match = 0;
            if (options->num_mime_filters == 0) {
                mime_match = 1;
            } else {
                for (int i = 0; i < options->num_mime_filters; ++i) {
                    if (strcmp(mime_buffer, options->mime_filters[i]) == 0) {
                        mime_match = 1;
                        break;
                    }
                }
            }

            if (mime_match) {
                char resolved_item_path[MAX_PATH_LEN];
                if (realpath(path_buffer, resolved_item_path) == NULL) {
                    fprintf(stderr, "Error resolving path for item %s: %s. Skipping.\n", path_buffer, strerror(errno));
                    continue;
                }
                if (add_file_to_list(all_files_list, resolved_item_path, statbuf.st_size, mime_buffer) != 0) {
                    fprintf(stderr, "Error adding file %s to list. Skipping.\n", resolved_item_path);
                }
            }
        }
    }

    if (closedir(dir) == -1) {
        fprintf(stderr, "Error closing directory %s: %s\n", dir_path, strerror(errno));
    }
}


int main(int argc, char *argv[]) {
    initialize_global_options();

    int parse_result = parse_arguments(argc, argv, &g_options);
    if (parse_result == 2) { // Help was displayed
        free_global_options();
        return 0; // Exit successfully after help
    }
    if (parse_result == 1) { // Argument error
        free_global_options();
        return 1; // Exit with error status
    }
    // parse_result == 0 means success

    file_list_t *all_files = create_file_list();
    if (!all_files) {
        free_global_options();
        return 1;
    }

    //printf("Scanning directories (using 'file' command for MIME types)...\n");
    for (int i = 0; i < g_options.num_directories; ++i) {
        char resolved_dir_path[MAX_PATH_LEN];
        // Resolve the top-level directory path once
        if (realpath(g_options.directories[i], resolved_dir_path) == NULL) {
            fprintf(stderr, "Error resolving path for input directory %s: %s. Skipping.\n", g_options.directories[i], strerror(errno));
            continue;
        }
        //printf("Processing directory: %s\n", resolved_dir_path);
        // Pass the already resolved directory path to collect_files_from_directory
        collect_files_from_directory(resolved_dir_path, all_files, &g_options);
    }

    //printf("Collected %zu files matching criteria.\n", all_files->count);

    if (all_files->count > 1) {
        //printf("Sorting files by size...\n");
        sort_file_list(all_files);
        find_and_print_duplicates(all_files);
    } else if (all_files->count == 0 && g_options.num_directories > 0) {
        // Check if any directories were actually processed (e.g. not all skipped due to realpath errors)
        int dirs_processed_successfully = 0;
        for (int i = 0; i < g_options.num_directories; ++i) {
            char temp_path[MAX_PATH_LEN];
            if (realpath(g_options.directories[i], temp_path) != NULL) {
                dirs_processed_successfully++;
                break;
            }
        }
        if (dirs_processed_successfully > 0) {
            printf("No files found matching criteria in the specified valid directories.\n");
        } else if (g_options.num_directories > 0) {
            printf("No valid directories could be processed.\n");
        }
        // If num_directories was 0 initially (and defaulted to "."), this message might not be ideal.
        // However, g_options.num_directories will be 1 if defaulted to ".".
    } else { // all_files.count is 0 or 1
        printf("Not enough files to compare for duplicates, or no files found.\n");
    }

    //printf("Cleaning up resources...\n");
    free_file_list(all_files);
    free_global_options();

    //printf("Done.\n");
    return 0;
}
