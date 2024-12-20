import sys
import logging
from tulpar import commands

# Настройка логирования
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

def print_help():
    help_text = """
    Usage: tulpar <command> [<args>]

    Commands:
      install <package>               Install a package
      remove <package>                Remove a package
      update <package>                Update a package
      -i <package.apg>                Install from local file
      -i <URL/GIT>                    Install from remote repository
      upgrade                         Upgrade all packages
      info                            List all packages
      info <package>                  Get information about a package
      search <query>                  Search for a package
      repo -a <URL>                   Add a repository
      repo -r <URL>                   Remove a repository
    """
    print(help_text)

def main():
    if len(sys.argv) < 2:
        logger.error("No command provided.")
        print_help()
        return

    command = sys.argv[1]
    args = sys.argv[2:]

    try:
        if command == 'install':
            commands.install(args)
        elif command == 'remove':
            commands.remove(args)
        elif command == 'update':
            commands.update(args)
        elif command == '-i':
            commands.install_local_or_remote(args)
        elif command == 'upgrade':
            commands.upgrade(args)
        elif command == 'info':
            commands.info(args)
        elif command == 'search':
            commands.search(args)
        elif command == 'repo':
            commands.repo(args)
        elif command == 'help':
            print_help()
        else:
            logger.error(f"Unknown command: {command}")
            print_help()
    except Exception as e:
        logger.error(f"An error occurred while executing the command '{command}': {e}")
        print(f"An error occurred: {e}")
        print_help()

if __name__ == '__main__':
    main()