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
vacuum -h | --help
```

| Argument     | Default       | Description                                          |
| ------------ | ------------- | ---------------------------------------------------- |
| `source_dir` | `.`           | Directory to scan for files.                         |
| `dest_base`  | `Documents`   | Base directory under which extension folders are made. |

`-h` / `--help` prints usage and exits.

For every **regular, non-hidden** file in `source_dir`, the file is moved to
`<dest_base>/<extension>/<filename>`. The destination directory is created if it
does not already exist.

### Behavior

- Hidden entries (names beginning with `.`), directories, and other non-regular
  files are skipped.
- Files without a usable extension (no `.`, or a trailing `.` only) are skipped.
- If a file with the same name already exists at the destination, it is left in
  place and a message is printed instead of overwriting.
- The directory listing is snapshotted before any moves, so files are never
  lost to mutating the directory mid-iteration.
- A failure to move one file is reported but does not stop the run; remaining
  files are still processed and the program exits non-zero.

- Files are moved with `rename(2)`; if that fails because the destination is on
  a different filesystem (`EXDEV`), `vacuum` falls back to copying the contents
  (preserving permission bits) and removing the original.

> **Note:** Paths are resolved relative to the current working directory, so
> `vacuum` is normally run from the parent of `source_dir` (the default `.`).

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

## Testing

The functional tests are a shell script that exercises the built binary:

```sh
ctest --test-dir build --output-on-failure   # via CMake
# or run it directly against any binary:
./tests/run.sh build/vacuum
```

## Requirements

- A C17 compiler (GCC or Clang).
- A POSIX environment (uses `dirent.h`, `stat`, `mkdir`, `rename`, `access`).

## License

Released under the [MIT License](LICENSE).
