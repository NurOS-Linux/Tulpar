/* NurOS/Tulpar/utils.cpp ruzen42 */
#include "utils.hpp"
#include "../colors.hpp"
#include "../parse_json/parse_json.hpp"

#include <iostream>
#include <filesystem>
#include <unistd.h>

using namespace parse_json;

namespace utils 
{
    void check_root()
    {
        auto username = geteuid();

        if (username != 0)
        {
            std::cout << username << "\n";
            std::cout << COLOR_RED << "Error: " << COLOR_RESET << "Permission denied\n";
            std::exit(3);
        }
    }

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
        check_root();

        char answer;
        std::cout << "Proceed install package " + name + "? [Y/n] ";

        std::cin >> answer;

        if (std::tolower(answer) == 'n')
        {
            std::cout << "Cancel installation" << "\n";
            return;
        }


        if (!std::filesystem::exists("/tmp/tulpar/"))
        {
            std::filesystem::create_directory("/tmp/tulpar/");
        }

        if (!std::filesystem::exists("/tmp/tulpar/" + name))
        {
           std::filesystem::create_directory("/tmp/tulpar/" + name);
        }

        try
        {
            if (!std::filesystem::exists("/tmp/tulpar/" + file))
            {
                std::filesystem::copy(filename, "/tmp/tulpar");
            }
        }
        catch (const std::filesystem::filesystem_error& err)
        {
            std::cout << COLOR_RED << "Error: " << COLOR_RESET << err.what() << "\n";
            return;
        }

        try
        {
            // Deprecated:
            system(("tar xvf /tmp/tulpar/" + file + " -C /tmp/tulpar/" + name + " >> /dev/null").c_str());
        }
        catch (std::system_error err)
        {
            std::cout << COLOR_RED << "Error: " << COLOR_RESET << err.what() << "\n";
            return;
        }

        utils::package data_package = parse_file("/tmp/tulpar/" + name + "/metadata.json");

        std::filesystem::copy("/tmp/tulpar/" + name + "/data/", rootfs, std::filesystem::copy_options::recursive);

        std::cout << COLOR_GREEN << "Installed with Success" << "\n";
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
