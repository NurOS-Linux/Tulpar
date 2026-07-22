# tulpar

CLI package manager frontend for NurOS.

> [!NOTE]
> **Repository Mirror & Issue Tracking**
>
> - **Primary Repository:** Active development takes place on our self-hosted Forgejo instance at [git.nuros.org](https://git.nuros.org/core/tulpar) (accounts are restricted to the core team).
> - **GitHub Mirror:** This GitHub repository is a **read-only mirror**.
> - **Issues & Contributions:** External contributors are welcome and encouraged to open Issues and Pull Requests directly here on GitHub! The team will review and process them.

## About

tulpar is the command-line package manager for NurOS. It resolves,
downloads, installs, removes, and upgrades packages, backed by
[libapg](https://git.nuros.org/core/libapg) for local package database
and transaction handling, and by a Tulpar Server instance for remote
repository operations over HTTP.

libapg has no networking and no logging of its own; everything
network-related, repository-related, configuration, caching, and
logging is tulpar's responsibility, built on top of libapg's package,
database, transaction, dependency graph, signature, and journal APIs.

## Dependencies

| Dependency | Description |
|------------|-------------|
| [Meson](https://mesonbuild.com/) | Build system |
| [Ninja](https://ninja-build.org/) | Build tool |
| [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) | Helper tool for compiling |
| [libapg](https://git.nuros.org/core/libapg) | Packaging backend library, pulled in as a meson subproject |
| [libcurl](https://curl.se/libcurl/) | HTTP(S) client used for all Tulpar Server communication |
| [yyjson](https://github.com/ibireme/yyjson) | JSON library, same one libapg uses internally |

Everything else (terminal color output, progress bars, argument
parsing) is hand-written with no external dependency.

## The libapg dependency

tulpar consumes libapg as a meson subproject, wired up through
`subprojects/libapg.wrap`, a `wrap-git` pointing at
`https://git.nuros.org/core/libapg.git`, pinned to a specific commit.
Meson fetches and builds it automatically as part of a normal
`meson setup`.

**Manual mirror override:** if `git.nuros.org` is unreachable from your
network, libapg is also mirrored read-only at
[github.com/nuros-linux/libapg.git](https://github.com/nuros-linux/libapg.git).
meson's `.wrap` format does not support a fallback URL list in a single
file, so this is a manual override, not something the wrap file itself
encodes: edit `subprojects/libapg.wrap` and change the `url` to the
GitHub mirror before running `meson setup`, or drop a working checkout
directly at `subprojects/libapg/` (meson prefers an existing directory
over fetching the wrap).

The pinned revision is only the latest one known when this wrap was
written; it does not track `main` automatically and will be bumped by
the maintainer as libapg evolves.

## Signing backends

libapg builds in both signing backends side by side and lets the
caller choose at runtime; tulpar exposes that choice as its own
`sign_backend` setting:

- **sodium** (the default): libsodium, Ed25519 signing. Calls libapg's
  unsuffixed `sign_verify` / `keyring_load` / ... functions.
- **gpgme**: OpenPGP signing via GnuPG. Calls libapg's `_gpgme`
  suffixed `sign_verify_gpgme` / `keyring_load_gpgme` / ... functions.

Set `sign_backend = sodium` or `sign_backend = gpgme` in `tulpar.conf`,
or pass `--sign-backend sodium|gpgme` on the command line for a single
invocation. `require_signature` defaults to `false`, matching Tulpar
Server's own default; when a package's signature is missing or fails
verification, tulpar always prints a loud warning before proceeding,
regardless of `require_signature`. Set `require_signature = true` (or
pass `--require-signature`) to reject unsigned or unverifiable
packages outright instead of just warning.

## Install-script sandbox

Lifecycle scripts (pre-install, post-install, pre-remove, post-remove)
are executed by libapg's `run_script()`, isolated using the strongest
primitive available on the host:

- **Linux**: `unshare()` with isolated network, mount, UTS, and IPC
  namespaces.
- **FreeBSD**: `chroot()` into the alternate install root, when one is
  given (via `--dest`).
- **Other POSIX platforms**: no sandbox primitive is available;
  scripts run without isolation.

tulpar does not reimplement any of this. It only resolves where the
package database lives under `--dest`, makes sure the target root's
directory structure exists before committing, and passes the resolved
root path into `trans_commit()`, which threads it through to every
script invocation libapg triggers internally.

## Building

```bash
meson setup build --buildtype=release
meson compile -C build
```

### Running tests

```bash
meson test -C build
```

### Installing

```bash
sudo meson install -C build
```

## Configuration

### tulpar.conf

Global settings are read from two locations, in order, with later
values overriding earlier ones:

1. `/etc/apg/tulpar.conf` (system-wide)
2. `~/.config/apg/tulpar.conf` (per-user override)

The format is plain `key = value` lines; `#` starts a comment; there
are no sections. See `tulpar.conf(5)` for the full list of keys
(`cache_dir`, `db_dir`, `log_file`, `repodata_ttl`,
`max_parallel_downloads`, `require_signature`, `sign_backend`,
`color_theme`, `verbose`, `quiet`, `journald`).

### Repositories

The repository list is a plain text file, one URL per line, `#` for
comments, no key/value syntax, no sections:

- `/etc/apg/repos` (system-wide)
- `~/.config/apg/repos` (per-user addition, for managing personal
  repositories without root)

Manage it with `tulpar repo add|remove|list|update` rather than
editing it by hand if you'd rather not deal with the raw format.

### Cache

- `/var/cache/apg/pkgs/`: downloaded `.apg` archives.
- `/var/cache/apg/repodata/`: cached `repodata.json` per configured
  repository, refreshed automatically once `repodata_ttl` (default 900
  seconds) has elapsed, or on demand with `tulpar repo update`.

### Logging

Plain syslog-style text lines, one per event, written to
`/var/log/tulpar.log` by default. Override the path with `log_file` in
`tulpar.conf` or a CLI flag. `--verbose` and `--quiet` control both log
and terminal output verbosity. On Linux, log lines can additionally be
mirrored to syslog/journald by setting `journald = true`.

## Command reference

Every command has a full name, a single-letter short alias usable as
the first word, and the same letter usable as a leading flag:
`tulpar install pkg`, `tulpar i pkg`, and `tulpar -i pkg` are
identical.

| Command | Alias | Description |
|---------|-------|-------------|
| `install` | `i` | Install one or more packages |
| `remove`, `uninstall` | `r` | Remove an installed package |
| `upgrade` | `u` | Upgrade one named package, or all installed packages |
| `search` | `s` | Search the local database and remote repository indexes |
| `list` | `l` | List installed packages |
| `info`, `show` | `n` | Show full metadata for a package |
| `verify` | `v` | Verify installed package files |
| `orphans`, `autoremove` | `o` | List and remove orphaned packages |
| `hold` | `g` | Block a package from upgrade or removal |
| `unhold` | `x` | Release a held package |
| `history`, `audit`, `log` | `a` | Show past install/remove operations |
| `repo add\|remove\|list\|update` | `e` | Manage configured repositories |
| `download` | `w` | Fetch a package without installing it |
| `key add\|list` | `k` | Manage the trusted signing keyring |

`-h`/`-V`/`-y`/`-j`/`-d`/`-q` are reserved globally for help, version,
`--yes`, `--json`, `--dest`, and `--quiet`, so no command alias reuses
those letters; see `tulpar(1)` for the full rationale behind each
letter choice.

Every install/remove/upgrade-family command accepts `--dest <path>` to
operate against an alternate filesystem root instead of the live
system. The package database then lives under
`<dest>/var/lib/apg/db`, dependency resolution runs against that
database, and root privileges are not required (the destination may be
user-owned). Without `--dest`, tulpar operates on the live root and
requires `geteuid() == 0` before any write operation.

Full documentation for every command is available as a man page:
`tulpar(1)`, `tulpar-install(1)`, `tulpar-remove(1)`,
`tulpar-upgrade(1)`, `tulpar-search(1)`, `tulpar-list(1)`,
`tulpar-info(1)`, `tulpar-verify(1)`, `tulpar-orphans(1)`,
`tulpar-hold(1)`, `tulpar-unhold(1)`, `tulpar-history(1)`,
`tulpar-repo(1)`, `tulpar-download(1)`, `tulpar-key(1)`, and
`tulpar.conf(5)`.

## License

This project is licensed under the **GNU General Public License v3.0**
(GPL-3.0).

See the LICENSE file for details.
