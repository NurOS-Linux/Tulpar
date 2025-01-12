#!/usr/bin/python3

from os import path,replace,chdir,removedirs
from tarfile import open as taropen
from argparse import ArgumentParser
from random import randint
from hashlib import md5
from json import load

ARGPARSE = ArgumentParser()
SUB = ARGPARSE.add_subparsers(dest='command')
SUB.add_parser('install').add_argument('PACKAGE')
ARGPARSE.add_argument('-i',required=False)
ARGS = ARGPARSE.parse_args()

class Proceed:
    def __init__(self,file_path:str):
        self.file_path=file_path
        if self.file_path.endswith('.apg'):
            with taropen(self.file_path,'r:xz')as __file:
                self.file_path=f'.tulpar-unknown{randint(1,999999)}'
                __file.extractall(self.file_path)
                __file.close()
                self.metadata=load(open(path.join(self.file_path,'metadata.json')))
                try:replace(self.file_path,path.join('/var/lib/tulpar/packages',self.metadata['name']))
                except:
                    if path.exists(path.join('/var/lib/tulpar/packages',self.metadata['name'])):
                        print(f' & "nano" is already installed with version {self.metadata["version"]}')
                        self.exit(1)
                self.file_path='/var/lib/tulpar/packages/'+self.metadata['name']
                chdir(self.file_path)
        else:chdir(self.file_path)
        print('--- Installing '+self.metadata['name'])
        print(' * Checking checksums...')
        self.checksum()
    
    def exit(self):
        removedirs(self.file_path)

    def checksum(self):
        if not path.exists('md5sums'):
            print(" - Checksum file is not found in package.")
            self.exit(1)
        
        with open('md5sums') as f:
            checksums = f.readlines()
            f.close()
        
        for line in checksums:
            parts = line.strip().split('  ')
            if len(parts) == 2:
                expected_checksum, filename = parts
                file_path = path.join('data',filename)
                if path.exists(file_path):
                    with open(file_path, 'rb') as file_content:
                        file_data = file_content.read()
                        actual_checksum = md5(file_data).hexdigest()
                        if actual_checksum != expected_checksum:
                            print(f" - Checksum mismatch for {filename}")
                            self.exit(1)
                        else:
                            print(f' + {file_path} == {actual_checksum}',end='\r')
                else:
                    print(f" - Missing file: {filename}")
                    self.exit(1)
        #print("All checksums match!")

if ARGS.i is not None:
    Proceed(ARGS.i)
else:
    print("No input file specified.")