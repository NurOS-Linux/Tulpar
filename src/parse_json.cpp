/* NurOS/Tulpar/parse_json.cpp ruzen42 */
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

#include "../utils/utils.hpp"
#include "../colors.hpp"

namespace parse_json
{
utils::package parse_file(std::string metadata)
{
    std::fstream input(metadata);
    utils::package data_package;
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
    catch (std::string err)
    {
        std::cout << COLOR_RED << "Error with parsing metadata: " << COLOR_RESET << err << "\n";
    }

    std::string deps = "";

    for (int i {0}; i < data_package.dependencies.size(); ++i)
      deps = deps + ", " + data_package.dependencies[i];
    /*
    std::cout << "Data of package: \n";
    std::cout << "\tName: " << data_package.name << "\n";
    std::cout << "\tVersion: " << data_package.version << "\n";
    std::cout << "\tDescription: " << data_package.description << "\n";
    std::cout << "\tLicense: " << data_package.license << "\n";
    std::cout << "\tArch: " << data_package.architecture << "\n";
    std::cout << "\tDeps: " << deps << "\n";
    std::cout << "\tMaintainer: " << data_package.maintainer << "\n";
    */
    return data_package;
}
}

