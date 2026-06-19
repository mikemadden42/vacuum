# vacuum

A small command-line utility that tidies a directory by moving each file into a
subdirectory named after its extension.

Given a flat folder of mixed files, `vacuum` sorts them into per-extension
buckets:

```
.                         Documents/
├── report.pdf            ├── pdf/
├── notes.md       ==>    │   └── report.pdf
├── photo.jpg             ├── md/
└── photo2.jpg            │   └── notes.md
                          └── jpg/
                              ├── photo.jpg
                              └── photo2.jpg
```

This is a C17 port of an original Zig implementation.

## Usage

```
vacuum [source_dir] [dest_base]
```

| Argument     | Default       | Description                                          |
| ------------ | ------------- | ---------------------------------------------------- |
| `source_dir` | `.`           | Directory to scan for files.                         |
| `dest_base`  | `Documents`   | Base directory under which extension folders are made. |

For every **regular, non-hidden** file in `source_dir`, the file is moved to
`<dest_base>/<extension>/<filename>`. The destination directory is created if it
does not already exist.

### Behavior

- Hidden entries (names beginning with `.`), directories, and other non-regular
  files are skipped.
- Files without a usable extension (no `.`, or a trailing `.` only) are skipped.
- If a file with the same name already exists at the destination, it is left in
  place and a message is printed instead of overwriting.

> **Note:** Files are moved with `rename(2)`, so `source_dir` and `dest_base`
> should live on the same filesystem. Paths are resolved relative to the current
> working directory, so `vacuum` is normally run from the parent of
> `source_dir` (the default `.`).

### Examples

```sh
# Organize the current directory into ./Documents/<ext>/
vacuum

# Organize ./Downloads into ./sorted/<ext>/
vacuum Downloads sorted
```

Example output:

```
Moved report.pdf to Documents/pdf
Moved notes.md to Documents/md
File photo.jpg already exists in Documents/jpg
```

## Building

### With CMake

```sh
cmake -B build
cmake --build build
```

The resulting binary is `build/vacuum`. To install it:

```sh
cmake --install build            # honors CMAKE_INSTALL_PREFIX
```

### Directly with a compiler

```sh
cc -std=c17 -Wall -Wextra -o vacuum vacuum.c
```

## Requirements

- A C17 compiler (GCC or Clang).
- A POSIX environment (uses `dirent.h`, `stat`, `mkdir`, `rename`, `access`).

## License

See repository for license details.
