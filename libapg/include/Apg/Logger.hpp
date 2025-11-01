// NurOS Ruzen42 2025

#ifndef LIBAPG_LOGGER_HPP
#define LIBAPG_LOGGER_HPP
#include <string>


enum LogLevel { Info, Debug, Warn, Error, Fatal };

class Logger
{
public:
	static void LogError(const std::string &log);
	static void LogWarn(const std::string &log);
	static void LogInfo(const std::string &log);
	static void LogDebug(const std::string &log);
	static void LogFatal(const std::string &log);
private:
	static void Log(LogLevel level, const std::string &log);
};
#endif //LIBAPG_LOGGER_HPP