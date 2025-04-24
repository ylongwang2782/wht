#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "TaskCPP.h"
#include "bsp_gpio.hpp"
#include "bsp_log.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include "gd32f4xx.h"

#ifdef __cplusplus
}
#endif

class BinaryMatrix {
   private:
    std::vector<std::vector<int>> matrix;

   public:
    size_t rows, cols;

    // 获取矩阵的行数
    uint16_t getRows() const { return rows; }

    // 获取矩阵的列数
    uint16_t getCols() const { return cols; }

    uint16_t getSize() const { return rows * cols; }

    // 设置矩阵中的某个元素（仅能为0或1）
    void setValue(size_t r, size_t c, int value) {
        if (r >= rows || c >= cols) {
        }
        if (value != 0 && value != 1) {
        }
        matrix[r][c] = value;
    }

    // 获取矩阵中的某个元素
    int getValue(size_t r, size_t c) const {
        if (r >= rows || c >= cols) {
        }
        return matrix[r][c];
    }

    // 调整矩阵大小
    void resize(size_t new_rows, size_t new_cols) {
        matrix.resize(new_rows);
        for (auto& row : matrix) {
            row.resize(new_cols, 0);
        }
        rows = new_rows;
        cols = new_cols;
    }

    std::vector<uint8_t> flatten() const {
        std::vector<uint8_t> result;
        size_t byteCount = (rows * cols + 7) / 8;    // 计算需要的字节数
        result.reserve(byteCount);                   // 预分配空间

        uint8_t currentByte = 0;
        int bitCount = 7;    // 从最高位开始

        for (const auto& row : matrix) {
            for (const auto& bit : row) {
                currentByte |= (bit & 0x01) << bitCount;
                bitCount--;
                if (bitCount < 0) {
                    result.push_back(currentByte);
                    currentByte = 0;
                    bitCount = 7;
                }
            }
        }

        // 处理最后不足8位的部分
        if (bitCount != 7) {
            result.push_back(currentByte);
        }

        return result;
    }
};

// 导通引脚信息
const std::pair<GPIO::Port, GPIO::Pin> condPinInfo[] = {
    // PA0 - PA7
    {GPIO::Port::A, GPIO::Pin::PIN_3},    // PA3
    {GPIO::Port::A, GPIO::Pin::PIN_4},    // PA4
    {GPIO::Port::A, GPIO::Pin::PIN_5},    // PA5
    {GPIO::Port::A, GPIO::Pin::PIN_6},    // PA6
    {GPIO::Port::A, GPIO::Pin::PIN_7},    // PA7
    // PC4 - PC5
    {GPIO::Port::C, GPIO::Pin::PIN_4},    // PC4
    {GPIO::Port::C, GPIO::Pin::PIN_5},    // PC5
    // PB0 1
    {GPIO::Port::B, GPIO::Pin::PIN_0},    // PB0
    {GPIO::Port::B, GPIO::Pin::PIN_1},    // PB1
    // PF11 - PF15
    {GPIO::Port::F, GPIO::Pin::PIN_11},    // PF11
    {GPIO::Port::F, GPIO::Pin::PIN_12},    // PF12
    {GPIO::Port::F, GPIO::Pin::PIN_13},    // PF13
    {GPIO::Port::F, GPIO::Pin::PIN_14},    // PF14
    {GPIO::Port::F, GPIO::Pin::PIN_15},    // PF15
    // PG0 - PG1
    {GPIO::Port::G, GPIO::Pin::PIN_0},    // PG0
    {GPIO::Port::G, GPIO::Pin::PIN_1},    // PG1
    // PE7 - PE15
    {GPIO::Port::E, GPIO::Pin::PIN_7},     // PE7
    {GPIO::Port::E, GPIO::Pin::PIN_8},     // PE8
    {GPIO::Port::E, GPIO::Pin::PIN_9},     // PE9
    {GPIO::Port::E, GPIO::Pin::PIN_10},    // PE10
    {GPIO::Port::E, GPIO::Pin::PIN_11},    // PE11
    {GPIO::Port::E, GPIO::Pin::PIN_12},    // PE12
    {GPIO::Port::E, GPIO::Pin::PIN_13},    // PE13
    {GPIO::Port::E, GPIO::Pin::PIN_14},    // PE14
    {GPIO::Port::E, GPIO::Pin::PIN_15},    // PE15
    // PB10 - PB15
    {GPIO::Port::B, GPIO::Pin::PIN_10},    // PB10
    {GPIO::Port::B, GPIO::Pin::PIN_11},    // PB11
    {GPIO::Port::B, GPIO::Pin::PIN_12},    // PB12
    {GPIO::Port::B, GPIO::Pin::PIN_13},    // PB13
    {GPIO::Port::B, GPIO::Pin::PIN_14},    // PB14
    {GPIO::Port::B, GPIO::Pin::PIN_15},    // PB15
    // PD8 - PD15
    {GPIO::Port::D, GPIO::Pin::PIN_8},     // PD8
    {GPIO::Port::D, GPIO::Pin::PIN_9},     // PD9
    {GPIO::Port::D, GPIO::Pin::PIN_10},    // PD10
    {GPIO::Port::D, GPIO::Pin::PIN_11},    // PD11
    {GPIO::Port::D, GPIO::Pin::PIN_12},    // PD12
    {GPIO::Port::D, GPIO::Pin::PIN_13},    // PD13
    {GPIO::Port::D, GPIO::Pin::PIN_14},    // PD14
    {GPIO::Port::D, GPIO::Pin::PIN_15},    // PD15
    // PG2 - PG8
    {GPIO::Port::G, GPIO::Pin::PIN_2},    // PG2
    {GPIO::Port::G, GPIO::Pin::PIN_3},    // PG3
    {GPIO::Port::G, GPIO::Pin::PIN_4},    // PG4
    {GPIO::Port::G, GPIO::Pin::PIN_5},    // PG5
    {GPIO::Port::G, GPIO::Pin::PIN_6},    // PG6
    {GPIO::Port::G, GPIO::Pin::PIN_7},    // PG7
    {GPIO::Port::G, GPIO::Pin::PIN_8},    // PG8
    // PC6 - PC9
    {GPIO::Port::C, GPIO::Pin::PIN_6},    // PC6
    {GPIO::Port::C, GPIO::Pin::PIN_7},    // PC7
    {GPIO::Port::C, GPIO::Pin::PIN_8},    // PC8
    {GPIO::Port::C, GPIO::Pin::PIN_9},    // PC9
    // PA8 - PA12 PA15
    {GPIO::Port::A, GPIO::Pin::PIN_8},     // PA8
    {GPIO::Port::A, GPIO::Pin::PIN_9},     // PA9
    {GPIO::Port::A, GPIO::Pin::PIN_10},    // PA10
    {GPIO::Port::A, GPIO::Pin::PIN_11},    // PA11
    {GPIO::Port::A, GPIO::Pin::PIN_12},    // PA12
    {GPIO::Port::A, GPIO::Pin::PIN_15},    // PA15
    // PC10 - PC11
    {GPIO::Port::C, GPIO::Pin::PIN_10},    // PC10
    {GPIO::Port::C, GPIO::Pin::PIN_11},    // PC11
    //  PD0 - PD1, PD3 -PD4
    {GPIO::Port::D, GPIO::Pin::PIN_0},    // PD0
    {GPIO::Port::D, GPIO::Pin::PIN_1},    // PD1
    {GPIO::Port::D, GPIO::Pin::PIN_3},    // PD3
    {GPIO::Port::D, GPIO::Pin::PIN_4},    // PD4
};

class Harness {
   public:
    BinaryMatrix data;
    std::vector<GPIO> pins;
    int rowIndex = 0;

    bool isDeviceOutput() {
        return rowIndex >= startConductionNum &&
               rowIndex <= startConductionNum + conductionNum;
    }

    void run() {
        if (isDeviceOutput()) {
            int index = rowIndex - startConductionNum;

            // reset last pin
            if (int lastIndex = index - 1; lastIndex >= 0) {
                pins[lastIndex].bit_reset();
                pins[lastIndex].mode_set(GPIO::Mode::INPUT);
            }

            // set current pin
            if (index < conductionNum) {
                pins[index].mode_set(GPIO::Mode::OUTPUT);
                pins[index].bit_set();
            }
        }

        for (size_t i = 0; i < data.cols; i++) {
            data.setValue(rowIndex, i, pins[i].input_bit_get());
        }
    }

    void init(uint8_t conductionNum, uint16_t totalConductionNum,
              int startConductionNum) {
        this->conductionNum = conductionNum;
        this->totalConductionNum = totalConductionNum;
        this->startConductionNum = startConductionNum;
        data.resize(totalConductionNum, conductionNum);
        for (int i = 0; i < conductionNum; ++i) {
            pins.emplace_back(condPinInfo[i].first, condPinInfo[i].second,
                              GPIO::Mode::INPUT);
        }
    }

    void reload() {
        // 将最后一个被设置为输出的引脚重置为输入
        int lastIndex = rowIndex - startConductionNum - 1;
        pins[lastIndex].mode_set(GPIO::Mode::INPUT);
        // 复位行索引
        rowIndex = 0;
    }

    void deinit() { pins.clear(); }

   private:
    uint8_t conductionNum;
    uint16_t totalConductionNum;
    uint16_t startConductionNum;    // 起始导通数量
};