// NurOS Ruzen42 2025

#include "Apg/Md5Hash.hpp"
#include "Apg/Logger.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <openssl/evp.h>

bool Md5Hash::VerifyPackageIntegrity(const fs::path& md5sumsFile, const fs::path& dataDirectory)
{
    try
    {
        if (!fs::exists(md5sumsFile) || !fs::is_regular_file(md5sumsFile))
        {
            Logger::LogError("MD5 sums file not found: " + md5sumsFile.string());
            return false;
        }

        if (!fs::exists(dataDirectory) || !fs::is_directory(dataDirectory))
        {
            Logger::LogError("Data directory not found: " + dataDirectory.string());
            return false;
        }

        auto md5Map = LoadMd5Sums(md5sumsFile);
        if (md5Map.empty())
        {
            Logger::LogError("No valid MD5 entries found in: " + md5sumsFile.string());
            return false;
        }

        Logger::LogInfo("Verifying " + std::to_string(md5Map.size()) + " files from: " + md5sumsFile.string());

        size_t verifiedCount = 0;
        size_t failedCount = 0;

        for (const auto& [relativePath, expectedHash] : md5Map)
        {
            fs::path fullPath = dataDirectory / relativePath;

            if (!fs::exists(fullPath) || !fs::is_regular_file(fullPath))
            {
                Logger::LogWarn("File not found: " + relativePath);
                failedCount++;
                continue;
            }

            if (VerifyFileIntegrity(fullPath, expectedHash))
            {
                verifiedCount++;
                Logger::LogDebug("Verified: " + relativePath);
            }
            else
            {
                failedCount++;
                Logger::LogError("Integrity check failed for: " + relativePath);
            }
        }

        Logger::LogInfo("Verification complete: " + std::to_string(verifiedCount) +
                       " passed, " + std::to_string(failedCount) + " failed");

        return failedCount == 0;
    }
    catch (const std::exception& e)
    {
        Logger::LogError("MD5 verification failed: " + std::string(e.what()));
        return false;
    }
}

std::unordered_map<std::string, std::string> Md5Hash::LoadMd5Sums(const fs::path& md5sumsFile)
{
    std::unordered_map<std::string, std::string> md5Map;

    try
    {
        std::ifstream file(md5sumsFile);
        if (!file.is_open())
        {
            Logger::LogError("Cannot open MD5 sums file: " + md5sumsFile.string());
            return md5Map;
        }

        std::string line;
        size_t lineNumber = 0;

        while (std::getline(file, line))
        {
            lineNumber++;

            if (line.empty() || line[0] == '#')
            {
                continue;
            }

            if (auto [hash, filepath] = ParseMd5Line(line); !hash.empty() && !filepath.empty())
            {
                md5Map[filepath] = hash;
            }
            else
            {
                Logger::LogWarn("Invalid MD5 line " + std::to_string(lineNumber) + ": " + line);
            }
        }

        Logger::LogInfo("Loaded " + std::to_string(md5Map.size()) + " MD5 entries");
    }
    catch (const std::exception& e)
    {
        Logger::LogError("Failed to load MD5 sums: " + std::string(e.what()));
    }

    return md5Map;
}

std::optional<std::string> Md5Hash::CalculateFileMd5(const fs::path& filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
    {
        Logger::LogError("Cannot open file for MD5 calculation: " + filepath.string());
        return std::nullopt;
    }

    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (!context)
    {
        Logger::LogError("Failed to create EVP context");
        return std::nullopt;
    }

    if (EVP_DigestInit_ex(context, EVP_md5(), nullptr) != 1)
    {
        Logger::LogError("Failed to initialize MD5 digest");
        EVP_MD_CTX_free(context);
        return std::nullopt;
    }

    char buffer[4096];
    bool success = true;

    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
    {
        if (EVP_DigestUpdate(context, buffer, file.gcount()) != 1)
        {
            Logger::LogError("Failed to update MD5 digest");
            success = false;
            break;
        }
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLength = 0;

    if (success && EVP_DigestFinal_ex(context, digest, &digestLength) != 1)
    {
        Logger::LogError("Failed to finalize MD5 digest");
        success = false;
    }

    EVP_MD_CTX_free(context);

    if (!success)
    {
        return std::nullopt;
    }

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digestLength; ++i)
    {
        ss << std::setw(2) << static_cast<int>(digest[i]);
    }

    return ss.str();
}

bool Md5Hash::VerifyFileIntegrity(const fs::path& filepath, const std::string& expectedHash)
{
    auto calculatedHash = CalculateFileMd5(filepath);

    if (!calculatedHash.has_value())
    {
        return false;
    }

    const bool result = calculatedHash.value() == expectedHash;

    if (!result)
    {
        Logger::LogWarn("Hash mismatch for " + filepath.string() +
                       ": expected " + expectedHash +
                       ", got " + calculatedHash.value());
    }

    return result;
}

std::pair<std::string, std::string> Md5Hash::ParseMd5Line(const std::string& line)
{
    std::string hash;
    std::string filepath;

    std::istringstream iss(line);
    iss >> hash;

    if (hash.length() != 32)
    {
        return {"", ""};
    }

    for (char c : hash)
    {
        if (!std::isxdigit(c))
        {
            return {"", ""};
        }
    }

    char separator;
    iss >> separator;

    std::getline(iss, filepath);

    filepath.erase(0, filepath.find_first_not_of(" \t"));

    if (filepath.empty())
    {
        return {"", ""};
    }

    return {hash, filepath};
}