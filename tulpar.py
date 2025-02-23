#!/usr/bin/env python3

from os import getuid
if getuid()!=0:
    print('⚠ You must run Tulpar as root user.')
    exit(1)
from os import path,replace,chdir,remove,listdir,system
from tarfile import open as taropen
from argparse import ArgumentParser
from random import randint
from shutil import rmtree
from hashlib import md5
from json import load

ARGPARSE=ArgumentParser(description="Package Manager CLI")
SUB=ARGPARSE.add_subparsers(dest='command')
SUB.add_parser('install',help='Install the public package').add_argument('PACKAGE',help="Name of the package to install")
SUB.add_parser('remove',help='Remove the installed package').add_argument('PACKAGE',help="Name of the package to remove")
SUB.add_parser('list',help='Lists all installed packages')
ARGPARSE.add_argument('-i',metavar="PACKAGE_PATH",help='Path to the local package (for install command)')
ARGS=ARGPARSE.parse_args()
#if not ARGS.command and not ARGS.i or ARGS.command is not None and ARGS.i is not None:ARGPARSE.error("Either a command or the -i option is required.")

def ask():
    try:
        while True:
            answer=input('[y/n]: ').lower()
            if answer=='y':return True
            elif answer=='n':return False
    except:exit(0)

class ProceedInstall:
    def __init__(self,file_path:str,local:bool):
        if not path.exists(file_path):
            print(' - This file path not found.')
            self.cancel(1)
        self.file_path=path.abspath(file_path)
        self.temp_path=path.join('/var/spool/tulpar',str(randint(1,999999999)))
        if self.file_path.endswith('.apg'):
            print(f' * Unpacking {self.file_path}...')
            with taropen(self.file_path,'r:xz')as __file:
                __file.extractall(self.temp_path)
                __file.close()
            self.file_path=self.temp_path
        else:
            if local:
                print(' - This is not APG package file.')
                self.cancel(1)
        chdir(self.file_path)
        self.checksum()
        self.metadata=load(open('metadata.json'))
        if path.exists(path.join('/var/lib/tulpar/packages',self.metadata['name'])):
            print(f'⚠ "{self.metadata["name"]}" is already installed. Do you want to reinstall it?')
            if not ask():self.cancel(0)
            else:ProceedRemove(self.metadata["name"])
        replace(self.file_path,'/var/lib/tulpar/packages/'+self.metadata['name'])
        self.file_path='/var/lib/tulpar/packages/'+self.metadata['name']
        chdir(self.file_path)
        self.data_path=path.abspath('data')
        self.install()
    def cancel(self,exit_code):
        if path.exists(self.temp_path):
            print(' * Removing cache files...')
            rmtree(self.file_path)
        print('--- Canceled operation.')
        exit(exit_code)
    def checksum(self):
        print(' * Checking checksums...')
        if not path.exists('md5sums'):
            print(" - Checksum file is not found in package.")
            self.cancel(1)
        else:
            with open('md5sums') as __md5sumsfile:
                checksums=__md5sumsfile.readlines()
                __md5sumsfile.close()
        for line in checksums:
            parts=line.strip().split()
            print(parts)
            if len(parts)==2:
                file,expected_checksum=parts
                file_path=path.join(self.data_path,file)
                print(file_path)
                if path.exists(file_path):
                    with open(file_path,'rb') as file_content:
                        file_data = file_content.read()
                        actual_checksum=md5(file_data).hexdigest()
                        if actual_checksum != expected_checksum:
                            print(f" - Checksum mismatch for {file}")
                            print(f' - {expected_checksum} != {actual_checksum}')
                            self.cancel(1)
                        else:
                            #print(f' + {file} == {actual_checksum}')
                            ...
                else:
                    print(f" - Missing file: {file}")
                    self.cancel(1)
    def __inst_dir(self,dir:str):
        for file in listdir(dir):
            file=path.join(dir,file)
            if path.isdir(file):self.__inst_dir(file)
            else:replace(file,file.replace(dir,'/'))
    def install(self):
        print(' ? Do you want to install this package?')
        if ask():
            system('scripts/preinstall')
            self.__inst_dir(self.data_path)
            rmtree(self.data_path)
            system('scripts/postinstall')
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
            self.packages_to_remove={self.metadata['name']:[self.file_path]}
            self.get_packages(self.file_path)
            print(' & This operation will remove these packages:')
            print(', '.join(self.packages_to_remove))
            print(' ? Do you want to continue?')
            if ask():
                for package in self.packages_to_remove.items():
                    print(' & Removing '+package[0])
                    for file in package[1]:
                        if path.isdir(file):rmtree(file)
                        else:remove(file)
                        print(' * Removed '+file)
                print(f'-+- Successfully removed "{self.metadata["name"]}"')
            else:exit(0)
    def get_files(self,package_path,package:list):
        with open(path.join(package_path,'md5sums'))as __files:
            files=__files.readlines()
            for file in files:
                file=file.split()[0]
                package.append(file)
            __files.close()
    def get_packages(self):
        self.get_files(self.file_path,self.packages_to_remove[self.metadata['name']])
        for dependency in self.metadata['dependencies']:
            if dependency['name']==self.metadata['name']:
                print('/!\\ Warning! Unexpected loop in dependencies list.')
                continue
            dependency_file_path=path.join('/var/lib/tulpar/packages',dependency['name'])
            if not path.exists(dependency_file_path):
                print(f' /!\\ Dependency "{dependency["name"]}" is installed manually or not found.')
                continue
            self.packages_to_remove[dependency['name']]=[]
            self.get_files(dependency_file_path,self.packages_to_remove[dependency['name']])
        for other_package in listdir('/var/lib/tulpar/packages'):
            # Проверяем другие пакеты если они не относятся к операции
            if other_package in self.packages_to_remove:continue
            other_package_metadata=load(open(path.join('/var/lib/tulpar/packages',other_package,'metadata.json')))
            for other_package_dependency in other_package_metadata['dependencies']:
                if other_package_dependency in self.packages_to_remove:del self.packages_to_remove[other_package_dependency]

class ListPackages:
    def __init__(self):
        for package in listdir('/var/lib/tulpar/packages'):
            package=path.join('/var/lib/tulpar/packages',package)
            metadata=load(open(path.join(package,'metadata.json')))
            print(f' * {metadata["name"]}, {metadata["version"]}')

try:
    if ARGS.command is not None:
        if ARGS.command=='install':
            if ARGS.PACKAGE is not None:ProceedInstall(ARGS.PACKAGE,False)
            else:ProceedInstall(ARGS.i,True)
        elif ARGS.command=='remove':ProceedRemove(ARGS.PACKAGE)
        elif ARGS.command=='list':ListPackages()

except Exception as exception:
    print(f'/!\\ An error occurred while working with package manager: {exception}\nRaise an exception for developers?')
    if ask():raise exception
    else:exit(1)