#include <iostream>
#include <vector>
#include <argparse/argparse.hpp>
#include <apg_package/ApgPackage.hpp>

#define RED "\e[31m"
#define DEFAULT "\e[0m"

using namespace argparse;

int main(const int argc, char **argv)
{
    ArgumentParser program("apginstall", "0.1.0",  default_arguments::all);

    program.add_argument("-f", "--force")
        .default_value(false)
        .implicit_value(true)
        .help("force install");

    program.add_argument("pkgs")
        .help("pkgs.apg to install")
        .remaining();

    std::vector<std::string> packages;

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err)
    {
        std::cerr << RED <<  "Error" << DEFAULT << err.what() << "\n";
        std::cerr << program << "\n";
        return 1;
    }

    try
    {
        packages = program.get<std::vector<std::string>>("pkgs");
    }
    catch (const std::exception& err)
    {
        std::cerr << RED << "Error: " << DEFAULT << "Please specify *.apg\n" << " " << err.what() << "\n";
        std::cerr << program << "\n";
        return 1;
    }

    const auto pkg = new ApgPackage();

    for (const auto& pkgsFilesNames : packages)
    {
        std::cout << "Installing: " << pkgsFilesNames << "\n";
        std::cout << pkg->toString() << "\n";
    }

    delete pkg;

    return 0;
}
