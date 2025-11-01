// NurOS Ruzen42 2025
#include "apg/ApgPackage.hpp"
#include <sstream>
#include <utility>

#include "apg/ApgLmdbDb.hpp"

using nlohmann::json;

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
    std::vector<std::string> replaces,
    const bool installedByHand,
    std::filesystem::path path
)
    : metadata
    {
        std::move(name),
        std::move(version),
        std::move(architecture),
        std::move(description),
        std::move(maintainer),
        std::move(license),
        std::move(homepage),
        std::move(dependencies),
        std::move(conflicts),
        std::move(provides),
        std::move(replaces)
    },
    installedByHand(installedByHand),
    path(std::move(path))
{
}

void ApgPackage::fromJson(const json& j)
{
    metadata.name          = j.value("name", "");
    metadata.version       = j.value("version", "");
    metadata.architecture  = j.value("architecture", "");
    metadata.description   = j.value("description", "");
    metadata.maintainer    = j.value("maintainer", "");
    metadata.license       = j.value("license", "");
    metadata.homepage      = j.value("homepage", "");
    metadata.dependencies  = j.value("dependencies", std::vector<std::string>{});
    metadata.conflicts     = j.value("conflicts", std::vector<std::string>{});
    metadata.provides      = j.value("provides", std::vector<std::string>{});
    metadata.replaces      = j.value("replaces", std::vector<std::string>{});
}

json ApgPackage::toJson() const
{
    return
{
        {"name", metadata.name},
        {"version", metadata.version},
        {"architecture", metadata.architecture},
        {"description", metadata.description},
        {"maintainer", metadata.maintainer},
        {"license", metadata.license},
        {"homepage", metadata.homepage},
        {"dependencies", metadata.dependencies},
        {"conflicts", metadata.conflicts},
        {"provides", metadata.provides},
        {"replaces", metadata.replaces}
    };
}

std::string ApgPackage::toString() const
{
    std::ostringstream out;
    out << metadata.name << " "
        << metadata.version << " ("
        << metadata.architecture << ")";
    return out.str();
}

const PackageMetadata& ApgPackage::getMetadata() const { return metadata; }
void ApgPackage::setMetadata(const PackageMetadata& newMetadata) { metadata = newMetadata; }

const std::string& ApgPackage::getName() const { return metadata.name; }
const std::string& ApgPackage::getVersion() const { return metadata.version; }
const std::string& ApgPackage::getArchitecture() const { return metadata.architecture; }
const std::string& ApgPackage::getDescription() const { return metadata.description; }
const std::string& ApgPackage::getMaintainer() const { return metadata.maintainer; }
const std::string& ApgPackage::getLicense() const { return metadata.license; }
const std::string& ApgPackage::getHomepage() const { return metadata.homepage; }

void ApgPackage::setName(const std::string& newName) { metadata.name = newName; }
void ApgPackage::setVersion(const std::string& newVersion) { metadata.version = newVersion; }
void ApgPackage::setArchitecture(const std::string& newArch) { metadata.architecture = newArch; }
void ApgPackage::setDescription(const std::string& newDesc) { metadata.description = newDesc; }
void ApgPackage::setMaintainer(const std::string& newMain) { metadata.maintainer = newMain; }
void ApgPackage::setLicense(const std::string& newLic) { metadata.license = newLic; }
void ApgPackage::setHomepage(const std::string& newHome) { metadata.homepage = newHome; }

bool ApgPackage::WriteToDb(const LmdbDb& db) const
{
    const std::string key = metadata.name;
    const std::string value = toJson().dump();
    return db.put(key, value, true);
}

std::vector<ApgPackage> ApgPackage::LoadAllFromDb(const LmdbDb& db)
{
    std::vector<ApgPackage> result;
    for (const auto& [key, value] : db.entries())
    {
        try
        {
            json j = json::parse(value);
            ApgPackage pkg;
            pkg.fromJson(j);
            result.push_back(std::move(pkg));
        }
        catch (...) { /* ignore */ }
    }
    return result;
}

std::optional<ApgPackage> ApgPackage::LoadFromDb(const LmdbDb& db, const std::string& name)
{
    auto value = db.get(name);
    if (!value.has_value()) return std::nullopt;

    try {
        const json j = json::parse(value.value());
        ApgPackage pkg;
        pkg.fromJson(j);
        return pkg;
    } catch (...)
    {
        return std::nullopt;
    }
}

bool ApgPackage::RemoveFromDb(const LmdbDb& db) const
{
    return db.del(metadata.name);
}

bool ApgPackage::Install() const
{

    return true;
}

bool ApgPackage::Remove() const
{

}


