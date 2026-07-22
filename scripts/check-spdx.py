#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

import sys
from pathlib import Path

SOURCE_EXTS = frozenset(['.c', '.h', '.py'])


def missing_tags(path: Path) -> list[str]:
    top = path.read_text(errors='replace').split('\n', 10)[:10]
    block = '\n'.join(top)
    found = []
    if 'SPDX-License-Identifier:' not in block:
        found.append(f'{path}: no SPDX-License-Identifier')
    if 'SPDX-FileCopyrightText:' not in block:
        found.append(f'{path}: no SPDX-FileCopyrightText')
    return found


def iter_sources(roots: list[str]) -> list[Path]:
    out = []
    for r in roots:
        p = Path(r)
        if p.is_file():
            out.append(p)
        else:
            out += [f for f in p.rglob('*') if f.is_file() and f.suffix in SOURCE_EXTS]
    return out


if __name__ == '__main__':
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument('paths', nargs='*', default=['.'])
    roots = ap.parse_args().paths

    issues = [msg for src in iter_sources(roots) for msg in missing_tags(src)]
    for i in issues:
        print(i)
    sys.exit(1 if issues else 0)
