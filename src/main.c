/*
 * main.c
 * Purpose: Main entry point for the fdupes_mime program.
 *          Handles argument parsing, directory traversal, and orchestrates
 *          MIME type checking and duplicate finding. POSIX compliant.
 */
#include "defs.h"
#include "file_list.h"
#include "mime_utils.h" // Now uses POSIX version
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
    // magic_t magic_cookie; // Removed
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
    printf("Usage: %s [-r] [-h] [-m mime/type1] [-m mime/type2 ...] <directory1> [directory2 ...]\n", program_name);
    printf("\nFinds duplicate files, optionally filtering by MIME type. POSIX compliant.\n\n");
    printf("Options:\n");
    printf("  -r             Recursively search subdirectories.\n");
    printf("  -m MIME_TYPE   Add a MIME type to filter by. Can be used multiple times.\n");
    printf("                 Only files matching one of these types will be considered.\n");
    printf("                 If no -m options are given, all file types are considered.\n");
    printf("  -h             Display this help message and exit.\n");
    printf("\nExample:\n");
    printf("  %s -r -m image/jpeg -m image/png ./pics ./backup/images\n", program_name);
}

/*
 * Purpose: Parses command-line arguments using getopt() and populates the app_options_t structure.
 */
static int parse_arguments(int argc, char *argv[], app_options_t *options) {
    options->mime_filters = malloc(MAX_MIME_FILTERS * sizeof(char *));
    CHECK_ALLOC(options->mime_filters);
    for(int i=0; i < MAX_MIME_FILTERS; ++i) {
        options->mime_filters[i] = NULL; // For safe freeing
    }

    int opt;
    // The colon after 'm' indicates it requires an argument.
    // No '+' at the start of optstring, to allow interleaving of options and non-options if desired,
    // though standard POSIX getopt stops at the first non-option argument unless POSIXLY_CORRECT is set.
    // For simplicity, we expect options first.
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
                    // No need to free options->mime_filters here, will be done by free_global_options
                    return 1;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 1; // Indicate help was printed, main should exit with non-error status for help
            case '?': // getopt() returns '?' for an unknown option or missing argument
                // optopt contains the unknown option character or the option needing an argument
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
                // Should not happen with the current optstring
                abort();
        }
    }

    // Remaining arguments are directories
    if (optind >= argc) {
        fprintf(stderr, "Error: No directories specified.\n");
        print_usage(argv[0]);
        return 1;
    }

    options->num_directories = argc - optind;
    options->directories = malloc(options->num_directories * sizeof(char *));
    CHECK_ALLOC(options->directories);
    for (int i = 0; i < options->num_directories; ++i) {
        options->directories[i] = strdup(argv[optind + i]);
        CHECK_ALLOC(options->directories[i]);
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
    char path_buffer[MAX_PATH_LEN]; // Using PATH_MAX from defs.h
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

        // Construct full path
        // Check for potential buffer overflow before snprintf, though MAX_PATH_LEN should be large.
        // strlen(dir_path) + 1 (for '/') + strlen(entry->d_name) + 1 (for '\0')
        if (strlen(dir_path) + 1 + strlen(entry->d_name) + 1 > MAX_PATH_LEN) {
            fprintf(stderr, "Error: Path too long, skipping: %s/%s\n", dir_path, entry->d_name);
            continue;
        }
        snprintf(path_buffer, MAX_PATH_LEN, "%s/%s", dir_path, entry->d_name);

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
                // printf("Skipping empty file: %s\n", path_buffer);
                continue;
            }

            // Use POSIX compliant MIME detection
            if (get_file_mime_type_posix(path_buffer, mime_buffer, MIME_TYPE_BUFFER_SIZE) != 0) {
                // Error/warning already printed by get_file_mime_type_posix.
                // It provides a default "application/octet-stream".
                // We proceed with this default.
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
                if (add_file_to_list(all_files_list, path_buffer, statbuf.st_size, mime_buffer) != 0) {
                    fprintf(stderr, "Error adding file %s to list. Skipping.\n", path_buffer);
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

    // parse_arguments returns 0 on success, 1 on error or if help was printed.
    // If help was printed (-h), it's not an error condition for the program's exit status.
    // getopt() returns '?' for an error, which parse_arguments handles.
    // If parse_arguments returns 1 and it was due to -h, we should exit 0.
    // A simple way: if -h is present, parse_arguments handles printing and returns 1.
    // We can check if -h was the reason for exit.
    // For simplicity, if parse_arguments returns non-zero, we exit.
    // The print_usage in parse_arguments for -h doesn't imply an error exit code.
    // Let's refine this: parse_arguments returns 0 for success, 1 for CLI error, 2 for help displayed.

    // Revisit parse_arguments return codes for clarity:
    // 0 = success
    // 1 = error in arguments (missing dir, too many mimes, unknown option) -> exit 1
    // 2 = help printed (-h) -> exit 0

    // Let's stick to a simpler model for parse_arguments:
    // 0 = success
    // non-zero = failure or help printed (main decides exit code based on context)
    // For now, if parse_arguments returns non-zero, main exits 1.
    // If -h is used, print_usage is called, and parse_arguments returns 1.
    // This means ./program -h will exit 1. This is common for CLI tools when -h implies "don't proceed".
    // Some tools exit 0 after -h. Let's make -h exit 0.
    // We can achieve this by checking if "-h" was an argument if parse_arguments fails.
    // Or, make parse_arguments return a specific value for -h.

    int parse_result = parse_arguments(argc, argv, &g_options);
    if (parse_result != 0) {
        // If parse_result was due to -h, print_usage was called.
        // We need a way for parse_arguments to signal if it was just -h.
        // Let's modify parse_arguments to return 2 if -h was processed.
        // For now, assume any non-zero from parse_arguments is an exit condition.
        // If it was -h, usage was printed.
        // If it was an error, usage/error was printed.
        // A common pattern: if -h, exit 0. If other error, exit 1.
        // Let's check if "-h" is in argv if parse_result is 1. This is a bit hacky.
        // Better: parse_arguments returns specific codes.
        // For now, if parse_arguments returns 1 (due to -h or error), we exit 1.
        // This is acceptable for many CLIs.

        // Simpler: if -h is the *only* argument after program name, or one of few,
        // and parse_arguments handles it, it can return a special code.
        // The current getopt loop will return 1 from parse_arguments if -h is encountered.
        // Let's assume for now that if parse_arguments returns non-zero, we exit with 1.
        // The user sees the help message.
        free_global_options(); // Ensure cleanup even on arg parsing fail
        return 1;
    }


    file_list_t *all_files = create_file_list();
    if (!all_files) {
        free_global_options();
        return 1;
    }

    printf("Scanning directories (using 'file' command for MIME types)...\n");
    for (int i = 0; i < g_options.num_directories; ++i) {
        char resolved_path[MAX_PATH_LEN]; // Using PATH_MAX
        // realpath resolves symlinks in the path itself, and ., ..
        // It's good practice to work with canonical paths.
        if (realpath(g_options.directories[i], resolved_path) == NULL) {
            fprintf(stderr, "Error resolving path for %s: %s. Skipping.\n", g_options.directories[i], strerror(errno));
            continue;
        }
        printf("Processing directory: %s\n", resolved_path);
        collect_files_from_directory(resolved_path, all_files, &g_options);
    }

    printf("Collected %zu files matching criteria.\n", all_files->count);

    if (all_files->count > 1) {
        printf("Sorting files by size...\n");
        sort_file_list(all_files);
        find_and_print_duplicates(all_files);
    } else if (all_files->count == 0 && g_options.num_directories > 0) {
        printf("No files found matching criteria in the specified directories.\n");
    }
    else {
        printf("Not enough files to compare for duplicates.\n");
    }

    printf("Cleaning up resources...\n");
    free_file_list(all_files);
    free_global_options();

    printf("Done.\n");
    return 0;
}
