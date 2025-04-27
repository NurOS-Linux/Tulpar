/* NurOS/Tulpar/utils.cpp ruzen42 */
#include "utils.hpp"
#include "../colors.hpp"
#include "../parse_json/parse_json.hpp"
//#include "../tarxx.h"

#include <iostream>
#include <filesystem>

using namespace parse_json;

namespace utils 
{
  std::string find_name_package(std::string file)
  {
    std::string name;

    for (unsigned int i = 0;i <= file.length(); ++i)
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
    auto name = find_name_package(file);
    std::filesystem::create_directory("/tmp/tulpar/");
    std::filesystem::create_directory("/tmp/tulpar/" + name);

    if (std::filesystem::exists("/tmp/tulpar/" + file))
    {
      std::filesystem::remove("/tmp/tulpar/" + file);
    }

    try
    {
      std::filesystem::copy(filename, "/tmp/tulpar");
    }
    catch (const std::filesystem::filesystem_error& err)
    {
      std::cout << COLOR_RED << "Error: " << COLOR_RESET << err.what() << "\n";
      return;
    }

    system(("tar xvf /tmp/tulpar/" + file + " -C /tmp/tulpar/" + name).c_str());

    utils::package data_package = parse_file("/tmp/tulpar/" + name + "/metadata.json");

    std::filesystem::copy("/tmp/tulpar/" + name + "/data/", rootfs, std::filesystem::copy_options::recursive);
  }

  void install_package(const std::string& pkg) 
  {
    if (pkg.empty())
    {
      std::cerr << COLOR_RED << "Error: " << COLOR_RESET << "Package name cannot be empty" << "\n";
      return;
    }
    std::cout << COLOR_GREEN << "" << pkg << "\n";
  }

  void remove_package(const std::string& pkg) 
  {
    if (pkg.empty())
    {
      std::cerr << COLOR_RED << "Error: " << COLOR_RESET << "Package name cannot be empty" << "\n";
      return;
    }
    std::cout << COLOR_GREEN << "Good" << pkg << "\n";
  }

    void update_package(const std::string& pkg)
    {
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
