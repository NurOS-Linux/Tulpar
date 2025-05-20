/* NurOS/Tulpar/utils.cpp ruzen42 */
#include "utils.hpp"
#include "../colors.hpp"
#include "../parse_json/parse_json.hpp"

#include <iostream>
#include <iomanip>
#include <filesystem>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <openssl/md5.h>

using namespace parse_json;

namespace utils
{

auto directory_pkgs = "/var/log/tulpar/pkgs/";

std::string compute_MD5_from_file(const std::string &filePath)
{
    std::ifstream file(filePath, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        std::cerr << COLOR_RED << "Error: " << COLOR_RESET << "Cannot open file: " << filePath << "\n";
        return "";
    }

    long fileSize = file.tellg();
    char *memBlock = new (std::nothrow) char[fileSize];
    if (!memBlock)
    {
        std::cerr << COLOR_RED << "Error: " << COLOR_RESET << "Memory allocation failed\n";
        file.close();
        return "";
    }

    file.seekg(0, std::ios::beg);
    file.read(memBlock, fileSize);
    file.close();

    unsigned char result[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<unsigned char *>(memBlock), fileSize, result);
    delete[] memBlock;

    std::stringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(result[i]);
    }

    return ss.str();
}


void create_list()
{
    if (!std::filesystem::exists(directory_pkgs))
    {
        std::filesystem::create_directory(directory_pkgs);
        std::cout << COLOR_GREEN << "Finished: " << COLOR_RESET << "lists created\n";
    }
}

void add_package_to_list(std::string& pkg, std::string version)
{
    if (!std::filesystem::exists(directory_pkgs))
    {
        create_list();
    }

    try
    {
        std::filesystem::create_directory(directory_pkgs + pkg + "-" + version);
    }
    catch (std::filesystem::filesystem_error err)
    {
        std::cout << COLOR_RED << "Error: " << COLOR_RESET << err.what() << "\n";
    }
}

void check_root()
{
    auto username = geteuid();

    if (username != 0)
    {
        std::cout << COLOR_RED << "Error: " << COLOR_RESET << "Permission denied\n";
        std::exit(3);
    }
}

std::string find_name_package(std::string file)
{
    std::string name;

    for (unsigned int i = 0; i <= file.length(); ++i)
    {
        if (file[i] == '-')
        {
            break;
        }
        name += file[i];
    }

    return name;
}

bool ask_yn(const std::string& ask)
{
    char answer;
    std::cout << ask + "? [Y/n] ";
    std::cin >> answer;

    auto value = true;
    
    if (std::tolower(answer) == 'n')
    {
        std::cout << "Cancel\n";
        value = false;
    }
    return value;
}

void install_local_package(const std::string& file, const std::string& rootfs)
{
    if (!std::filesystem::exists(file))
    {
        std::cout << COLOR_RED << "Error: " << COLOR_RESET << "File does not exist: " << file << "\n";
        return;
    }

    const auto filename = std::filesystem::absolute(file);
    auto name = find_name_package(filename.filename().string());

    std::filesystem::create_directories("/tmp/tulpar/" + name);

    try
    {
        std::filesystem::copy(filename, "/tmp/tulpar/" + filename.filename().string(), std::filesystem::copy_options::overwrite_existing);
    }
    catch (const std::filesystem::filesystem_error& err)
    {
        std::cerr << COLOR_RED << "Error: " << COLOR_RESET << err.what() << "\n";
        return;
    }

    std::cout << "Packages to be installed: \n";
    std::vector<std::string> pkgs;
    pkgs.insert(pkgs.begin(), name);

    for (int i = 0; i < pkgs.size(); ++i)
    {
      std::cout << COLOR_GREEN << pkgs[i] << COLOR_RESET << "\n";
    }
    
    // Note: Replace with libarchive or safer alternative
    system(("tar xvf /tmp/tulpar/" + filename.filename().string() + " -C /tmp/tulpar/" + name + " >> /dev/null").c_str());

    std::string metadata = "/tmp/tulpar/" + name + "/metadata.json";

    if (!std::filesystem::exists(metadata))
    {
      std::cerr << COLOR_RED << "Error: " << COLOR_RESET << "Package are corrupted, metadata.json not found" << "\n";
      return;
    }
    utils::package data_package = parse_file("/tmp/tulpar/" + name + "/metadata.json");
    auto pkg_name = data_package.name;
    auto pkg_ver = data_package.version;



    try
    {
        std::cout << "Installing package\n";
        if (std::filesystem::exists("/tmp/tulpar/" + name + "/data"))
        {
            std::filesystem::copy("/tmp/tulpar/" + name + "/data/", rootfs, std::filesystem::copy_options::recursive);
        }
        else
        {
            std::cout << COLOR_RED << "Error: " << COLOR_RESET << "Data directory not found\n";
            return;
        }
    }
    catch (const std::filesystem::filesystem_error& err)
    {
        std::cout << COLOR_RED << "Error: " << COLOR_RESET << err.what() << "\n";
        return;
    }

    std::cout << COLOR_GREEN << "Finished: " << COLOR_RESET << "Installed without any errors\n";
}

void check_empty(std::string pkg)
{
    if (pkg.empty())
    {
        std::cerr << COLOR_RED << "Error: " << COLOR_RESET << "Package name cannot be empty" << "\n";
        return;
    }
}

void install_package(const std::string& pkg, const std::string& rootfs)
{
    check_empty(pkg);
    if (pkg.substr(0, 2) == "./")
    {
        install_local_package(pkg, rootfs);
        return;
    }



    std::cout << COLOR_GREEN << "" << pkg << "\n";
}

void remove_package(const std::string& pkg, const std::string& rootfs)
{
    check_empty(pkg);
    std::cout << COLOR_GREEN << "Good" << pkg << "\n";
}

void update_database()
{
    //not
}

void search_package(const std::string& pkg)
{
    check_empty(pkg);
}

void clean_cache()
{
    if (std::filesystem::exists("/tmp/tulpar") ||  std::filesystem::exists("/var/cache/tulpar"))
    {
        try
        {
            std::cout <<  "Removing /tmp/tulpar" << "\n";
            std::filesystem::remove_all("/tmp/tulpar/");
            std::cout <<  "Removing /var/cache/tulpar" << "\n";
            std::filesystem::remove_all("/var/cache/tulpar/");
        }
        catch (std::filesystem::filesystem_error err)
        {
            std::cout << COLOR_RED << "Error with cleaning: " << COLOR_RESET << err.what() << "\n";
            return;
        }
    }
    std::cout << COLOR_GREEN << "Finished: " << COLOR_RESET << "All done without errors\n";
}

void get_list(const std::string& what)
{
    std::string path;

    if (what == "installed")
    {
        path = directory_pkgs;
    }
    else if (what == "available")
    {
        path = directory_pkgs;
    }

    create_list();

    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        std::cout << entry.path().filename() << "\n";
    }
}
}
