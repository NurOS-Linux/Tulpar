#include <iostream>
#include <vector>
#include <argparse/argparse.hpp>
#include <apg_package/ApgPackage.hpp>

#define RED "\e[31m"
#define DEFAULT "\e[0m"

using namespace argparse;

int main(const int argc, char **argv)
{
    ArgumentParser program("apglist", "0.1.0",  default_arguments::all);

    program.add_argument("-v", "--verbose")
        .default_value(false)
        .implicit_value(true)
        .help("more details");

    program.add_argument("-i", "--installed")
        .default_value(false)
        .implicit_value(true)
        .help("show packages installed in your system");

    program.add_argument("-n", "--only-number")
        .default_value(false)
        .implicit_value(true)
        .help("show only number of packages");


    std::vector<ApgPackage> packages;

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
        packages.clear();
    }
    catch (const std::exception& err)
    {
        std::cerr << RED << "Error: " << DEFAULT << err.what() << "\n";
        std::cerr << program << "\n";
        return 1;
    }

    return 0;
}
