Tulpar

![Alt text](./Tulpar.png)

Tulpar is a lightweight and flexible package manager designed for NurOS, a Linux distribution. It provides a simple command-line interface to manage software packages, including installation, removal, updating, and searching. Tulpar is built with C++ and utilizes the argparse library for handling command-line arguments.
Features

    Install Packages: Install software packages with optional root filesystem specification.
    Remove Packages: Uninstall packages from the system.
    Update Packages: Update the package database or specific packages.
    Search Packages: Search for packages in the database.
    Clean Cache: Clear the package cache to free up space.
    Customizable Root: Specify a custom root filesystem path for package operations.

Installation

To build and install Tulpar, follow these steps:

    Clone the Repository:
    bash

git clone https://github.com/NurOS-Linux/Tulpar.git
cd Tulpar
Install Dependencies: Ensure you have the following dependencies installed:

    A C++ compiler (e.g., g++)
    cmake for building the project
    argparse library (included or installed separately)

Build the Project:
bash
mkdir build && cd build
cmake ..
make
Install Tulpar:
bash

    sudo make install

Usage

Tulpar supports several subcommands for package management. Below is the basic syntax and available commands:
bash
tulpar <subcommand> [options]
Subcommands

    install: Install a package.
    bash

tulpar install <package_name> [--root <path>]
Example:
bash
tulpar install firefox --root /custom/root
remove: Remove a package.
bash
tulpar remove <package_name> [--root <path>]
Example:
bash
tulpar remove firefox
update: Update the package database or a specific package.
bash
tulpar update [<package_name>]
Example:
bash
tulpar update @world
search: Search for a package in the database.
bash
tulpar search <package_name>
Example:
bash
tulpar search firefox
clean: Clear the package cache.
bash

    tulpar clean

Options

    --root <path>: Specify a custom root filesystem path for installation or removal (default: /).
    --help: Display help information for the command or subcommand.
    --version: Display the version of Tulpar.

Example

To install a package named vim:
bash
tulpar install vim

To search for a package:
bash
tulpar search python

To clean the cache:
bash
tulpar clean
Contributing

Contributions to Tulpar are welcome! To contribute:

    Fork the repository.
    Create a new branch for your feature or bug fix.
    Submit a pull request with a clear description of your changes.

Please ensure your code follows the project's coding style and includes appropriate tests.
License

Tulpar is licensed under the MIT License. See the LICENSE file for details.
Contact

For issues, suggestions, or questions, please open an issue on the GitHub repository or contact the NurOS team.
