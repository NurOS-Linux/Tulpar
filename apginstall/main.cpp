#include <iostream>
#include <argparse/argparse.hpp>

#define RED "\e[31m"
#define DEFAULT "\e[0m"
using namespace std;
using namespace argparse;

int main(int argc, char **argv) {
    ArgumentParser program("apginstall", "0.1.0",  default_arguments::all);

    program.add_argument("-f", "--force")
        .default_value(false)
        .implicit_value(true)
        .help("force install");

    program.add_argument("pkgs")
        .help("pkgs.apg to install")
        .remaining();

    vector<string> packages;

    try {
        program.parse_args(argc, argv);
    } catch (const runtime_error& err) {
        cerr << RED <<  "Error" << DEFAULT << err.what() << "\n";
        cerr << program << "\n";
        return 1;
    }

    try {
        packages = program.get<vector<string>>("pkgs");
    } catch (const exception& err) {
        cerr << RED << "Error: " << DEFAULT << "Please specify *.apg\n";
        cerr << program << "\n";
        return 1;
    }

    for (const auto& pkg : packages) {
        cout << "Installing: " << pkg << "\n";
    }

    return 0;
}
