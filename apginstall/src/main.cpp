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

    bool checkSums;
    bool force = false;

    program.add_argument("-f", "--force")
        .default_value(false)
        .implicit_value(true)
        .help("force install");

    program.add_argument("--no-check-md5sums")
        .default_value(false)
        .implicit_value(true)
        .help("skip checking md5sums (RISK)");

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
        checkSums = !program.get<bool>("no-check-md5sums");
        force = program.get<bool>("force");
        if (force) checkSums = false;
    }
    catch (...)
    {
        Logger::LogError("Specify a *.apg");
        std::cerr << program << "\n";
        return 1;
    }

    std::vector<ApgPackage> pkgs;
    for (const auto &pkg : pkgsFilenames)
    {
        fs::path path = pkg;
        pkgs.emplace_back(path, true);

    }
    auto db = LmdbDb();
    try
    {
        const fs::path dbPath = root.string() + "/" + "var/lib/tulpar/";\
        fs::create_directory(dbPath);
        db = LmdbDb(dbPath.string() + "local.db");
    }
    catch (const std::exception& err)
    {
       Logger::LogError("Error while opening db: " + static_cast<std::string>(err.what()));
    }

    for (auto &pkg : pkgs)
    {
        Logger::LogInfo("Installing Package in " + root.string());
        try
        {
            pkg.Install(std::move(db), checkSums, root);
        }
        catch (const std::exception &err)
        {
            Logger::LogError("Can't install package: " + static_cast<std::string>(err.what()));
        }
    }

    db.Close();

    return 0;
}
