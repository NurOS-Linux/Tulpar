from tulpar.package_manager import PackageManager
from tulpar.repository_manager import RepositoryManager

pkg_manager = PackageManager()
repo_manager = RepositoryManager()

def install(args):
    if len(args) < 1:
        print("Usage: tulpar install <package>")
        return
    package = args[0]
    pkg_manager.install_package(package)

def remove(args):
    if len(args) < 1:
        print("Usage: tulpar remove <package>")
        return
    package = args[0]
    pkg_manager.remove_package(package)

def update(args):
    if len(args) < 1:
        print("Usage: tulpar update <package>")
        return
    package = args[0]
    pkg_manager.update_package(package)

def install_local_or_remote(args):
    if len(args) < 1:
        print("Usage: tulpar -i <package.apg> or <URL/GIT>")
        return
    source = args[0]
    pkg_manager.install_from_source(source)

def upgrade(args):
    pkg_manager.upgrade_all()

def info(args):
    if len(args) == 0:
        pkg_manager.list_packages()
    else:
        package = args[0]
        pkg_manager.package_info(package)

def search(args):
    if len(args) < 1:
        print("Usage: tulpar search <query>")
        return
    query = args[0]
    pkg_manager.search_package(query)

def repo(args):
    if len(args) < 2:
        print("Usage: apgtool repo -a <URL> or -r <URL>")
        return
    action = args[0]
    url = args[1]
    if action == '-a':
        repo_manager.add_repo(url)
    elif action == '-r':
        repo_manager.remove_repo(url)
    else:
        print(f"Unknown action: {action}")