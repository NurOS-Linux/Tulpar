import sys
from tulpar import commands

def main():
    if len(sys.argv) < 2:
        print("Usage: tulpar <command> [<args>]")
        return

    command = sys.argv[1]
    args = sys.argv[2:]

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
    else:
        print(f"Unknown command: {command}")

if __name__ == '__main__':
    main()