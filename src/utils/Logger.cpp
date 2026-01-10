#include "utils/Logger.h"
#include <stdarg.h>
#include <stdio.h>

LogLevel Logger::_level = LOG_LEVEL_INFO;

void Logger::begin(unsigned long baud) {
  Serial.begin(baud);
  while (!Serial) { delay(1); }
}

void Logger::setLevel(LogLevel lvl) {
  _level = lvl;
}

void Logger::printTimestamp() {
  unsigned long ms = millis();
  unsigned long s = ms / 1000;
  unsigned long h = (s / 3600) % 24;
  unsigned long m = (s / 60) % 60;
  unsigned long sec = s % 60;
  unsigned long rem = ms % 1000;
  char buf[16];
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu.%03lu", h, m, sec, rem);
  Serial.print("["); Serial.print(buf); Serial.print("] ");
}

void Logger::log(LogLevel lvl, const char* tag, const char* msg) {
  if (lvl < _level) return;
  printTimestamp();
  const char* lvlstr = "DEBUG";
  switch (lvl) {
    case LOG_LEVEL_DEBUG: lvlstr = "DEBUG"; break;
    case LOG_LEVEL_INFO: lvlstr = "INFO"; break;
    case LOG_LEVEL_WARN: lvlstr = "WARN"; break;
    case LOG_LEVEL_ERROR: lvlstr = "ERROR"; break;
  }
  Serial.print("["); Serial.print(lvlstr); Serial.print("]");
  Serial.print("["); Serial.print(tag); Serial.print("] ");
  Serial.println(msg);
}

void Logger::logf(LogLevel lvl, const char* tag, const char* fmt, ...) {
  if (lvl < _level) return;
  char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  log(lvl, tag, buffer);
}
