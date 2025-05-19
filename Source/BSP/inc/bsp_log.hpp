#pragma once
#ifndef _LOG_H_
#define _LOG_H_

#include <stdarg.h>

#include <array>

#include "QueueCPP.h"
#include "TaskCPP.h"
#include "bsp_uart.hpp"
#include "queue.h"
#include "task.h"

#define LOG_TASK_DEPTH_SIZE 512
#define LOG_TASK_PRIO       TaskPrio_Highest

// 定义日志消息的最大长度
#define LOG_QUEUE_SIZE 256
// 定义日志队列的长度
#define LOG_QUEUE_LENGTH 20

// 日志消息结构体
struct LogMessage {
    std::array<char, LOG_QUEUE_SIZE> message;
};

// 日志级别枚举

class Logger {
   public:
    enum class Level { VERBOSE, DEBUGL, INFO, WARN, ERROR, RAW = VERBOSE };

    Logger(Uart& uart) : uart(uart), logQueue("LogQueue") {}

    Uart& uart;    // 串口对象的引用
    FreeRTOScpp::Queue<LogMessage, LOG_QUEUE_LENGTH> logQueue;

    Level currentLevel = Level::VERBOSE;

    void setLogLevel(Level level) { currentLevel = level; }

    void log(Level level, const char* TAG, const char* format, va_list args) {
        // if (level < currentLevel) return;
        // 定义日志级别的字符串表示
        static const char* levelStr[] = {"V", "D", "I", "W", "E", "R"};

        // 缓冲区大小
        constexpr size_t bufferSize = LOG_QUEUE_SIZE;

        // 缓存区，用于存储格式化后的日志内容
        char buffer[bufferSize];

        // 格式化日志内容
        vsnprintf(buffer, sizeof(buffer), format, args);

        // 获取当前时间戳
        uint32_t tick = xTaskGetTickCount();
        uint32_t seconds = tick / configTICK_RATE_HZ;
        uint32_t milliseconds =
            (tick % configTICK_RATE_HZ) * 1000 / configTICK_RATE_HZ;

        // 添加时间戳和级别前缀
        char finalMessage[bufferSize + 32];
        snprintf(finalMessage, sizeof(finalMessage),
                 "[%03lu.%03lu] [%s] [%-6s] %s\n", seconds, milliseconds,
                 levelStr[static_cast<int>(level)], TAG, buffer);
        // 输出日志
        output(level, finalMessage);
    }
    void v(const char* TAG, const char* format, ...) {
        if (Level::VERBOSE < currentLevel) return;
        va_list args;
        va_start(args, format);
        log(Level::VERBOSE, TAG, format, args);
        va_end(args);
    }
    void d(const char* TAG, const char* format, ...) {
        if (Level::DEBUGL < currentLevel) return;
        va_list args;
        va_start(args, format);
        log(Level::DEBUGL, TAG, format, args);
        va_end(args);
    }

    void i(const char* TAG, const char* format, ...) {
        if (Level::INFO < currentLevel) return;
        va_list args;
        va_start(args, format);
        log(Level::INFO, TAG, format, args);
        va_end(args);
    }

    void w(const char* TAG, const char* format, ...) {
        if (Level::WARN < currentLevel) return;
        va_list args;
        va_start(args, format);
        log(Level::WARN, TAG, format, args);
        va_end(args);
    }

    void e(const char* TAG, const char* format, ...) {
        if (Level::ERROR < currentLevel) return;
        va_list args;
        va_start(args, format);
        log(Level::ERROR, TAG, format, args);
        va_end(args);
    }

    void r(uint8_t* data, size_t size) {
        if (Level::RAW < currentLevel) return;
        // 每个字节需要两个字符+一个空格，最后一个字节不需要空格
        char buffer[size * 3];
        for (size_t i = 0; i < size; i++) {
            // 最后一个字节不加空格
            if (i == size - 1) {
                snprintf(buffer + i * 3, 3, "%02X", data[i]);
            } else {
                snprintf(buffer + i * 3, 4, "%02X ", data[i]);
            }
        }
        buffer[size * 3 - 1] = '\0';    // 添加字符串终止符
        va_list emptyArgs;
        const char* TAG = "";
        log(Level::RAW, TAG, buffer, emptyArgs);
    }

   private:
    void output(Level level, const char* message) {
        LogMessage logMsg;
        std::strncpy(logMsg.message.data(), message, LOG_QUEUE_SIZE - 1);
        logMsg.message[LOG_QUEUE_SIZE - 1] = '\0';

        // 将日志消息放入队列
        if (!logQueue.add(logMsg, portMAX_DELAY)) {
            printf("Failed to add log message to queue!\n");
        }
    }
};

class LogTask : public TaskClassS<LOG_TASK_DEPTH_SIZE> {
   private:
    Logger& Log;

   public:
    LogTask(Logger& Log)
        : TaskClassS<LOG_TASK_DEPTH_SIZE>("LogTask", LOG_TASK_PRIO), Log(Log) {}

    void task() override {
        char buffer[LOG_QUEUE_SIZE + 8];
        for (;;) {
            LogMessage logMsg;
            // 从队列中获取日志消息
            if (Log.logQueue.pop(logMsg, portMAX_DELAY)) {
                Log.uart.data_send(
                    reinterpret_cast<const uint8_t*>(logMsg.message.data()),
                    strlen(logMsg.message.data()));
            }
        }
    }
};

#endif
