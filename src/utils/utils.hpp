/* NurOS/Tulpar/utils.hpp ruzen42 */
#pragma once

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

namespace utils 
{
  struct package 
  {
    std::string name;
    std::string version;
    std::string architecture;
    std::string description;
    std::string maintainer;
    std::string license;
    std::string homepage;
    std::vector<std::string> dependencies;
    std::vector<std::string> conflicts;
    std::vector<std::string> provides;
    std::vector<std::string> replaces;
  };

  std::string find_package_name(const std::string file);
  void install_package(const std::string& pkg);
  void install_local_package(const std::string& pkg, const std::string& rootfs);
  void remove_package(const std::string& pkg);
  void update_package(const std::string& pkg);
  void search_package(const std::string& pkg);
}

#endif
