// NurOS Ruzen42 2025

#ifndef LIBAPG_MD5HASH_HPP
#define LIBAPG_MD5HASH_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;

class Md5Hash
{
public:
	static bool VerifyPackageIntegrity(const fs::path& md5sumsFile, const fs::path& dataDirectory);
	static std::unordered_map<std::string, std::string> LoadMd5Sums(const fs::path& md5sumsFile);
	static std::optional<std::string> CalculateFileMd5(const fs::path& filepath);

private:
	static bool VerifyFileIntegrity(const fs::path& filepath, const std::string& expectedHash);
	static std::pair<std::string, std::string> ParseMd5Line(const std::string& line);
};

#endif // LIBAPG_MD5HASH_HPP