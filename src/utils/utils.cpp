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
    if (!std::filesystem::exists("/var/lib/tulpar"))
    {
        std::filesystem::create_directory("/var/lib/tulpar");
    }

    std::ofstream list("/var/lib/tulpar/list.json");
    std::ofstream list_installed("/var/lib/tulpar/list_installed.json");

    if (!list.is_open() || !list_installed.is_open())
    {
        std::cerr << COLOR_RED << "Error: " << COLOR_RESET << "Failed to create list files\n";
        return;
    }

    list << nlohmann::json::object().dump(4);
    list_installed << nlohmann::json::array().dump(4);

    list.close();
    list_installed.close();

    std::cout << COLOR_GREEN << "Finished: " << COLOR_RESET << "lists created\n";
}

void add_package_to_list(std::string& pkg)
{
    if (!std::filesystem::exists("/var/lib/tulpar/list.json"))
    {
        create_list();
    }

    nlohmann::json json_array;
    std::ifstream input_file("/var/lib/tulpar/list_installed.json");
    if (input_file.is_open())
    {
        try
        {
            input_file >> json_array;
        }
        catch (const nlohmann::json::parse_error& e)
        {
            std::cerr << COLOR_RED << "Error: " << COLOR_RESET << "Invalid JSON in list_installed.json: " << e.what() << "\n";
        }
        input_file.close();
    }

    if (!json_array.is_array())
    {
        json_array = nlohmann::json::array();
    }

    json_array.push_back(pkg);

    std::ofstream output_file("/var/lib/tulpar/list_installed.json");
    if (!output_file.is_open())
    {
        std::cerr << COLOR_RED << "Error: " << COLOR_RESET << "Cannot open list_installed.json for writing\n";
        return;
    }

    output_file << json_array.dump(4);
    output_file.close();
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

void install_local_package(const std::string& file, const std::string& rootfs)
{
    const auto filename = std::filesystem::absolute(file);
    auto name = find_name_package(filename.filename().string());

    if (!std::filesystem::exists(filename))
    {
        std::cout << COLOR_RED << "Error: " << COLOR_RESET << "File does not exist: " << filename << "\n";
        return;
    }

    std::filesystem::create_directories("/tmp/tulpar/" + name);

    try
    {
        std::filesystem::copy(filename, "/tmp/tulpar/" + filename.filename().string(), std::filesystem::copy_options::overwrite_existing);
    }
    catch (const std::filesystem::filesystem_error& err)
    {
        std::cout << COLOR_RED << "Error: " << COLOR_RESET << err.what() << "\n";
        return;
    }

    std::cout << "Extracting tar archive to /tmp/tulpar/" + name + "\n";
    // Note: Replace with libarchive or safer alternative
    system(("tar xvf /tmp/tulpar/" + filename.filename().string() + " -C /tmp/tulpar/" + name + " >> /dev/null").c_str());

    utils::package data_package = parse_file("/tmp/tulpar/" + name + "/metadata.json");
    auto pkg_name = data_package.name;

    std::cout << COLOR_GREEN << "Finished: " << COLOR_RESET << "Extracting tar archive\n";

    check_root();
    std::cout << "Package to be installed: " << COLOR_GREEN << pkg_name << COLOR_RESET << "\n";

    char answer;
    std::cout << "Proceed install package " + name + "? [" << COLOR_GREEN << "Y" << COLOR_RESET << "/" << COLOR_RED << "n" << COLOR_RESET << "] ";
    std::cin >> answer;

    if (std::tolower(answer) == 'n')
    {
        std::cout << "Cancel installation\n";
        return;
    }

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

    std::cout << COLOR_GREEN << "Finished: " << COLOR_RESET << "Installed with Success\n";
}

void install_package(const std::string& pkg)
{
    check_root();
    if (pkg.empty())
    {
        std::cerr << COLOR_RED << "Error: " << COLOR_RESET << "Package name cannot be empty" << "\n";
        return;
    }
    std::cout << COLOR_GREEN << "" << pkg << "\n";
}

void remove_package(const std::string& pkg)
{
    check_root();
    if (pkg.empty())
    {
        std::cerr << COLOR_RED << "Error: " << COLOR_RESET << "Package name cannot be empty" << "\n";
        return;
    }
    std::cout << COLOR_GREEN << "Good" << pkg << "\n";
}

void update_package(const std::string& pkg)
{
    check_root();
    if (pkg.empty())
    {
        std::cerr << COLOR_RED << "Error: " << COLOR_RESET << "Package name cannot be empty" << "\n";
        return;
    }
    std::cout << COLOR_GREEN << "Good" << pkg << "\n";
}

void search_package(const std::string& pkg)
{
    if (pkg.empty())
    {
        std::cerr << COLOR_RED << "Error: " << COLOR_RESET << "Package name cannot be empty" << "\n";
        return;
    }
    std::cout << COLOR_GREEN << "Good" << pkg << "\n";
}

void clean_cache()
{
    check_root();
    std::cout << COLOR_GREEN << "Cleaning..." << COLOR_RESET << "\n";
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
    std::cout << COLOR_GREEN << "All done without errors" << COLOR_RESET << "\n";
}
}
