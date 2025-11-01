// NurOS Ruzen42 2025
#ifndef APG_PACKAGE_HPP
#define APG_PACKAGE_HPP

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class ApgPackage
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

public:
    ApgPackage() = default;

    ApgPackage(
        std::string name,
        std::string version,
        std::string architecture,
        std::string description,
        std::string maintainer,
        std::string license,
        std::string homepage,
        std::vector<std::string> dependencies = {},
        std::vector<std::string> conflicts = {},
        std::vector<std::string> provides = {},
        std::vector<std::string> replaces = {}
    );

    void fromJson(const nlohmann::json &j);
    nlohmann::json toJson() const;
    std::string toString() const;

    const std::string& getName() const;
    const std::string& getVersion() const;
    const std::string& getArchitecture() const;
    const std::string& getDescription() const;
    const std::string& getMaintainer() const;
    const std::string& getLicense() const;
    const std::string& getHomepage() const;
    const std::vector<std::string>& getDependencies() const;
    const std::vector<std::string>& getConflicts() const;
    const std::vector<std::string>& getProvides() const;
    const std::vector<std::string>& getReplaces() const;

    void setName(const std::string& newName);
    void setVersion(const std::string& newVersion);
    void setArchitecture(const std::string& newArch);
    void setDescription(const std::string& newDesc);
    void setMaintainer(const std::string& newMaintainer);
    void setLicense(const std::string& newLicense);
    void setHomepage(const std::string& newHomepage);
    void setDependencies(const std::vector<std::string>& newDeps);
    void setConflicts(const std::vector<std::string>& newConflicts);
    void setProvides(const std::vector<std::string>& newProvides);
    void setReplaces(const std::vector<std::string>& newReplaces);

    void addDependency(const std::string& dep);
    void addConflict(const std::string& conflict);
    void addProvide(const std::string& provide);
    void addReplace(const std::string& replace);

    bool hasDependencies() const;
    bool hasConflicts() const;
    bool hasProvides() const;
    bool hasReplaces() const;

    void clearDependencies();
    void clearConflicts();
    void clearProvides();
    void clearReplaces();

private:
    static std::string vectorToString(const std::vector<std::string>& vec);
};

#endif