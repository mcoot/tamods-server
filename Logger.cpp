#include "Logger.h"

static FILE* _file = NULL;
static Logger::Level _level = Logger::LOG_DEBUG;
static bool useMultipleLogFiles = false;

static bool openFile() {
	if (_level == Logger::LOG_NONE)
		return false;

	if (!_file)
	{
		const char *profile = getenv("USERPROFILE");
		std::string directory;
		if (profile)
			directory = std::string(profile) + "\\Documents\\";
		else
			directory = std::string("C:\\");

		// Find a non-existent file
		int logFileNum = 0;
		std::string logFilePath = std::string(directory + LOGFILE_NAME);
		if (useMultipleLogFiles) {
			while (true) {
				FILE* f = fopen(logFilePath.c_str(), "r");
				if (!f) break;
				fclose(f);
				logFileNum++;
				logFilePath = std::string(directory + LOGFILE_NAME + std::to_string(logFileNum));
			}
		}

		_file = fopen(logFilePath.c_str(), "w+");
	}
	return !!_file;
}

void Logger::setLevel(Level level) {
	_level = level;
}

Logger::Level Logger::getLevel() {
	return _level;
}

bool Logger::isEnabled() {
	return _level < LOG_NONE;
}

void Logger::cleanup() {
	if (!_file) return;
	fclose(_file);
}

void Logger::printLog(Level level, const char *str) {
	if (level < _level) return;
	if (!openFile()) return;

	fprintf(_file, "%s\n", str);
	fflush(_file);
}

void Logger::log(Level level, const char *format, ...)
{
	char buff[256];
	va_list args;

	// Only log things more important or equal to the current level
	if (level < _level) return;
	if (!openFile()) return;

	va_start(args, format);
	vsprintf(buff, format, args);
	va_end(args);
	fprintf(_file, "%s\n", buff);
	fflush(_file);
}

void Logger::noln(Level level, const char *format, ...)
{
	char buff[256];
	va_list args;

	if (level < _level) return;
	if (!openFile()) return;

	va_start(args, format);
	vsprintf(buff, format, args);
	va_end(args);
	fprintf(_file, buff);
}

static void vlog(Logger::Level level, const char *format, va_list args) {
	char buff[256];

	if (level < _level) return;
	if (!openFile()) return;

	vsprintf(buff, format, args);
	fprintf(_file, "%s\n", buff);
	fflush(_file);
}

void Logger::debug(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vlog(LOG_DEBUG, format, args);
	va_end(args);
}

void Logger::printDebug(const char *str) {
	printLog(LOG_DEBUG, str);
}

void Logger::info(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vlog(LOG_INFO, format, args);
	va_end(args);
}

void Logger::printInfo(const char *str) {
	printLog(LOG_INFO, str);
}

void Logger::warn(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vlog(LOG_WARN, format, args);
	va_end(args);
}

void Logger::printWarn(const char *str) {
	printLog(LOG_WARN, str);
}

void Logger::error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vlog(LOG_ERROR, format, args);
	va_end(args);
}

void Logger::printError(const char *str) {
	printLog(LOG_ERROR, str);
}

void Logger::fatal(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vlog(LOG_FATAL, format, args);
	va_end(args);
}

void Logger::printFatal(const char *str) {
	printLog(LOG_FATAL, str);
}