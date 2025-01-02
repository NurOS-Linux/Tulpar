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
# Install package
tulpar install <package-name>

# Remove package
tulpar remove <package-name>

# Update specific package
tulpar update <package-name>

# Install from local .apg file
tulpar -i <package.apg>

# Install from remote repository/git
tulpar -i <repo-url/git-url>

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
tulpar-repo -a <repository-url>

# Remove repository
tulpar-repo -r <repository-url>
```

## Contributors

- [taliildar](https://github.com/AnmiTaliDev)
- [GoldenVadim](https://github.com/GoldenVadim)
- [ShwoubleTrouble](https://github.com/ShwoubleTrouble)

## Installation

```bash
# Build from source
git clone https://github.com/nuros-linux/tulpar.git
cd tulpar
make
sudo make install
```

## Configuration

Default configuration file is located at `/etc/tulpar/tulpar.conf`

## Development

Requirements:
- C++17 compiler
- SQLite3
- libcurl
- OpenSSL
- systemd
- libarchive

## License

This project is licensed under the GPL 3 License - see the [LICENSE](LICENSE) file for details.

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Support

If you encounter any problems, please [open an issue](https://github.com/nuros-linux/Tulpar/issues).
