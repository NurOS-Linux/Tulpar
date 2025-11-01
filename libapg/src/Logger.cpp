// NurOS Ruzen42 2025

#include <iostream>

#include "Apg/Logger.hpp"

// Убрать повторное объявление класса - определяем только методы

void Logger::Log(const LogLevel level, const std::string &log)
{
	switch (level)
	{
		case Info:
			std::cout << R"(e[1;32mINFO:e[0m )" << log << "\n";
			break;
		case Debug:
			std::cout << R"(e[1;35mDEBUG:e[0m )" << log << "\n";
			break;
		case Warn:
			std::cout << R"(e[1;33mWARN:e[0m )" << log << "\n";
			break;
		case Error:
			std::cout << R"(e[1;31mERROR:e[0m )" << log << "\n";
			break;
		case Fatal:
			std::cout << R"(e[1;31mFATAL:e[0m )" << log << "\n";
			break;
	}
}

void Logger::LogError(const std::string &log)
{
	Log(Error, log);
}

void Logger::LogWarn(const std::string &log)
{
	Log(Warn, log);
}

void Logger::LogFatal(const std::string &log)
{
	Log(Fatal, log);
}

void Logger::LogDebug(const std::string &log)
{
	Log(Debug, log);
}

void Logger::LogInfo(const std::string &log)
{
	Log(Info, log);
}