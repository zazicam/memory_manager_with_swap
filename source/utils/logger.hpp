#pragma once

#include <mutex>
#include <sstream>

#define LOG_BEGIN                                                              \
    logger.Lock();                                                             \
    logger << "\n[logger: " << __FILE__ << " line: " << __LINE__ << "]\n";     \
    logger.Flush();

#define LOG_END logger.Unlock();

class Logger : public std::ostringstream {
    std::ostream &output;
    std::mutex mutex;

  public:
    explicit Logger(std::ostream &str);

    void Flush();
    void Lock();
    void Unlock();
};

extern Logger logger;
