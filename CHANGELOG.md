# Changelog

All notable changes to tulpar are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.2.0]

### Added

- `tulpar graph` command: exports the installed package dependency graph
  in Graphviz DOT format via libapg's `dep_graph_new()`/
  `dep_graph_add_installed()`/`dep_graph_export_dot()`, with `-o <file>`
  to write to a file instead of stdout.
- Internationalization (i18n) via GNU gettext: all user-facing CLI output
  (usage text, command summaries, prompts, errors, warnings, transaction
  plan/conflict output) is now translatable. Includes a complete Russian
  translation (`po/ru.po`).
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
  available in remote repodata — not just exact name matches.
- Version pinning for `tulpar upgrade`: `tulpar upgrade name=version`
  upgrades (or downgrades) to that exact version instead of the newest
  available one.
- `--exclude <name>` flag on `tulpar upgrade` (repeatable) to skip
  specific packages during a full upgrade.

### Changed

- libapg pinned to v2.0.0 (`2cf0ec8`), then to v2.1.0 (`01d5418`).

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
