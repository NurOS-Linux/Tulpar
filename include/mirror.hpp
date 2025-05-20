/* NurOS/Tulpar/mirror.hpp ruzen42 */
#pragma once

#ifndef MIRROR_HPP 
#define MIRROR_HPP

#include <string>
#include <vector>

namespace mirror
{
struct repo 
{
  std::string protocol;
  std::string url;
  int port;
};

std::vector<mirror::repo> parse_config(const std::string& file);
void download_pkg(std::string pkg, mirror::repo repo);
void get_list_of_available(mirror::repo repo, const std::string& path_to_save);
}

#endif
