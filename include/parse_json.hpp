/* NurOS/Tulpar/parse_json.hpp ruzen42 */
#pragma once
#ifndef PARSE_JSON_HPP

#define PARSE_JSON_HPP

#include "utils.hpp"

using namespace std;
using namespace utils;

namespace parse_json
{
package parse_file(const string& metadata);
void parse_file(const utils& data_package);
}

#endif
