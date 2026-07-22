#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

TARGETS = [
    (ROOT / 'meson.build', r"(version:\s*')\d+\.\d+\.\d+(')"),
    (ROOT / 'flake.nix',   r'(version\s*=\s*")\d+\.\d+\.\d+(";\s*)'),
]

SEMVER = re.compile(r'^\d+\.\d+\.\d+$')


def current_version() -> str:
    text = (ROOT / 'meson.build').read_text()
    m = re.search(r"version:\s*'(\d+\.\d+\.\d+)'", text)
    if not m:
        sys.exit('error: could not find version in meson.build')
    return m.group(1)


def bump(path: Path, pattern: str, new: str) -> bool:
    text = path.read_text()
    updated, n = re.subn(pattern, rf'\g<1>{new}\2', text)
    if n == 0:
        print(f'  skip  {path.relative_to(ROOT)}  (pattern not found)')
        return False
    path.write_text(updated)
    print(f'  ok    {path.relative_to(ROOT)}')
    return True


def main() -> None:
    old = current_version()
    print(f'current version: {old}')

    new = input('new version:     ').strip()
    if not SEMVER.match(new):
        sys.exit(f'error: "{new}" is not a valid semver (X.Y.Z)')
    if new == old:
        sys.exit('error: new version is the same as current')

    print()
    for path, pattern in TARGETS:
        if path.exists():
            bump(path, pattern, new)
        else:
            print(f'  skip  {path.relative_to(ROOT)}  (not found)')

    print()
    print(f'commit: chore: bump version to {new}')


if __name__ == '__main__':
    main()
