# Tulpar

[![License](https://img.shields.io/github/license/nuros-linux/Tulpar)](LICENSE)
[![Version](https://img.shields.io/github/v/release/nuros-linux/Tulpar)](https://github.com/nuros-linux/Tulpar/releases)

Tulpar is a package manager created for the NurOS project.

## How Tulpar Works

Tulpar works with package repositories that store binary and source packages. When a user executes an installation or removal command, Tulpar automatically:

1. Checks available repositories
2. Downloads required files and dependencies
3. Installs them on the system

During package installation or removal, all dependent components are processed automatically to ensure program functionality.

## Commands

### Package Management
```bash
# Install package from remote repositories
tulpar install <package-name>

# Remove package
tulpar remove <package-name>

# Update specific package
tulpar update <package-name>

# Install from local .apg file
tulpar -i <package.apg>

# System upgrade
tulpar upgrade

# Package information
tulpar info                    # Brief list of all packages
tulpar info <package-name>     # Detailed package information

# Search packages
tulpar search <query>
```

### Repository Management
```bash
# Add repository
tulpar repo -a <repository-url>

# Remove repository
tulpar repo -r <repository-url>
```

## Configuration

Configuration files are located at `/etc/tulpar/config`  
Installed packages are located at `/var/lib/tulpar/packages/`
Unpacked packages at process are located at `/var/spool/tulpar/`

## License

This project is licensed under the GPL 3 License - see the [LICENSE](LICENSE) file for details.

## Support

If you encounter any problems, please [open an issue](https://github.com/nuros-linux/Tulpar/issues).
