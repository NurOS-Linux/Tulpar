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
std::string find_name_package(const std::string file);
std::string compute_MD5_from_file(const std::string& file);
void create_list();
void add_package_to_list(std::string& pkg);
void check_root();
void install_package(const std::string& pkg, const std::string& root);
void remove_package(const std::string& pkg, const std::string& root);
void update_database();
void search_package(const std::string& pkg);
void clean_cache();
}

#endif
