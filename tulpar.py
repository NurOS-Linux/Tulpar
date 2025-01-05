#!/usr/bin/python3
#/var/lib/tulpar

from argparse import ArgumentParser
ARGPARSE=ArgumentParser()
SUB=ARGPARSE.add_subparsers(dest='command')
SUB.add_parser('install').add_argument('PACKAGE')
ARGPARSE.add_argument('-i',required=False)
ARGS=ARGPARSE.parse_args()

from tarfile import TarFile
from hashlib import sha256
from json import load
from os import path

class Proceed:
    def __init__(self,path):
        self.path=path
        print('Installing '+self.path)
        self.checksum()
    def checksum(self):
        print('Checking content...')
        __checksum=load(open(self.path+'/checksums.json'))
        print(__checksum)
        for file in __checksum['files']:
            with open(self.path+'/data/'+file['name'],'rb')as __file:
                print(file)
                __sum=sha256(__file.read()).hexdigest()
                if __sum!=file['sha256']:
                    print('Checksum failed in "'+file['name']+'" file\n\n'+__sum)
                    exit(1)
                __file.close()

if ARGS.i!=None:
    if ARGS.i.endswith('.apg'):
        TarFile(ARGS.i).extractall('/var/lib/tulpar/packages/'+...)
        ...
    else:
        Proceed(ARGS.i)