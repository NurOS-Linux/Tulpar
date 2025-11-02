// NurOS Ruzen42 2025

#include <iostream>

#include "Apg/Logger.hpp"

void Logger::Log(const LogLevel level, const std::string &log)
{
	switch (level)
	{
		case Info:
			std::cout << "\033[1;32mINFO:\033[0m " << log << "\n";
			break;
		case Debug:
			std::cout << "\033[1;35mDEBUG:\033[0m " << log << "\n";
			break;
		case Warn:
			std::cout << "\033[1;33mWARN:\033[0m " << log << "\n";
			break;
		case Error:
			std::cout << "\033[1;31mERROR:\033[0m " << log << "\n";
			break;
		case Fatal:
			std::cout << "\033[1;31mFATAL:\033[0m " << log << "\n";
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