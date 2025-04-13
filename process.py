from argparse import ArgumentParser
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

from os import path,replace,chdir,remove,listdir,system
from packaging.version import Version
from tarfile import open as taropen
from colorama import Fore
from random import randint
from shutil import rmtree
from requests import get
from hashlib import md5
from json import load
 
if ARGS.command is None:
    print('warn','You must specify a command.')
    exit(1)
else:
    if ARGS.command=='install':
        class ProceedInstall:
            def __init__(self,file_path:str):
                self.packages_to_install=[]
                with open('/etc/tulpar/repolist','rt')as repolist:
                    self.repositories=[i.strip()for i in repolist.readlines()]
                    repolist.close()
                if file_path.endswith('.apg'):
                    if not path.exists(file_path):
                        print(' - This file path not found.')
                        self.cancel(1)
                    self.packages_to_install.append(self.unpack(file_path))
                else:self.packages_to_install.append(self.get_info(file_path))
                print(' & Resolving dependencies...')
                for dependency in self.packages_to_install[0]['dependencies']:
                    if path.exists(path.join('/var/lib/tulpar/packages',dependency)):continue
                    else:self.get_dependency(dependency)
                self.checksum()
            def cancel(self,exit_code):
                for package in self.packages_to_install:
                    if package['extracted-path'].startswith('/var/spool/tulpar/'):rmtree(package['extracted-path'])
                exit(exit_code)
            def get_info(self,package):
                log('info',f'Retrieving information about {package}...')
                for repo in self.repositories:
                    try:
                        with get(path.join(repo,'packages',package,'info'))as request:
                            if request.status_code!=200:raise request.status_code
                            else:package_info=request.json()
                        if not CURRENT_ARCH in package_info['architecture']:
                            print(' - This package is not supports machine`s architecture!')
                            exit(1)
                        package_info['from-repo']=repo
                        package_info['extracted-path']=None
                        return package_info
                    except Exception as e:log('warn',f'Failed to retrieve information about package: {e}')
                    finally:continue
            def unpack(self,file):
                log('info',f'Unpacking {path.basename(file)}...')
                try:
                    with taropen(file,'r:xz')as __file:
                        file=path.join('/var/spool/tulpar',f'{randint(1,9999999999)}')
                        __file.extractall(file,filter='fully_trusted')
                        __file.close()
                    with open(path.join(file,'metadata.json'),'rt')as metadata:
                        package=load(metadata)
                        package['extracted-path']=file
                        package['from-repo']=None
                        metadata.close()
                        return package
                except Exception as e:
                    log('warn',f'Failed to unpack package: {e}')
                    exit(1)
            def download(self,package_info):
                try:
                    with get(path.join(package_info['from-repo'],'packages',package_info['name']+f'?arch={CURRENT_ARCH}&version={package_info["version"]}'))as request:
                        if request.status_code!=200:raise request.status_code
                        else:
                            package_path=path.join('/var/cache/tulpar',f'{randint(1,9999999999)}.apg')
                            with open(package_path,'wb')as _:
                                _.write(request.content)
                                _.close()
                            package_info['extracted-path']=package_path
                            return package_info
                except Exception as e:print(Fore.RED+f' - Failed to download {package_info['name']}: {e}')
            def get_dependency(self,package):
                package=self.get_info(package)
                if package is None:print(Fore.YELLOW+f'/!\\ Continue without this dependency.')
                else:self.packages_to_install.append(package)
            def __checksum_check_dir(self,file):
                md5sums_file=path.join(file,'md5sums')
                if not path.exists(md5sums_file):
                    print(" - Checksum file is not found in package.")
                    self.cancel(1)
                else:
                    with open(md5sums_file) as __md5sumsfile:
                        checksums=__md5sumsfile.readlines()
                        __md5sumsfile.close()
                data_path=path.join(file,'data')
                for line in checksums:
                    line=line.strip().split()
                    if len(line)==2:file_to_check,expected_checksum=line
                    if file_to_check.startswith('usr/local'):
                        print('/!\\ Installing to /usr/local/ folders are not allowed!')
                        self.cancel(1)
                    file_path=path.join(data_path,file_to_check)
                    if path.exists(file_path):
                        with open(file_path,'rb') as file_content:
                            file_data = file_content.read()
                            actual_checksum=md5(file_data).hexdigest()
                            if actual_checksum != expected_checksum:
                                print(f" - Checksum mismatch for {file}")
                                self.cancel(1)
                            else:
                                #print(f' + {file} == {actual_checksum}')
                                ...
                    else:
                        print(f" - Missing file: {file}")
                        self.cancel(1)
            def checksum(self):
                print(' & Checking files...')
                for package in self.packages_to_install:
                    print(f' * Checking {package["name"]}...')
                    self.__checksum_check_dir(package['extracted-path'])
            def __inst_dir(self,dir:str):
                for file in listdir(dir):
                    file=path.join(dir,file)
                    if path.isdir(file):self.__inst_dir(file)
                    else:replace(file,file.replace(dir,'/'))
            def install(self):
                print(', '.join(self.packages_to_install))
                print(' ? Do you want to install these packages?')
                if ask():
                    for package in self.packages_to_install:
                        system(path.join('/var/lib/tulpar/packages',package,'scripts/preinstall'))
                        self.__inst_dir(self.data_path)
                        rmtree(self.data_path)
                        system(path.join('/var/lib/tulpar/packages',package,'scripts/postinstall'))
                    print(f'-+- Successfully installed "{self.metadata["name"]}"')
                else:self.cancel(0)

class ProceedRemove:
    def __init__(self,name:str):
        self.file_path=path.join('/var/lib/tulpar/packages',name)
        if not path.exists(self.file_path):
            print(f' - This package is not found.')
            exit(1)
        else:
            self.metadata=load(open(path.join(self.file_path,'metadata.json')))
            self.packages_to_remove={self.metadata['name']:[self.file_path]}
            self.get_packages()
            print(' & This operation will remove these packages:')
            print(', '.join([i[0] for i in self.packages_to_remove.items()]))
            print(' ? Do you want to continue?')
            if ask():
                for package in self.packages_to_remove.items():
                    print(' & Removing '+package[0])
                    system(path.join('/var/lib/tulpar/packages',package[0],'scripts/preremove'))
                    for file in package[1]:
                        if path.isdir(file):rmtree(file)
                        else:remove(file)
                        print(' * Removed '+file)
                    system(path.join('/var/lib/tulpar/packages',package[0],'scripts/postremove'))
                print(f'-+- Successfully removed "{self.metadata["name"]}"')
            else:exit(0)
    def get_files(self,package_path,package:list):
        with open(path.join(package_path,'md5sums'))as __files:
            files=__files.readlines()
            for file in files:
                file=file.split()[0]
                if not file.startswith('/'):file=path.join('/',file)
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
                print(f'/!\\ Dependency "{dependency["name"]}" is installed manually or not found.')
                continue
        for other_package in listdir('/var/lib/tulpar/packages'):
            # Проверяем другие пакеты если они не относятся к операции
            if other_package in self.packages_to_remove:continue
            other_package_metadata=load(open(path.join('/var/lib/tulpar/packages',other_package,'metadata.json')))
            for other_package_dependency in other_package_metadata['dependencies']:
                if other_package_dependency in self.packages_to_remove:
                    if other_package_dependency==self.metadata['name']:
                        if not ask('/!\\ Warning! This package is required by other package, are you sure you want to delete it?'):exit(0)
                    del self.packages_to_remove[other_package_dependency]
        for dependency in self.packages_to_remove:
            self.packages_to_remove[dependency['name']].append(path.join('/var/lib/tulpar/packages',dependency['name']))
            self.get_files(dependency_file_path,self.packages_to_remove[dependency['name']])

class PackageInfo:
    def __init__(self,package_name):
        self.package_path=path.join('/var/lib/tulpar/packages',package_name)
        if not path.exists(self.package_path):
            print('/!\\ This package is not found')
            exit(1)
        else:
            with open(path.join(self.package_path,'metadata.json'))as self.metadata:self.metadata=load(self.metadata)
            print(f' & {self.metadata['name']} with version {self.metadata['version']} for {self.metadata['architecture']}\n   Made by {self.metadata['maintainer']} under license {self.metadata['license']}, {self.metadata['homepage']}\n   {self.metadata['description']}')

class ListPackages:
    def __init__(self):
        for package in listdir('/var/lib/tulpar/packages'):
            package=path.join('/var/lib/tulpar/packages',package)
            metadata=load(open(path.join(package,'metadata.json')))
            print(f' * {metadata["name"]}, {metadata["version"]}')