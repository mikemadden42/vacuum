/*
 * vacuum - organize files into per-extension subdirectories.
 *
 * Usage: vacuum [source_dir] [dest_base]
 *   source_dir defaults to "."
 *   dest_base  defaults to "Documents"
 *
 * For each regular, non-hidden file in source_dir, the file is moved into
 * <dest_base>/<extension>/, creating the destination directory as needed.
 * If a file with the same name already exists at the destination it is left
 * in place.
 *
 * Port of the original Zig implementation to C17.
 */

#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * Join two path components with a '/' separator into a freshly allocated
 * string. The caller owns the returned buffer.
 */
static char *path_join(const char *a, const char *b) {
    size_t la = strlen(a);
    size_t lb = strlen(b);
    char *out = malloc(la + 1 + lb + 1);
    if (out == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    memcpy(out, a, la);
    out[la] = '/';
    memcpy(out + la + 1, b, lb);
    out[la + 1 + lb] = '\0';
    return out;
}

/*
 * Return a pointer to the extension of a file name, i.e. the last '.' in the
 * base name, including the dot. Returns NULL when there is no usable
 * extension (no dot, or a leading dot only).
 */
static const char *path_extension(const char *name) {
    const char *dot = strrchr(name, '.');
    if (dot == NULL || dot == name) {
        return NULL;
    }
    return dot;
}

/*
 * Recursively create a directory and all missing parents, like `mkdir -p`.
 * Returns 0 on success, -1 on failure (errno set).
 */
static int create_dir_path(const char *path) {
    char *buf = strdup(path);
    if (buf == NULL) {
        return -1;
    }

    for (char *p = buf + 1; *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(buf, 0777) != 0 && errno != EEXIST) {
                free(buf);
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(buf, 0777) != 0 && errno != EEXIST) {
        free(buf);
        return -1;
    }

    free(buf);
    return 0;
}

/*
 * Test whether a path refers to a regular file. Uses lstat() so that symbolic
 * links are not followed: a symlink is reported as not-a-regular-file and thus
 * skipped, matching the original Zig behavior of moving only true files.
 */
static int is_regular_file(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) {
        return 0;
    }
    return S_ISREG(st.st_mode);
}

int main(int argc, char **argv) {
    const char *source_dir = argc > 1 ? argv[1] : ".";
    const char *dest_base = argc > 2 ? argv[2] : "Documents";

    DIR *dir = opendir(source_dir);
    if (dir == NULL) {
        fprintf(stderr, "vacuum: cannot open '%s': %s\n", source_dir,
                strerror(errno));
        return EXIT_FAILURE;
    }

    int status = EXIT_SUCCESS;
    struct dirent *entry;
    errno = 0;
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;

        /* Skip hidden entries (including "." and ".."). */
        if (name[0] == '.') {
            errno = 0;
            continue;
        }

        char *src_path = path_join(source_dir, name);
        if (!is_regular_file(src_path)) {
            free(src_path);
            errno = 0;
            continue;
        }

        const char *ext = path_extension(name);
        if (ext == NULL || ext[1] == '\0') {
            free(src_path);
            errno = 0;
            continue;
        }
        const char *ext_name = ext + 1;

        char *dest_dir = path_join(dest_base, ext_name);
        if (create_dir_path(dest_dir) != 0) {
            fprintf(stderr, "vacuum: cannot create '%s': %s\n", dest_dir,
                    strerror(errno));
            free(dest_dir);
            free(src_path);
            status = EXIT_FAILURE;
            break;
        }

        char *dest_path = path_join(dest_dir, name);

        if (access(dest_path, F_OK) == 0) {
            printf("File %s already exists in %s\n", name, dest_dir);
        } else if (errno != ENOENT) {
            fprintf(stderr, "vacuum: cannot access '%s': %s\n", dest_path,
                    strerror(errno));
            status = EXIT_FAILURE;
            free(dest_path);
            free(dest_dir);
            free(src_path);
            break;
        } else if (rename(src_path, dest_path) != 0) {
            fprintf(stderr, "vacuum: cannot move '%s' to '%s': %s\n", src_path,
                    dest_path, strerror(errno));
            status = EXIT_FAILURE;
            free(dest_path);
            free(dest_dir);
            free(src_path);
            break;
        } else {
            printf("Moved %s to %s\n", name, dest_dir);
        }

        free(dest_path);
        free(dest_dir);
        free(src_path);
        errno = 0;
    }

    if (errno != 0 && status == EXIT_SUCCESS) {
        fprintf(stderr, "vacuum: error reading '%s': %s\n", source_dir,
                strerror(errno));
        status = EXIT_FAILURE;
    }

    closedir(dir);
    return status;
}
