fdupes_mime - POSIX Compliant Duplicate File Finder with MIME Filtering

This program scans specified directories for duplicate files, with an option to filter files based on their MIME types. It aims for POSIX compliance.

Build Instructions:
-------------------
You will need `gcc` and `make`. The program uses the standard 'file' command for MIME type detection, which should be available on any POSIX system.

The executable will be created as `build/fdupes_mime`.
The build mode affects the compilation flags (e.g., debug symbols, optimizations).

To build in debug mode (default):
  make
  (or make MODE=debug)

To build in release mode:
  make MODE=release

To clean build artifacts:
  make clean

Usage:
------
  ./build/fdupes_mime [-r] [-h] [-m mime/type ...] [directory ...]

If no directories are specified, the current directory (.) is used by default.
Options and directory arguments can be provided in any order.

Options:
  -r                     Recursively search subdirectories.
  -m MIME_TYPE           Add a MIME type to filter by. Can be used multiple times.
                         Only files matching one of these types will be considered.
                         If no -m options are given, all file types are considered.
  -h                     Display this help message and exit.

Example Scenarios:
  make MODE=release
  ./build/fdupes_mime -r -m image/jpeg ./pictures ./archive/images
  ./build/fdupes_mime -m text/plain    # Scans current directory for text/plain files
  ./build/fdupes_mime dir1 -r -m application/pdf dir2 # Options and dirs interleaved

Notes:
------
- The program uses the 'file' command via popen() for MIME type detection.
- All file paths are resolved to their canonical absolute paths before comparison.
- Error checking is performed for system calls and memory allocation.
- Memory is managed dynamically and freed before exit.
