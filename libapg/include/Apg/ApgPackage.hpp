// NurOS Ruzen42 2025
#ifndef APG_PACKAGE_HPP
#define APG_PACKAGE_HPP

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "Apg/ApgLmdbDb.hpp"

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
    std::filesystem::path path;
    std::vector<std::filesystem::path> files;

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
        std::filesystem::path path = {},
        bool installedByHand = true
    );

    ApgPackage(std::string name, std::string version, std::string architecture, std::string description,
               std::string maintainer, std::string license, std::string homepage, std::vector<std::string> dependencies,
               std::vector<std::string> conflicts, std::vector<std::string> provides, std::vector<std::string> replaces,
               bool installedByHand, std::filesystem::path path);
    ApgPackage(const std::filesystem::path &path, bool installedByHand);
    void fromJson(const nlohmann::json &j);
    [[nodiscard]] nlohmann::json toJson() const;
    [[nodiscard]] std::string toString() const;

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
    void setMaintainer(const std::string& newMain);
    void setLicense(const std::string& newLic);
    void setHomepage(const std::string& newHome);

    void WriteToDb(const ApgDb &db) const;
    static std::vector<ApgPackage> LoadAllFromDb(const ApgDb& db);
    static std::optional<ApgPackage> LoadFromDb(const ApgDb& db, const std::string& name);
    bool RemoveFromDb(const ApgDb &db) const;
    bool Install(ApgDb db, bool checkSums, const std::string &root);

    bool Remove(const ApgDb &db, const fs::path &root) const;
};

#endif // APG_PACKAGE_HPP