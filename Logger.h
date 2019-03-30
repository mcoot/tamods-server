#pragma once

#include "buildconfig.h"

#include <cstdio>
#include <cstdarg>
#include <Windows.h>
#include <string>

namespace Logger
{
	enum Level {
		LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL, LOG_NONE
	};

	void printLog(Level level, const char *str);
	void log(Level level, const char *format, ...);
	void noln(Level level, const char *format, ...);
	void putc(Level level, char c);
	void flush(Level level);

	void setLevel(Level level);
	Level getLevel();
	bool isEnabled();
	
	void debug(const char *format, ...);
	void printDebug(const char *str);
	void info(const char *format, ...);
	void printInfo(const char *str);
	void warn(const char *format, ...);
	void printWarn(const char *str);
	void error(const char *format, ...);
	void printError(const char *str);
	void fatal(const char *format, ...);
	void printFatal(const char *str);

	void cleanup();
};