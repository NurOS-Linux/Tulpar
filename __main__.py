#!/usr/bin/env python3

from .misc import log,getuid
if getuid()!=0:
    log('warn','You must run Tulpar as root user.')
    exit(1)
log('info','Welcome to the Tulpar!')
from.import ArgumentParser

ARGS=ArgumentParser(description="Tulpar - The package manager for NurOS (CLI)")
SUB=ARGS.add_subparsers(dest='command')
SUB.add_parser('install',help='Install the public package').add_argument('PACKAGE',help='Name or path of the package to install')
SUB.add_parser('remove',help='Remove the installed package').add_argument('PACKAGE',help='Name of the package to remove')
SUB.add_parser('info',help='Display information about package').add_argument('PACKAGE',help='Name of the package to describe')
SUB.add_parser('list',help='Lists all installed packages')
REPO=SUB.add_parser('repo')
REPO.add_argument('-a',help='Adds repository to list')
REPO.add_argument('-r',help='Remove repository from list')
ARGS=ARGS.parse_args()

if len(ARGS.__dict__)==1 and ARGS.command==None:
    log('warn','No command is provided.')
    exit(1)

try:
    if ARGS.command is not None:
        log('info','Running process...')
        if ARGS.command=='install':
            from .process import ProceedInstall
            ProceedInstall(ARGS.PACKAGE)
        elif ARGS.command=='remove':
            from .process import ProceedRemove
            ProceedRemove(ARGS.PACKAGE)
        elif ARGS.command=='info':
            from .process import PackageInfo
            PackageInfo(ARGS.PACKAGE)
        elif ARGS.command=='list':
            from .process import ListPackages
            ListPackages()
except Exception as exception:
    log('fail',f'  <{exception.__class__.__name__}> {exception}')
finally:log('success','Ended process.')