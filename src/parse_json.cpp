/* NurOS/Tulpar/parse_json.cpp ruzen42 */
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

#include "include/utils.hpp"
#include "include/colors.hpp"

using namespace std;
using namespace utils;

namespace parse_json
{
package parse_file(const string& metadata)
{
    fstream input(metadata);
    package data_package;
    nlohmann::json json;
    input >> json;
    try
    {
        data_package.name = json["name"];
        data_package.architecture = json["architecture"];
        data_package.license = json["license"];
        data_package.description = json["description"];
        data_package.version = json["version"];
        data_package.maintainer = json["maintainer"];
        data_package.homepage = json["homepage"];
        data_package.dependencies = json["dependencies"].get<std::vector<std::string>>();
        data_package.conflicts = json["conflicts"].get<std::vector<std::string>>();
        data_package.provides = json["provides"].get<std::vector<std::string>>();
        data_package.replaces = json["replaces"].get<std::vector<std::string>>();
    }
    catch (string err)
    {
        cout << COLOR_RED << "Error with parsing metadata: " << COLOR_RESET << err << "\n";
    }

    std::string deps = "";

    for (int i {0}; i < data_package.dependencies.size(); ++i)
      deps = deps + ", " + data_package.dependencies[i];

    show_metadata(data_package);

    return data_package;
}

void show_metadata(const package& data_package)
{
    cout << "Data of package: \n";
    cout << "\tName: " << data_package.name << "\n";
    cout << "\tVersion: " << data_package.version << "\n";
    cout << "\tDescription: " << data_package.description << "\n";
    cout << "\tLicense: " << data_package.license << "\n";
    cout << "\tArch: " << data_package.architecture << "\n";
    cout << "\tDeps: " << deps << "\n";
    cout << "\tMaintainer: " << data_package.maintainer << "\n";
}
}

