#include <iostream>
#include <vector>
#include <Apg/ApgLmdbDb.hpp>
#include <Apg/ApgPackage.hpp>
#include <Apg/Logger.hpp>
#include <argparse/argparse.hpp>

using namespace argparse;
namespace fs = std::filesystem;

int main(const int argc, char **argv)
{
    ArgumentParser program("apginstall", "0.1.0",  default_arguments::all);

    program.add_argument("-f", "--force")
        .default_value(false)
        .implicit_value(true)
        .help("force install");

    program.add_argument("-r", "--root")
        .help("specify root directory")
        .default_value(std::string("/"));

    program.add_argument("FILES")
        .help("pkg.apg to install")
        .remaining();

    std::vector<std::string> pkgsFilenames;
    fs::path root;

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err)
    {
        Logger::LogError(err.what());
        std::cerr << program << "\n";
        return 1;
    }

    try
    {
        pkgsFilenames = program.get<std::vector<std::string>>("FILES");
        root = program.get<std::string>("root");
    }
    catch (...)
    {
        Logger::LogError("Specify a *.apg");
        std::cerr << program << "\n";
        return 1;
    }

    std::vector<ApgPackage> pkgs;
    fs::path path = pkgsFilenames[0];
    pkgs.emplace_back(path, true);
    const fs::path dbPath = "/var/lib/tulpar/local.db";
    fs::create_directory(dbPath);
    auto db = LmdbDb(dbPath.string());
    pkgs[0].Install(std::move(db), root);

    return 0;
}
