# TODO

Tracked issues and improvements for `vacuum`.

## Done

- [x] **Add a `LICENSE` file.** MIT license added; README now links to it.
- [x] **Add tests.** Shell-based functional tests in `tests/run.sh`, wired into
      CTest and CI. Covers extension bucketing, dotfile / symlink / extensionless
      skipping, multi-dot extensions, the "already exists" path, custom
      `dest_base`, trailing-slash handling, and CLI argument handling.
- [x] **Add `--help` / argument validation.** `-h`/`--help` prints usage;
      unknown options and too many arguments are rejected with usage.
- [x] **Snapshot directory entries before processing.** Entries are collected
      into a list and the directory is closed before any moves, so iteration is
      never affected by `rename()`-ing files out of `source_dir`.
- [x] **Continue-vs-abort error handling.** A failed `mkdir`/`rename`/`access`
      is now reported but does not stop the run; remaining files are processed
      and the program exits non-zero if any failure occurred.
- [x] **Check stdout write errors.** `fflush` + `ferror(stdout)` are checked at
      exit and surface as a failure.
- [x] **Normalize trailing slash on `dest_base`.** `path_join` no longer
      duplicates a separator when the left side already ends in `/`.
- [x] **Add CI.** `.github/workflows/ci.yml` builds with gcc and clang and runs
      the test suite via `ctest`.

## Remaining (optional)

- [ ] **`EXDEV` (cross-filesystem) fallback.** A move across filesystems still
      fails for that file (now reported and skipped rather than aborting). A
      copy + unlink fallback would let `vacuum` work across mount points.
