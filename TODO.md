# TODO

Tracked issues and improvements for `vacuum`. Roughly ordered by priority.

## Real gaps

- [ ] **Add a `LICENSE` file.** `README.md` says "See repository for license
      details" but no license exists. Either add one (MIT / Apache-2.0 / etc.)
      or remove that line from the README.
- [ ] **Add tests.** A shell-based test that sets up a temp directory, runs
      `vacuum`, and asserts the resulting tree would lock in the behaviors
      currently verified by hand: extension bucketing, dotfile / symlink /
      extensionless skipping, and the "already exists" path.
- [ ] **Add `--help` / argument validation.** `vacuum --help` currently calls
      `opendir("--help")` and errors confusingly. Print usage for `-h`/`--help`
      and reject obviously-wrong invocations (e.g. too many args).

## Robustness / design

- [ ] **Snapshot directory entries before processing.** Files are `rename()`d
      out of `source_dir` while `readdir()` is still iterating it. POSIX leaves
      it unspecified whether entries modified mid-iteration are returned (works
      in practice on Linux, but not guaranteed). Fix: collect all names first,
      close the iteration, then process the list.
- [ ] **Decide on continue-vs-abort error handling.** A single `mkdir`/`rename`
      failure currently `break`s and leaves remaining files unprocessed. For a
      batch tool, logging the failure and continuing (then exiting non-zero at
      the end) is usually friendlier. This is also how a cross-filesystem move
      (`EXDEV`) surfaces today — one such file kills the whole batch. A
      copy+unlink fallback for `EXDEV` could be considered.

## Minor

- [ ] **Check `printf` return values.** A stdout write error currently goes
      unnoticed.
- [ ] **Normalize trailing slash on `dest_base`.** `Documents/` yields a
      cosmetic `Documents//txt`. Harmless but ugly.
- [ ] **Add CI.** A GitHub Actions workflow building with both gcc and clang,
      once there is a remote. Nice-to-have, not essential.
