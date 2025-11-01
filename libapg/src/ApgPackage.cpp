// NurOS Ruzen42 2025
#include "apg_package/ApgPackage.hpp"
#include <iostream>

ApgPackage::ApgPackage(
    std::string name,
    std::string version,
    std::string architecture,
    std::string description,
    std::string maintainer,
    std::string license,
    std::string homepage,
    std::vector<std::string> dependencies,
    std::vector<std::string> conflicts,
    std::vector<std::string> provides,
    std::vector<std::string> replaces
) : name(std::move(name)),
    version(std::move(version)),
    architecture(std::move(architecture)),
    description(std::move(description)),
    maintainer(std::move(maintainer)),
    license(std::move(license)),
    homepage(std::move(homepage)),
    dependencies(std::move(dependencies)),
    conflicts(std::move(conflicts)),
    provides(std::move(provides)),
    replaces(std::move(replaces))
{
}

void ApgPackage::fromJson(const nlohmann::json &j)
{
    name = j.value("name", "");
    version = j.value("version", "");
    architecture = j.value("architecture", "");
    description = j.value("description", "");
    maintainer = j.value("maintainer", "");
    license = j.value("license", "");
    homepage = j.value("homepage", "");

    if (j.contains("dependencies"))
    {
        dependencies = j["dependencies"].get<std::vector<std::string>>();
    }
    if (j.contains("conflicts"))
    {
        conflicts = j["conflicts"].get<std::vector<std::string>>();
    }
    if (j.contains("provides"))
    {
        provides = j["provides"].get<std::vector<std::string>>();
    }
    if (j.contains("replaces"))
    {
        replaces = j["replaces"].get<std::vector<std::string>>();
    }
}

nlohmann::json ApgPackage::toJson() const
{
    nlohmann::json j;
    j["name"] = name;
    j["version"] = version;
    j["architecture"] = architecture;
    j["description"] = description;
    j["maintainer"] = maintainer;
    j["license"] = license;
    j["homepage"] = homepage;

    if (!dependencies.empty())
    {
        j["dependencies"] = dependencies;
    }
    if (!conflicts.empty())
    {
        j["conflicts"] = conflicts;
    }
    if (!provides.empty())
    {
        j["provides"] = provides;
    }
    if (!replaces.empty())
    {
        j["replaces"] = replaces;
    }

    return j;
}

const std::string& ApgPackage::getName() const { return name; }
const std::string& ApgPackage::getVersion() const { return version; }
const std::string& ApgPackage::getArchitecture() const { return architecture; }
const std::string& ApgPackage::getDescription() const { return description; }
const std::string& ApgPackage::getMaintainer() const { return maintainer; }
const std::string& ApgPackage::getLicense() const { return license; }
const std::string& ApgPackage::getHomepage() const { return homepage; }
const std::vector<std::string>& ApgPackage::getDependencies() const { return dependencies; }
const std::vector<std::string>& ApgPackage::getConflicts() const { return conflicts; }
const std::vector<std::string>& ApgPackage::getProvides() const { return provides; }
const std::vector<std::string>& ApgPackage::getReplaces() const { return replaces; }

void ApgPackage::setName(const std::string& newName) { name = newName; }
void ApgPackage::setVersion(const std::string& newVersion) { version = newVersion; }
void ApgPackage::setArchitecture(const std::string& newArch) { architecture = newArch; }
void ApgPackage::setDescription(const std::string& newDesc) { description = newDesc; }
void ApgPackage::setMaintainer(const std::string& newMaintainer) { maintainer = newMaintainer; }
void ApgPackage::setLicense(const std::string& newLicense) { license = newLicense; }
void ApgPackage::setHomepage(const std::string& newHomepage) { homepage = newHomepage; }
void ApgPackage::setDependencies(const std::vector<std::string>& newDeps) { dependencies = newDeps; }
void ApgPackage::setConflicts(const std::vector<std::string>& newConflicts) { conflicts = newConflicts; }
void ApgPackage::setProvides(const std::vector<std::string>& newProvides) { provides = newProvides; }
void ApgPackage::setReplaces(const std::vector<std::string>& newReplaces) { replaces = newReplaces; }

void ApgPackage::addDependency(const std::string& dep) { dependencies.push_back(dep); }
void ApgPackage::addConflict(const std::string& conflict) { conflicts.push_back(conflict); }
void ApgPackage::addProvide(const std::string& provide) { provides.push_back(provide); }
void ApgPackage::addReplace(const std::string& replace) { replaces.push_back(replace); }

bool ApgPackage::hasDependencies() const { return !dependencies.empty(); }
bool ApgPackage::hasConflicts() const { return !conflicts.empty(); }
bool ApgPackage::hasProvides() const { return !provides.empty(); }
bool ApgPackage::hasReplaces() const { return !replaces.empty(); }

void ApgPackage::clearDependencies() { dependencies.clear(); }
void ApgPackage::clearConflicts() { conflicts.clear(); }
void ApgPackage::clearProvides() { provides.clear(); }
void ApgPackage::clearReplaces() { replaces.clear(); }

std::string ApgPackage::toString() const
{
    std::string result = "Package: " + name + " v" + version + "\n";
    result += "Arch: " + architecture + "\n";
    result += "Description: " + description + "\n";
    result += "Maintainer: " + maintainer + "\n";
    result += "License: " + license + "\n";
    result += "Homepage: " + homepage + "\n";

    if (!dependencies.empty())
    {
        result += "Dependencies: " + vectorToString(dependencies) + "\n";
    }
    if (!conflicts.empty())
    {
        result += "Conflicts: " + vectorToString(conflicts) + "\n";
    }
    if (!provides.empty())
    {
        result += "Provides: " + vectorToString(provides) + "\n";
    }
    if (!replaces.empty())
    {
        result += "Replaces: " + vectorToString(replaces) + "\n";
    }

    return result;
}

std::string ApgPackage::vectorToString(const std::vector<std::string>& vec)
{
    std::string result;
    for (size_t i = 0; i < vec.size(); ++i)
    {
        result += vec[i];
        if (i < vec.size() - 1)
        {
            result += ", ";
        }
    }
    return result;
}