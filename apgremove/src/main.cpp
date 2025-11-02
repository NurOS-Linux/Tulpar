#include <fstream>
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
    ArgumentParser program("apgremove", "0.1.0",  default_arguments::all);

    bool force = false;

    program.add_argument("-f", "--force")
        .default_value(false)
        .implicit_value(true)
        .help("force install");

    program.add_argument("-r", "--root")
        .help("specify root directory")
        .default_value(std::string("/"));

    program.add_argument("PKGS")
        .help("pkg name to remove")
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
        pkgsFilenames = program.get<std::vector<std::string>>("PKGS");
        root = program.get<std::string>("root");
        force = program.get<bool>("force");
    }
    catch (const std::exception &ex)
    {
        Logger::LogError("Specify a pkg");
        std::cerr << program << "\n";
        return 1;
    }

    std::vector<ApgPackage> pkgs;
    for (const auto &pkg : pkgsFilenames)
    {
        fs::path path = pkg;
        pkgs.emplace_back(path, true);
    }

    auto db = ApgDb();
    try
    {
        fs::path dbPath = "/var/lib/tulpar/";
        if (root != "/") dbPath = root / dbPath;
        fs::create_directory(dbPath);
        db = ApgDb(dbPath.string() + "local.db");
    }
    catch (const std::exception& err)
    {
       Logger::LogError("Error while opening db: " + static_cast<std::string>(err.what()));
    }

    for (auto &pkg : pkgs)
    {
        Logger::LogInfo("Remove Package in " + root.string());
        try
        {
            pkg.Remove(db, root);
        }
        catch (const std::exception &err)
        {
            Logger::LogError("Can't install package: " + static_cast<std::string>(err.what()));
        }
    }

    db.Close();

    return 0;
}
