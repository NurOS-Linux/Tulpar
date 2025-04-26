/* NurOS/Tulpar/utils.cpp ruzen42 */
#include "utils.hpp"
#include "colors.hpp"

#include <iostream>
#include <filesystem>

namespace utils 
{
  void install_local_package(const std::string& file)
  {
    std::cout << std::filesystem::absolute(file);
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
}
