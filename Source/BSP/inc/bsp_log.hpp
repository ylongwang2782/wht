#ifndef _LOG_H_
#define _LOG_H_

#include <stdarg.h>

#include "bsp_uart.hpp"

extern "C" {
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
}

#define USART_LOG      USART1
#define LOG_QUEUE_SIZE 64
class Logger {
   public:
    // 日志级别枚举
    enum class Level { VERBOSE, DEBUGL, INFO, WARN, ERROR, RAW };

    // 获取单例实例
    static Logger &getInstance() {
        static Logger instance;
        return instance;
    }

    // 日志记录方法
    void log(Level level, const char *format, va_list args) {
        // 定义日志级别的字符串表示
        static const char *levelStr[] = {"VERBOSE", "DEBUG", "INFO", "WARN",
                                         "ERROR"};

        // 缓冲区大小
        constexpr size_t bufferSize = LOG_QUEUE_SIZE;

        // 缓存区，用于存储格式化后的日志内容
        char buffer[bufferSize];

        // 格式化日志内容
        vsnprintf(buffer, sizeof(buffer), format, args);

        if (level != Level::RAW) {
            // 添加级别前缀
            char finalMessage[bufferSize + 8];
            snprintf(finalMessage, sizeof(finalMessage), "[%s]:%s\n",
                     levelStr[static_cast<int>(level)], buffer);
            // 输出日志
            output(level, finalMessage);
        } else {
            char finalMessage[bufferSize];
            snprintf(finalMessage, sizeof(finalMessage), "%s\n", buffer);
            // 输出日志
            output(level, finalMessage);
        }
    }

    // 快捷方法：VERBOSE 级别日志
    void v(const char *format, ...) {
        va_list args;
        va_start(args, format);
        log(Level::VERBOSE, format, args);
        va_end(args);
    }

    // 快捷方法：DEBUG 级别日志
    void d(const char *format, ...) {
        va_list args;
        va_start(args, format);
        log(Level::DEBUGL, format, args);
        va_end(args);
    }

    // 快捷方法：INFO 级别日志
    void i(const char *format, ...) {
        va_list args;
        va_start(args, format);
        log(Level::INFO, format, args);
        va_end(args);
    }

    // 快捷方法：WARN 级别日志
    void w(const char *format, ...) {
        va_list args;
        va_start(args, format);
        log(Level::WARN, format, args);
        va_end(args);
    }

    // 快捷方法：ERROR 级别日志
    void e(const char *format, ...) {
        va_list args;
        va_start(args, format);
        log(Level::ERROR, format, args);
        va_end(args);
    }

    void r(const char *format, ...) {
        va_list args;
        va_start(args, format);
        log(Level::RAW, format, args);
        va_end(args);
    }

    // 获取日志队列
    QueueHandle_t getLogQueue() const { return logQueue; }
    // 日志队列
    QueueHandle_t logQueue;

   private:
    // 私有构造函数，确保单例模式
    Logger() { logQueue = xQueueCreate(10, LOG_QUEUE_SIZE); }

    // 禁止拷贝和赋值
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    // 日志输出方法
    void output(Level level, const char *message) {
        // 将日志消息添加到队列中
        xQueueSend(logQueue, message, portMAX_DELAY);
    }
};

extern Logger &Log;

#endif
