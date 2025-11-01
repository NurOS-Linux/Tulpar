// NurOS Ruzen42 2025
#ifndef LIBAPG_APGARCHIVER_HPP
#define LIBAPG_APGARCHIVER_HPP

#pragma once
#include <filesystem>
#include <string>

class ApgArchiver
{
public:
	ApgArchiver() = default;
	~ApgArchiver() = default;

	static bool Extract(const std::string &path, const std::string &destDir);
};
#endif //LIBAPG_APGARCHIVER_HPP