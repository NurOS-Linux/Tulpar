// NurOS Ruzen42 2025
#ifndef APG_PACKAGE_HPP
#define APG_PACKAGE_HPP

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct PackageMetadata
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

class ApgPackage
{
    PackageMetadata metadata;
    bool installedByHand{};

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
        std::vector<std::string> replaces = {},
        bool installedByHand = true
    );

    void fromJson(const nlohmann::json &j);
    [[nodiscard]] nlohmann::json toJson() const;
    [[nodiscard]] std::string toString() const;

    // Metadata accessors
    [[nodiscard]] const PackageMetadata& getMetadata() const;
    void setMetadata(const PackageMetadata& newMetadata);

    [[nodiscard]] const std::string& getName() const;
    [[nodiscard]] const std::string& getVersion() const;
    [[nodiscard]] const std::string& getArchitecture() const;
    [[nodiscard]] const std::string& getDescription() const;
    [[nodiscard]] const std::string& getMaintainer() const;
    [[nodiscard]] const std::string& getLicense() const;
    [[nodiscard]] const std::string& getHomepage() const;

    void setName(const std::string& newName);
    void setVersion(const std::string& newVersion);
    void setArchitecture(const std::string& newArch);
    void setDescription(const std::string& newDesc);
    void setMaintainer(const std::string& newMaintainer);
    void setLicense(const std::string& newLicense);
    void setHomepage(const std::string& newHomepage);

    [[nodiscard]] const std::vector<std::string>& getDependencies() const;
    [[nodiscard]] const std::vector<std::string>& getConflicts() const;
    [[nodiscard]] const std::vector<std::string>& getProvides() const;
    [[nodiscard]] const std::vector<std::string>& getReplaces() const;

    void setDependencies(const std::vector<std::string>& newDeps);
    void setConflicts(const std::vector<std::string>& newConflicts);
    void setProvides(const std::vector<std::string>& newProvides);
    void setReplaces(const std::vector<std::string>& newReplaces);

    void addDependency(const std::string& dep);
    void addConflict(const std::string& conflict);
    void addProvide(const std::string& provide);
    void addReplace(const std::string& replace);

    [[nodiscard]] bool hasDependencies() const;
    [[nodiscard]] bool hasConflicts() const;
    [[nodiscard]] bool hasProvides() const;
    [[nodiscard]] bool hasReplaces() const;

    void clearDependencies();
    void clearConflicts();
    void clearProvides();
    void clearReplaces();

    [[nodiscard]] bool isInstalledByHand() const;
    void setInstalledByHand(bool installed);

private:
    static std::string vectorToString(const std::vector<std::string>& vec);
};

#endif