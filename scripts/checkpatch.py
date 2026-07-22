#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCRIPTS = Path(__file__).resolve().parent


def step(label: str, cmd: list[str]) -> bool:
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    out = (r.stdout + r.stderr).strip()
    if r.returncode != 0:
        print(f'FAIL  {label}')
        for line in out.splitlines():
            print(f'      {line}')
        return False
    print(f'ok    {label}')
    return True


def run_clang_format() -> bool:
    sources = [str(f) for f in ROOT.rglob('*')
               if f.suffix in ('.c', '.h')
               and '.git' not in f.parts
               and 'build' not in f.parts
               and 'subprojects' not in f.parts]
    return step('clang-format', ['clang-format', '--dry-run', '--Werror'] + sources)


def run_spdx() -> bool:
    return step('spdx', [sys.executable, str(SCRIPTS / 'check-spdx.py'),
                         'src', 'test', 'scripts'])


CHECKS = [run_clang_format, run_spdx]

if __name__ == '__main__':
    failed = sum(1 for c in CHECKS if not c())
    print()
    if failed:
        print(f'{failed} check(s) failed')
        sys.exit(1)
    print('all checks passed')
