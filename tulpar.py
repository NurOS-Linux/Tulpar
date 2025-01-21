#!/usr/bin/python3

from os import path,replace,chdir,remove,listdir,getuid
if getuid()!=0:
    print('You must run Tulpar as root user.')
    exit(1)
from tarfile import open as taropen
from argparse import ArgumentParser
from random import randint
from hashlib import md5
from json import load
import shutil

ARGPARSE=ArgumentParser()
SUB=ARGPARSE.add_subparsers(dest='command')
SUB.add_parser('install').add_argument('PACKAGE')
SUB.add_parser('remove').add_argument('PACKAGE')
ARGPARSE.add_argument('-i',required=False)
ARGS=ARGPARSE.parse_args()

class ProceedInstall:
    def __init__(self,file_path:str):
        self.file_path=file_path
        if self.file_path.endswith('.apg'):
            with taropen(self.file_path,'r:xz')as __file:
                self.file_path=path.abspath(f'.tulpar{randint(1,999999)}')
                __file.extractall(self.file_path)
                __file.close()
            chdir(self.file_path)
            self.metadata=load(open('metadata.json'))
            if path.exists(path.join('/var/lib/tulpar/packages',self.metadata['name'])):
                print(f' & "{self.metadata["name"]}" is already installed with version {self.metadata["version"]}')
                self.cancel(1)
            replace(self.file_path,'/var/lib/tulpar/packages/'+self.metadata['name'])
            self.file_path='/var/lib/tulpar/packages/'+self.metadata['name']
            chdir(self.file_path)
        print('--- Installing '+self.metadata['name'])
        self.checksum()
        self.install()
    def cancel(self,exit_code:int):
        remove(self.file_path)
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
    def __move_contents(self,src_dir,dest_dir):
        for item in listdir(src_dir):
            src_path = path.join(src_dir,item)
            if path.isdir(src_path):shutil.copytree(src_path,dest_dir,True,dirs_exist_ok=True)
            else:shutil.copy(src_path,dest_dir)
            print(f"Moved: {src_path} -> {dest_dir}")
    def install(self):
        self.__move_contents(path.abspath('data'),'/')
        print(f'-+- Successfully installed "{self.metadata["name"]}"')

if ARGS.i is not None:
    ProceedInstall(ARGS.i)