# tulpar Roadmap

## v0.1.0 (current)

- [x] install, remove/uninstall, upgrade, search, list, info/show,
      verify, orphans/autoremove, hold/unhold, history/audit/log
- [x] repo add/remove/list/update against a plain-text repo list
- [x] download, key add/list
- [x] `--dest` alternate-root support across the install/remove/
      upgrade family
- [x] tulpar.conf and repo list parsing, system-wide plus per-user
      overrides
- [x] repodata.json caching with configurable TTL
- [x] parallel downloads with a configurable limit
- [x] Tulpar Server REST v2 client over libcurl
- [x] signature verification against libapg's libsodium backend
- [x] colored terminal UI with NO_COLOR/tty detection, progress bars,
      transaction plan confirmation, `--json` output
- [x] syslog-style logging with optional Linux journald mirroring
- [x] man pages and shell completions (bash, zsh, fish)
- [x] unit tests for argument parsing, repo/config parsing, and
      transaction plan rendering

## v0.2.0

- [x] `tulpar install <url>`: install a package directly from a plain
      HTTP or FTP URL to a `.apg` file, bypassing the configured
      repository list entirely (e.g.
      `tulpar install https://example.com/foo-1.0.0-x86_64.apg`).
      Downloads straight to the package cache and otherwise follows
      the normal install flow (signature warning, plan, confirmation).
- [ ] `tulpar install <git-url>`: install from a git repository
      containing an already-unpacked package tree (a `data/`,
      `metadata.json`, `scripts/` layout, not a built `.apg` archive),
      for local development and testing workflows. Clones the
      repository, reads `metadata.json` directly instead of extracting
      an archive, and otherwise follows the normal transaction flow.
- [ ] richer dependency resolution: honour `provides`/`replaces` when
      resolving transitive dependencies against remote repositories,
      not only against packages already in the transaction
- [ ] version pinning and `--exclude` style upgrade filters

## v0.3.0 and later

- [ ] additional platform support beyond Linux and FreeBSD, following
      libapg's own portability as it expands
- [ ] a transaction history rollback command built on top of the
      existing journal/rollback primitives already exposed by libapg
- [ ] richer `--json` schemas with a documented stability guarantee
