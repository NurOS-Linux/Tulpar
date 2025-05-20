/* NurOS/Tulpar/mirror.cpp ruzen42 */
#include "mirror.hpp"

#include <iostream>
#include <vector>
#include <fstream>

namespace mirror 
{
  std::vector<mirror::repo> parse_config(std::string file)
  {
    std::vector<mirror::repo> value;
    std::ifstream f(file);
    f.seekg(0, std::ios::end);
    size_t size = f.tellg();
    std::string s(size, ' ');
    f.seekg(0);
    f.read(&s[0], size);
    std::cout << s;
    return value;
  }
} 
