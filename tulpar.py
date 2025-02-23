#!/usr/bin/python3

from os import getuid
if getuid()!=0:
    print('⚠ You must run Tulpar as root user.')
    exit(1)
from os import path,replace,chdir,remove,listdir
from tarfile import open as taropen
from argparse import ArgumentParser
from random import randint
from hashlib import md5
from json import load
import shutil

ARGPARSE=ArgumentParser()
SUB=ARGPARSE.add_subparsers(dest='command')
SUB.add_parser('install',help='Install the public package').add_argument('PACKAGE')
SUB.add_parser('remove',help='Remove the installed package').add_argument('PACKAGE')
SUB.add_parser('list',help='Lists all installed packages')
ARGPARSE.add_argument('-i',help='Install the local package, requires path to package file')
ARGS=ARGPARSE.parse_args()

if len(ARGS._get_kwargs())==0:
    print('⚠ Type -h for help.')
    exit(0)

def ask():
    try:
        while True:
            answer=input('[y/n]: ').lower()
            if answer=='y':return True
            elif answer=='n':return False
    except:exit(0)

class ProceedInstall:
    def __init__(self,file_path:str):
        if not path.exists(file_path):
            print(' - This package file path not found.')
            exit(1)
        self.file_path=file_path
        self.__temp=path.abspath(f'.tulpar{randint(1,999999)}')
        if self.file_path.endswith('.apg'):
            with taropen(self.file_path,'r:xz')as __file:
                self.file_path=self.__temp
                __file.extractall(self.file_path)
                __file.close()
            chdir(self.file_path)
            self.metadata=load(open('metadata.json'))
            if path.exists(path.join('/var/lib/tulpar/packages',self.metadata['name'])):
                print(f'⚠ "{self.metadata["name"]}" is already installed. Do you want to reinstall it?')
                if not ask():self.cancel(0)
                shutil.rmtree(path.join('/var/lib/tulpar/packages',self.metadata['name']))
            replace(self.file_path,'/var/lib/tulpar/packages/'+self.metadata['name'])
            self.file_path='/var/lib/tulpar/packages/'+self.metadata['name']
            chdir(self.file_path)
        else:
            print(' - This is not APG package file.')
            self.cancel(1)
        print('--- Installing '+self.metadata['name'])
        self.checksum()
        self.install()
    def cancel(self,exit_code:int):
        shutil.rmtree(self.__temp)
        exit(exit_code)
    def checksum(self):
        print(' * Checking checksums...')
        if not path.exists('md5sums'):
            print(" - Checksum file is not found in package.")
            self.cancel(1)
        else:
            with open('md5sums') as f:
                checksums=f.readlines()
                f.close()
        for line in checksums:
            parts = line.strip().split('  ')
            if len(parts) == 2:
                expected_checksum,filename=parts
                file_path = path.join('data',filename)
                if path.exists(file_path):
                    with open(file_path, 'rb') as file_content:
                        file_data = file_content.read()
                        actual_checksum = md5(file_data).hexdigest()
                        if actual_checksum != expected_checksum:
                            print(f" - Checksum mismatch for {filename}")
                            self.cancel(1)
                        else:
                            #print(f' + {filename} == {actual_checksum}')
                            ...
                else:
                    print(f" - Missing file: {filename}")
                    self.cancel(1)
    def install(self):
        print(' ? Do you want to install this package?')
        if ask():
            shutil.copytree(path.abspath('data'),'/',True,dirs_exist_ok=True)
            print(f'-+- Successfully installed "{self.metadata["name"]}"')
        else:
            self.cancel(0)

class ProceedRemove:
    def __init__(self,name:str):
        self.file_path=path.join('/var/lib/tulpar/packages',name)
        if not path.exists(self.file_path):
            print(f' - This package is not found.')
            exit(1)
        else:
            self.metadata=load(open(path.join(self.file_path,'metadata.json')))
            print('--- Removing '+self.metadata['name'])
            self.packages_to_remove={self.metadata['name']:[self.file_path]}
            with open(path.join(self.file_path,'md5sums'))as __file:
                files=__file.readlines()
                for file in files:
                    file=file.split()[1]
                    if not file.startswith('/'):file=path.join('/',file)
                    self.packages_to_remove[self.metadata['name']].append(file)
                __file.close()
            for dependent in self.metadata['dependencies']:
                if dependent==self.metadata['name']:
                    print('⚠ Warning! Unexpected loop in dependencies.')
                    continue
                dependency_file_path=path.join('/var/lib/tulpar/packages',dependent['name'])
                if not path.exists(dependency_file_path):
                    print(f'⚠ Note that "{dependent["name"]}" package is installed manually and package manager can`t remove it.')
                else:
                    self.packages_to_remove[dependent['name']]=[dependency_file_path]
                    with open(path.join(dependency_file_path,'md5sums'))as __file:
                        files=__file.readlines()
                        for file in files:
                            file=file.split()[1]
                            if not file.startswith('/'):file=path.join('/',file)
                            self.packages_to_remove[dependent['name']].append(file)
                        __file.close()
            for package in listdir('/var/lib/tulpar/packages/'):
                if package==self.metadata['name']:continue
                package=path.join('/var/lib/tulpar/packages',package)
                metadata=load(open(path.join(package,'metadata.json')))
                for dependent in metadata['dependencies']:
                    if dependent['name']in self.packages_to_remove:del self.packages_to_remove[dependent['name']]
            print(' & This operation will remove these packages:')
            print(', '.join(self.packages_to_remove))
            print(' ? Do you want to continue?')
            if ask():
                for package in self.packages_to_remove.items():
                    print(' & Removing '+package[0])
                    for file in package[1]:
                        if path.isdir(file):shutil.rmtree(file)
                        else:remove(file)
                        print(' * Removed '+file)
                print(f'-+- Successfully removed "{self.metadata["name"]}"')
            else:exit(0)

class ListPackages:
    def __init__(self):
        for package in listdir('/var/lib/tulpar/packages'):
            package=path.join('/var/lib/tulpar/packages',package)
            metadata=load(open(path.join(package,'metadata.json')))
            print(f' * {metadata["name"]}, {metadata["version"]}')

try:
    if ARGS.i is not None:
        ProceedInstall(ARGS.i)
    if ARGS.command is not None:
        if ARGS.command=='remove':
            ProceedRemove(ARGS.PACKAGE)
        elif ARGS.command=='list':ListPackages()

except Exception as exception:
    print(f'⚠ An error occurred while working with package manager: {exception}\nRaise an exception for developers?')
    if ask():raise exception
    else:exit(1)
