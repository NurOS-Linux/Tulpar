# Changelog

All notable changes to tulpar are documented in this file.

## [Unreleased]

### Added

- `tulpar help [command]` shows the usage of tulpar or of one command.

### Changed

- Without root the cache defaults to `$XDG_CACHE_HOME/apg`
  (`~/.cache/apg`) instead of `/var/cache/apg`, so searching, downloading
  and installing from repositories work for an unprivileged user.
- `key list` prints the names of the trusted keys instead of pointing at
  the keyring directory.

### Fixed

- An option that needs a value but got none, such as `--dest` at the end
  of the command line, is reported as missing its value instead of as an
  unknown option.
- `history` says that no entries match the filters, rather than that
  nothing is recorded, and `list --names --json` is rejected instead of
  ignoring `--json`.
- `hold`, `mark`, `upgrade` and `reinstall` use the same "package is not
  installed" message as `remove`, and a package that is not found in the
  repositories comes with a hint for the next step.
- `repo` shows only the usage of the sub-action that was misused.
- The orphan count uses the right plural form.

## [0.6.0] - 2026-11-10

### Added

- `verify --db` also checks the package database against itself and
  reports package records that cannot be read, file lists and ownership
  entries that do not match the installed packages, and listed files
  that have no owner or are owned by another package. Backed by
  libapg's new `db_check()`.
- `require_script_sandbox` option in `tulpar.conf`: when true, a package
  script that cannot be run in isolated namespaces fails the
  transaction, which is rolled back, instead of running without
  isolation. A hint names the option when this happens. Backed by
  libapg's new `trans_set_require_sandbox()`.
- `install` and `upgrade` compare the free space of the target
  filesystem with libapg's estimate of what the transaction needs
  before asking for confirmation, and warn with both sizes when it is
  not enough, then ask again. `--yes` continues without asking and
  `--dry-run` only warns. The estimate is conservative. Backed by
  libapg's new `trans_estimate_space()`.

- `owns <path>...` finds the package that owns a file, `files <name>`
  lists the files of an installed package, and `why <name>` explains
  why a package is installed, with the packages that require it and the
  chain from an explicitly installed package. `stats` summarises the
  database.
- `mark --auto|--explicit <name>...` switches packages between
  installed as a dependency and installed explicitly, which decides
  what `orphans` may remove.
- `reinstall <package|file.apg>...` installs the installed version of a
  package again to restore damaged files, from the repositories or from
  an archive, keeping the explicit or dependency state.
- `info` accepts the path of a `.apg` file and shows its metadata
  without installing it.
- `list` gains `--explicit`, `--deps`, `--held`, `--upgradable` and
  `--names`; `--upgradable` asks the repositories once per installed
  package.
- `history` gains `--package <name>` and `--since <spec>`, where the
  spec is an age such as `7d`, a date, or `@<unix time>`.
- `repo enable`, `repo disable` and `repo priority` manage repositories
  without deleting them, and `repo list` gains `--all` and `--json`. A
  disabled entry is stored as a `#disabled` line, which older versions
  of tulpar ignore, and a priority as `priority=N` after the URL, which
  older versions would read as part of the URL. Repositories are
  consulted in ascending priority, 100 by default.
- `--offline` and the `offline` option in `tulpar.conf` stop tulpar from
  touching the network: repository indexes come from the cache however
  old it is, and commands that need the network fail with a clear
  message.
- Shell completions suggest installed package names, repository URLs and
  `.apg` files, and cover all new commands and options.
- The JSON output of every command is documented in the new
  `tulpar-json(7)` manual page and covered by unit tests.

### Changed

- `orphans` and `autoremove` list and remove whole chains of orphaned
  packages in one run, including packages that only other orphans
  depend on, instead of only the packages that nothing depends on. Uses
  libapg's new `db_get_orphans_transitive()`.
- Package archives are read without being unpacked first, so resolving
  and planning an installation no longer spends time and temporary disk
  space proportional to the size of the package. In libapg's own
  measurement, installing a package of 200 MB went from 0.745 s to
  0.509 s. This comes with libapg 2.8.0.
- libapg pinned to v2.8.0 (`b388fe9`).

### Fixed

- Unreadable, truncated or path-traversing package archives, whether
  given as a file, downloaded from a repository or a URL, or built from
  a git repository, are rejected with "is not a valid package" before the
  plan is shown. libapg 2.8.0 no longer unpacks an archive while reading
  it, so tulpar now reads the whole archive itself to keep catching
  these.

## [0.5.0] - 2026-09-30

### Added

- `--color auto|always|never` global flag. Colors are decided separately
  for stdout and stderr and honor `NO_COLOR`, `CLICOLOR_FORCE`, and
  `TERM=dumb`; documented in a new ENVIRONMENT section of `tulpar(1)`.
- Status lines with right-aligned verbs (`Resolved`, `Downloading`,
  `Installing [i/n]`, `Upgrading`, `Removing`, `Finished … in 0.42s`)
  and a download progress bar with size, speed, and item count on
  stderr. The bar is only drawn when stderr is a terminal and is hidden
  by `-q`.
- `help:` hints under errors, for example for a locked database, a held
  package, dependents blocking a removal, a failed signature check, a
  full disk while unpacking, or a missing repository list.
- Suggestions for mistyped commands and options (`did you mean
  'install'?`, `did you mean '--dry-run'?`).
- Parallel downloads: `install` resolves dependencies level by level and
  downloads each level at once, and `upgrade` without arguments looks up
  every update first and downloads them together. The number of
  concurrent transfers comes from `max_parallel_downloads`.
- `--nodeps` now reports what it skips: `install --nodeps` names each
  missing dependency and the package that needs it, and
  `remove --nodeps` names every installed package that depends on a
  removed one.
- `install` of an already installed package upgrades it when a newer
  version is available from the repositories, a local archive, or a URL,
  reports it as already installed when the version is the same, and
  skips it with a warning when the installed version is newer. An
  explicit `install` of a package that was installed as a dependency
  marks it as installed explicitly, backed by libapg's new
  `db_set_installed_by_hand()`.

### Changed

- libapg pinned to v2.7.0 (`b9d424b`).
- The transaction plan is a table with `old -> new` versions for
  upgrades, the installed version for removals, and a summary line; an
  empty plan no longer asks for confirmation.
- `list` and `search` print aligned tables with `held` and `installed`
  tags, `info` prints a translated field list with the package status and
  marks dependencies that are not installed, `history` prints a table
  with timestamps, and the remaining commands use status lines instead of
  `==>`/`::` prefixes.
- Errors include the cause reported by libapg: the failing file and
  `strerror()` text, script exit status, the missing dependency and its
  version constraint, and whether the database is locked or cannot be
  opened.
- Network errors show the curl error or HTTP status; an unreachable
  repository is reported once instead of making packages look missing,
  and a repository without `repodata.json` is reported as such.
- Unsigned packages are summarized in one warning per transaction instead
  of one warning per package; the full signature paths are shown with
  `--verbose`.
- Confirmation prompts go to stderr; when standard input is not a
  terminal and `-y` is not given, the command fails with a hint instead
  of silently aborting, and a declined confirmation exits non-zero.
- Russian and Kazakh translations cover all new messages; status verbs
  and table columns are aligned by character width.

### Fixed

- `install` recorded every dependency it pulled in as installed
  explicitly, so `orphans`/`autoremove` never found them; dependencies are
  now added as auto-installed.
- `upgrade <pkg>`, `upgrade <pkg>=<version>`, the full `upgrade`, and
  `rollback` could install an unrelated package that merely provides the
  requested name.
- A failed download was reported as the package being up to date or not
  found.
- `install x x` read a freed package while building the SAT roots.
- Global flags placed after `--` were consumed instead of being passed to
  the command.

## [0.4.0] - 2026-09-16

### Added

- `--dry-run`/`-n` flag on `install`, `upgrade`, `remove`, `orphans`, and
  `rollback`: resolves and prints the transaction plan, verifies
  signatures where applicable, then stops without writing anything to
  disk, backed by libapg's new `trans_set_dry_run()`. The `-n` short
  form is only wired up for `install` and `remove`.
- `--nodeps` flag on `tulpar remove`: removes the requested package(s)
  even if other installed packages still depend on them, instead of
  refusing with `TRANS_ERR_HAS_DEPENDENTS`, backed by libapg's new
  `install_policy.skip_dependents_check`.

### Changed

- libapg pinned to v2.5.0 (`1ecebf7`)
- On an install/upgrade failure inside a transaction, the reported
  error now includes the underlying extraction failure detail from
  libapg's new `archive_last_error()` when available.

## [0.3.1] - 2026-09-09

### Changed

- Update `<util.h>` includes to `<apg/util.h>` following libapg 2.3.2 header relocation

## [0.3.0]

### Added

- `tulpar rollback` (also `undo`) command: undoes the most recent successful
  package transaction batch recorded in the database journal by inverting
  its operations (uninstalling installed packages, reinstalling removed
  packages).
- `tulpar search`: `--installed` and `--remote` flags to restrict search
  scope to locally installed packages or available remote repositories.
- `tulpar info <pkg>`: display dependencies, provides, conflicts,
  replaces, and reverse dependencies (`Required By`) for installed
  packages in both human-readable and `--json` formats.
- `tulpar clean` (also `autoclean`) command: purges downloaded package
  archives, temporary git clone workspaces, and metadata cache files
  from the package cache directory, with interactive confirmation and
  `-y`/`--yes` override.
- `tulpar list [pattern]`: filter the list of installed packages by an
  optional substring pattern in text and `--json` modes.
- `tulpar graph [package]`: export only the transitive dependency
  subgraph for a specific package instead of the entire database.
- `--nodeps` flag on `tulpar install`: installs exactly the requested
  package(s), skipping tulpar's own dependency resolution/download step
  entirely, backed by libapg's new `install_policy.skip_dependency_check`.

### Changed

- Replaced internal `copy_file()` and `remove_dir_recursive()` with
  libapg's `<util.h>` implementations.
- libapg pinned to v2.2.0 (`191635f`) with SAT dependency resolver
  integration in `resolve_install_closure()`, then to v2.3.0 (`df125f0`)
  for `install_policy.skip_dependency_check`.
- Strict option and positional argument validation across all CLI commands:
  unrecognized flags and excess arguments are rejected with an error
  and usage display instead of being ignored; added `--` delimiter support.

### Fixed

- `tulpar remove`: validate package installation before running a
  transaction, and support resolving `.apg` archive file paths to their
  installed package names.

## [0.2.0]

### Added

- `tulpar graph` command: exports the installed package dependency graph
  in Graphviz DOT format via libapg's `dep_graph_new()`/
  `dep_graph_add_installed()`/`dep_graph_export_dot()`, with `-o <file>`
  to write to a file instead of stdout.
- Internationalization (i18n) via GNU gettext: all user-facing CLI output
  (usage text, command summaries, prompts, errors, warnings, transaction
  plan/conflict output) is now translatable. Includes Russian and Kazakh
  translations (`po/ru.po`, `po/kk.po`).
- `tulpar install <url>`: install a package directly from a plain HTTP
  or FTP URL to a `.apg` file, bypassing the configured repository list
  entirely. Downloads straight to the package cache and otherwise
  follows the normal install flow (signature warning, plan,
  confirmation).
- `tulpar install <git-url>`: install from a git repository containing
  an already-unpacked package tree (`data/`, `metadata.json`,
  `scripts/`, not a built `.apg` archive), for local development and
  testing workflows. Supports `git+http(s)://`, `git+ssh://`,
  `git+git://`, `git+file://`, `git://`, and bare `.git`-suffixed URLs,
  with an optional `#ref` suffix to select a branch, tag, or commit.
- `--sign <sig-path>` flag on `tulpar install` to supply a detached
  signature separately from the `<pkg>.sig` convention.
- `--provider <name>=<package>` flag on `tulpar install` to force
  resolution of an ambiguous dependency or virtual (`provides`) name to
  a specific package, via libapg's `trans_prefer_provider()`.
- Interactive provider selection: when a dependency has multiple
  providers and `--provider` wasn't given, tulpar prompts for a choice
  on a tty; under `-y` or a non-interactive session it fails with a
  clear `--provider name=package` hint instead of guessing.
- Dependency resolution now honours `provides` and `replaces` against
  packages already queued in the transaction, already installed, and
  available in remote repodata, not just exact name matches.
- Version pinning for `tulpar upgrade`: `tulpar upgrade name=version`
  upgrades (or downgrades) to that exact version instead of the newest
  available one.
- `--exclude <name>` flag on `tulpar upgrade` (repeatable) to skip
  specific packages during a full upgrade.

### Changed

- libapg pinned to v2.0.0 (`2cf0ec8`) and v2.1.0 (`01d5418`).

### Fixed

- REST client brought in line with the real Tulpar Server API response
  shapes (`packages/{name}` detail response, channel/`noarch` fallback
  handling in download and dependency resolution).

## [0.1.0]

### Added

- Initial C rewrite of tulpar, backed by libapg for local package
  database and transaction handling: `install`, `remove`/`uninstall`,
  `upgrade`, `search`, `list`, `info`/`show`, `verify`,
  `orphans`/`autoremove`, `hold`/`unhold`, `history`/`audit`/`log`.
- `repo add`/`remove`/`list`/`update` against a plain-text repo list.
- `download`, `key add`/`list`.
- `--dest` alternate-root support across the install/remove/upgrade
  family.
- `tulpar.conf` and repo list parsing, system-wide plus per-user
  overrides.
- `repodata.json` caching with a configurable TTL.
- Parallel downloads with a configurable limit.
- Tulpar Server REST v2 client over libcurl.
- Signature verification against libapg's libsodium backend.
- Colored terminal UI with `NO_COLOR`/tty detection, progress bars,
  transaction plan confirmation, and `--json` output.
- Syslog-style logging with optional Linux journald mirroring.
- Man pages and shell completions (bash, zsh, fish).
- Unit tests for argument parsing, repo/config parsing, and transaction
  plan rendering.

### Removed

- gpgme signing backend support; libsodium is now the sole signing
  backend, matching libapg's own removal of the pluggable backend API.
- Checksum verification (`crc32sums`/`md5sums`); signature verification
  is now the sole integrity mechanism, matching libapg's removal of the
  checksum API.
