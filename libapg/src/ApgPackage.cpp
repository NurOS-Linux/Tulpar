// NurOS Ruzen42 2025
#include "Apg/ApgPackage.hpp"

#include <fstream>
#include <sstream>
#include <utility>

#include "Apg/ApgArchiver.hpp"
#include "Apg/ApgLmdbDb.hpp"
#include "Apg/Logger.hpp"
#include "Apg/Md5Hash.hpp"

using nlohmann::json;
namespace fs = std::filesystem;

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
    std::filesystem::path path,
    bool installedByHand
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

ApgPackage::ApgPackage(std::string name, std::string version, std::string architecture, std::string description,
    std::string maintainer, std::string license, std::string homepage, std::vector<std::string> dependencies,
    std::vector<std::string> conflicts, std::vector<std::string> provides, std::vector<std::string> replaces,
    const bool installedByHand, std::filesystem::path path)
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

ApgPackage::ApgPackage(const std::filesystem::path &path, const bool installedByHand)
{
    this->path = path;
    this->installedByHand = installedByHand;
}

void ApgPackage::fromJson(const json &j)
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
        {"version", metadata.version},
        {"architecture", metadata.architecture},
        {"description", metadata.description},
        {"maintainer", metadata.maintainer},
        {"license", metadata.license},
        {"homepage", metadata.homepage},
        {"dependencies", metadata.dependencies},
        {"conflicts", metadata.conflicts},
        {"provides", metadata.provides},
        {"replaces", metadata.replaces},
        {"files", files}
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

void ApgPackage::WriteToDb(const ApgDb &db) const
{
    const std::string key = metadata.name;
    const std::string value = toJson().dump();
    db.Put(key, value, true);
}

std::vector<ApgPackage> ApgPackage::LoadAllFromDb(const ApgDb& db)
{
    std::vector<ApgPackage> result;
    for (const auto& [key, value] : db.Entries())
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

std::optional<ApgPackage> ApgPackage::LoadFromDb(const ApgDb& db, const std::string& name)
{
    auto value = db.Get(name);
    if (!value.has_value()) return std::nullopt;

    try
    {
        const json j = json::parse(value.value());
        ApgPackage pkg;
        pkg.fromJson(j);
        return pkg;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

bool ApgPackage::RemoveFromDb(const ApgDb& db) const
{
    return db.Delete(metadata.name);
}

bool AddFilesFromDirectory(std::vector<fs::path> &files, const fs::path &directory)
{
    try
    {
        if (!fs::exists(directory) || !fs::is_directory(directory))
        {
            return false;
        }

        for ( const auto& entry : fs::recursive_directory_iterator(directory))
        {
            if (entry.is_regular_file())
            {
                fs::path relativePath = fs::relative(entry.path(), directory);
                files.push_back(relativePath);
            }
        }
        return true;

    }
    catch (const fs::filesystem_error&)
    {
        return false;
    }
}

bool CopyPackageFilesToRoot(const fs::path& sourceDir, const fs::path& destRoot = "/")
{
    try
    {
        const fs::path dataDir = sourceDir / "data";

        if (!fs::exists(dataDir) || !fs::is_directory(dataDir))
        {
            Logger::LogWarn("Data directory not found: " + dataDir.string());
            return false;
        }

        unsigned int copiedCount = 0;

        for (const auto& entry : fs::recursive_directory_iterator(dataDir))
        {
            if (entry.is_regular_file())
            {
                const fs::path& sourceFile = entry.path();
                const fs::path relativePath = fs::relative(sourceFile, dataDir);
                const fs::path destFile = destRoot / relativePath;

                if (const fs::path destParent = destFile.parent_path(); !fs::exists(destParent))
                {
                    fs::create_directories(destParent);
                }

                fs::copy_file(sourceFile, destFile, fs::copy_options::overwrite_existing);
                copiedCount++;

                Logger::LogDebug("Copied: " + relativePath.string());
            }
        }

        Logger::LogDebug("Successfully copied " + std::to_string(copiedCount) + " files to " + destRoot.string());
        return true;

    }
    catch (const fs::filesystem_error& e)
    {
        Logger::LogError("Failed to copy package files: " + std::string(e.what()));
        return false;
    }
}

bool CopyScriptsFilesToRoot(const fs::path& sourceDir, const std::string& pkgname, const fs::path& destRoot = "/")
{
    try
    {
        const fs::path scriptsDir = sourceDir / "scripts";

        if (!fs::exists(scriptsDir) || !fs::is_directory(scriptsDir))
        {
            Logger::LogWarn("scripts directory not found: " + scriptsDir.string());
            return false;
        }

        const fs::path targetBaseDir = destRoot / "var" / "lib" / "tulpar" / pkgname / "scripts";

        if (!fs::exists(targetBaseDir))
        {
            fs::create_directories(targetBaseDir);
        }

        unsigned int copiedCount = 0;
        const std::vector<std::string> allowedFiles = {"preremove", "postremove"};

        for (const auto& entry : fs::directory_iterator(scriptsDir))
        {
            if (entry.is_regular_file())
            {
                const fs::path& sourceFile = entry.path();

                if (const std::string filename = sourceFile.filename().string(); std::find(allowedFiles.begin(), allowedFiles.end(), filename) != allowedFiles.end())
                {
                    const fs::path destFile = targetBaseDir / filename;

                    fs::copy_file(sourceFile, destFile, fs::copy_options::overwrite_existing);
                    copiedCount++;

                    Logger::LogDebug("Copied: " + filename + " to " + destFile.string());
                }
                else
                {
                    Logger::LogDebug("Skipped: " + filename + " (not in allowed list)");
                }
            }
        }

        Logger::LogDebug("Successfully copied " + std::to_string(copiedCount) +
                       " script files to " + targetBaseDir.string());
        return true;
    }
    catch (const fs::filesystem_error& e)
    {
        Logger::LogError("Failed to copy package files: " + std::string(e.what()));
        return false;
    }
}

bool ApgPackage::Install(ApgDb db, bool checkSums = true, const std::string& root = "/")
{
    try
    {
        Logger::LogDebug("Installing: " + path.string());
        const auto pathToPkg = "/tmp/apg/" + path.filename().string();
        Logger::LogDebug("Extracting apg: " + path.string());
        ApgArchiver::Extract(path, pathToPkg);
        std::ifstream file(pathToPkg + "/metadata.json");
        auto jsonData = json::parse(file);
        fromJson(jsonData);
        if (checkSums)
        {
            if (!Md5Hash::VerifyPackageIntegrity(pathToPkg + "/md5sums", pathToPkg + "/data"))
            {
                Logger::LogError("Files are corrupted");
                return false;
            }
        }
        AddFilesFromDirectory(files, pathToPkg + "/data");
        CopyPackageFilesToRoot(pathToPkg, root);
        WriteToDb(db);
        return true;
    }
    catch (...)
    {
        return false;
    }
}



bool ApgPackage::Remove(const ApgDb &db, const fs::path &root) const
{
    for (const auto& file : files)
    {
        try
        {
            if (fs::exists(file) && fs::is_regular_file(file))
            {
                fs::path removePath = root / file;
                fs::remove(removePath);
                Logger::LogDebug("Removed file: " + removePath.string());
            }
        }
        catch (const fs::filesystem_error& e)
        {
            Logger::LogWarn("Could not remove " + file.string() + ": " + e.what());
        }
    }
    return RemoveFromDb(db);
}




