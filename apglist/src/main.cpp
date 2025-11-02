// NurOS Ruzen42 2025
#include <iostream>
#include <vector>
#include <Apg/ApgPackage.hpp>
#include <Apg/Logger.hpp>
#include <argparse/argparse.hpp>

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

    program.add_argument("-r", "--root")
        .help("specify root directory")
        .default_value(std::string("/"));

    program.add_argument("-n", "--only-number")
        .default_value(false)
        .implicit_value(true)
        .help("show only number of packages");


    fs::path root;

    try
    {
        program.parse_args(argc, argv);
        root = program.get<std::string>("root");
    }
    catch (...)
    {
        Logger::LogError("error while parsing args");
        return 1;
    }

    try
    {
        const fs::path path = root / "/var/lib/tulpar/local.db";
        const auto db = ApgDb(path);
        for (const auto pkgs = ApgPackage::LoadAllFromDb(db); const auto& package : pkgs)
        {
           std::cout << package.toString() << " " << package.getDescription() << "\n";
        }
    }
    catch (const std::exception& err)
    {
        Logger::LogError(err.what());
        return 1;
    }

    return 0;
}
