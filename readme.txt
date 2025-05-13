fdupes_mime - POSIX Compliant Duplicate File Finder with MIME Filtering

This program scans specified directories for duplicate files, with an option to filter files based on their MIME types. It aims for POSIX compliance.

Build Instructions:
-------------------
You will need `gcc` and `make`. The program uses the standard 'file' command for MIME type detection, which should be available on any POSIX system.

To build in debug mode:
  make MODE=debug
  The executable will be created as `build/debug/fdupes_mime`.

To build in release mode:
  make MODE=release
  The executable will be created as `build/release/fdupes_mime`.

To clean build artifacts:
  make clean

Usage:
------
Example for debug build:
  ./build/debug/fdupes_mime [-r] [-m mime/type1] [-m mime/type2] <directory1> [directory2 ...]

Example for release build:
  ./build/release/fdupes_mime [-r] [-m mime/type1] [-m mime/type2] <directory1> [directory2 ...]

Options:
  -r                     Recursively search subdirectories.
  -m MIME_TYPE           Add a MIME type to filter by. Can be used multiple times.
                         Only files matching one of these types will be considered.
                         If no -m options are given, all file types are considered.
  -h                     Display this help message and exit.

Example Scenario:
  make MODE=debug
  ./build/debug/fdupes_mime -r -m image/jpeg -m image/png ./pictures ./archive/images
  ./build/debug/fdupes_mime /home/user/documents

Notes:
------
- The program uses the 'file' command via popen() for MIME type detection.
- Error checking is performed for system calls and memory allocation.
- Memory is managed dynamically and freed before exit.
