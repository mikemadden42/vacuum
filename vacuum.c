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

static const char *PROG = "vacuum";

/* A growable list of owned strings. */
typedef struct {
    char **items;
    size_t len;
    size_t cap;
} StrList;

/* Append a freshly duplicated copy of `s` to the list. Aborts on OOM. */
static void strlist_push(StrList *list, const char *s) {
    if (list->len == list->cap) {
        size_t cap = list->cap ? list->cap * 2 : 16;
        char **items = realloc(list->items, cap * sizeof(*items));
        if (items == NULL) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        list->items = items;
        list->cap = cap;
    }
    char *copy = strdup(s);
    if (copy == NULL) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    list->items[list->len++] = copy;
}

static void strlist_free(StrList *list) {
    for (size_t i = 0; i < list->len; i++) {
        free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->len = list->cap = 0;
}

/*
 * Join two path components with a '/' separator into a freshly allocated
 * string. A trailing '/' on `a` is not duplicated. The caller owns the result.
 */
static char *path_join(const char *a, const char *b) {
    size_t la = strlen(a);
    size_t lb = strlen(b);
    int need_sep = (la == 0 || a[la - 1] != '/');
    char *out = malloc(la + (need_sep ? 1 : 0) + lb + 1);
    if (out == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    memcpy(out, a, la);
    size_t pos = la;
    if (need_sep) {
        out[pos++] = '/';
    }
    memcpy(out + pos, b, lb);
    out[pos + lb] = '\0';
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

static void usage(FILE *out) {
    fprintf(out,
            "Usage: %s [source_dir] [dest_base]\n"
            "\n"
            "Organize regular files in source_dir into\n"
            "<dest_base>/<extension>/ subdirectories.\n"
            "\n"
            "  source_dir   directory to scan        (default \".\")\n"
            "  dest_base    base for extension dirs  (default \"Documents\")\n"
            "\n"
            "Options:\n"
            "  -h, --help   show this help and exit\n",
            PROG);
}

/*
 * Read every entry name in `source_dir` into `names`. Returns 0 on success,
 * -1 on failure (a diagnostic is printed).
 */
static int collect_entries(const char *source_dir, StrList *names) {
    DIR *dir = opendir(source_dir);
    if (dir == NULL) {
        fprintf(stderr, "%s: cannot open '%s': %s\n", PROG, source_dir,
                strerror(errno));
        return -1;
    }

    int rc = 0;
    struct dirent *entry;
    errno = 0;
    while ((entry = readdir(dir)) != NULL) {
        strlist_push(names, entry->d_name);
        errno = 0;
    }
    if (errno != 0) {
        fprintf(stderr, "%s: error reading '%s': %s\n", PROG, source_dir,
                strerror(errno));
        rc = -1;
    }

    closedir(dir);
    return rc;
}

int main(int argc, char **argv) {
    const char *positional[2];
    size_t npos = 0;

    /* Argument parsing. */
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            usage(stdout);
            return EXIT_SUCCESS;
        }
        if (arg[0] == '-' && arg[1] != '\0') {
            fprintf(stderr, "%s: unknown option '%s'\n", PROG, arg);
            usage(stderr);
            return EXIT_FAILURE;
        }
        if (npos >= 2) {
            fprintf(stderr, "%s: too many arguments\n", PROG);
            usage(stderr);
            return EXIT_FAILURE;
        }
        positional[npos++] = arg;
    }

    const char *source_dir = npos > 0 ? positional[0] : ".";
    const char *dest_base = npos > 1 ? positional[1] : "Documents";

    /* Snapshot entries first so we don't mutate the directory mid-iteration. */
    StrList names = {0};
    if (collect_entries(source_dir, &names) != 0) {
        strlist_free(&names);
        return EXIT_FAILURE;
    }

    int had_error = 0;
    for (size_t i = 0; i < names.len; i++) {
        const char *name = names.items[i];

        /* Skip hidden entries (including "." and ".."). */
        if (name[0] == '.') {
            continue;
        }

        char *src_path = path_join(source_dir, name);
        if (!is_regular_file(src_path)) {
            free(src_path);
            continue;
        }

        const char *ext = path_extension(name);
        if (ext == NULL || ext[1] == '\0') {
            free(src_path);
            continue;
        }
        const char *ext_name = ext + 1;

        char *dest_dir = path_join(dest_base, ext_name);
        char *dest_path = path_join(dest_dir, name);

        if (create_dir_path(dest_dir) != 0) {
            fprintf(stderr, "%s: cannot create '%s': %s\n", PROG, dest_dir,
                    strerror(errno));
            had_error = 1;
        } else if (access(dest_path, F_OK) == 0) {
            printf("File %s already exists in %s\n", name, dest_dir);
        } else if (errno != ENOENT) {
            fprintf(stderr, "%s: cannot access '%s': %s\n", PROG, dest_path,
                    strerror(errno));
            had_error = 1;
        } else if (rename(src_path, dest_path) != 0) {
            fprintf(stderr, "%s: cannot move '%s' to '%s': %s\n", PROG,
                    src_path, dest_path, strerror(errno));
            had_error = 1;
        } else {
            printf("Moved %s to %s\n", name, dest_dir);
        }

        free(dest_path);
        free(dest_dir);
        free(src_path);
    }

    strlist_free(&names);

    /* Surface any deferred stdout write error. */
    if (fflush(stdout) != 0 || ferror(stdout)) {
        fprintf(stderr, "%s: error writing to stdout\n", PROG);
        had_error = 1;
    }

    return had_error ? EXIT_FAILURE : EXIT_SUCCESS;
}
