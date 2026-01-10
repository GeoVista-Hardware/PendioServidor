// Simple Logger utility for consistent, timestamped logs
#ifndef LOGGER_H
#define LOGGER_H
#include <Arduino.h>

enum LogLevel {
  LOG_LEVEL_DEBUG = 0,
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARN,
  LOG_LEVEL_ERROR,
};

class Logger {
public:
  static void begin(unsigned long baud = 115200);
  static void setLevel(LogLevel lvl);
  static void log(LogLevel lvl, const char* tag, const char* msg);
  static void logf(LogLevel lvl, const char* tag, const char* fmt, ...);
private:
  static LogLevel _level;
  static void printTimestamp();
};

#define LOGD(tag, ...) Logger::logf(LOG_LEVEL_DEBUG, tag, __VA_ARGS__)
#define LOGI(tag, ...) Logger::logf(LOG_LEVEL_INFO, tag, __VA_ARGS__)
#define LOGW(tag, ...) Logger::logf(LOG_LEVEL_WARN, tag, __VA_ARGS__)
#define LOGE(tag, ...) Logger::logf(LOG_LEVEL_ERROR, tag, __VA_ARGS__)

#endif // LOGGER_H
