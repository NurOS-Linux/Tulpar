import logging
from tulpar.package_manager import PackageManager
from tulpar.repository_manager import RepositoryManager

# Настройка логирования
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

pkg_manager = PackageManager()
repo_manager = RepositoryManager()

def install(args):
    if len(args) < 1:
        logger.error("Usage: tulpar install <package>")
        return
    package = args[0]
    try:
        pkg_manager.install_package(package)
        logger.info(f"Successfully installed package: {package}")
    except Exception as e:
        logger.error(f"Failed to install package {package}: {e}")

def remove(args):
    if len(args) < 1:
        logger.error("Usage: tulpar remove <package>")
        return
    package = args[0]
    try:
        pkg_manager.remove_package(package)
        logger.info(f"Successfully removed package: {package}")
    except Exception as e:
        logger.error(f"Failed to remove package {package}: {e}")

def update(args):
    if len(args) < 1:
        logger.error("Usage: tulpar update <package>")
        return
    package = args[0]
    try:
        pkg_manager.update_package(package)
        logger.info(f"Successfully updated package: {package}")
    except Exception as e:
        logger.error(f"Failed to update package {package}: {e}")

def install_local_or_remote(args):
    if len(args) < 1:
        logger.error("Usage: tulpar -i <package.apg> or <URL/GIT>")
        return
    source = args[0]
    try:
        pkg_manager.install_from_source(source)
        logger.info(f"Successfully installed from source: {source}")
    except Exception as e:
        logger.error(f"Failed to install from source {source}: {e}")

def upgrade(args):
    try:
        pkg_manager.upgrade_all()
        logger.info("Successfully upgraded all packages")
    except Exception as e:
        logger.error(f"Failed to upgrade all packages: {e}")

def info(args):
    try:
        if len(args) == 0:
            pkg_manager.list_packages()
        else:
            package = args[0]
            pkg_manager.package_info(package)
        logger.info("Successfully retrieved package information")
    except Exception as e:
        logger.error(f"Failed to retrieve package information: {e}")

def search(args):
    if len(args) < 1:
        logger.error("Usage: tulpar search <query>")
        return
    query = args[0]
    try:
        pkg_manager.search_package(query)
        logger.info(f"Successfully searched for package: {query}")
    except Exception as e:
        logger.error(f"Failed to search for package {query}: {e}")

def repo(args):
    if len(args) < 2:
        logger.error("Usage: apgtool repo -a <URL> or -r <URL>")
        return
    action = args[0]
    url = args[1]
    try:
        if action == '-a':
            repo_manager.add_repo(url)
            logger.info(f"Successfully added repository: {url}")
        elif action == '-r':
            repo_manager.remove_repo(url)
            logger.info(f"Successfully removed repository: {url}")
        else:
            logger.error(f"Unknown action: {action}")
    except Exception as e:
        logger.error(f"Failed to manage repository {url}: {e}")