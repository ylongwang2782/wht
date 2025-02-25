#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "bsp_gpio.hpp"
#include "bsp_log.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include "gd32f4xx.h"

#ifdef __cplusplus
}
#endif

struct DeviceConfigInfo {
    std::array<uint8_t, 4> ID;
    uint16_t sysHarnessNum;
    uint8_t devHarnessNum;
    uint8_t devNum;
};

class BinaryMatrix {
   private:
    std::vector<std::vector<int>> matrix;

   public:
    size_t rows, cols;
    // 构造函数，初始化矩阵
    BinaryMatrix(size_t r, size_t c)
        : matrix(r, std::vector<int>(c, 0)), rows(r), cols(c) {}

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

    std::vector<uint8_t> flatten() const {
        std::vector<uint8_t> result;
        result.reserve(rows * cols);    // 预分配空间提高效率
        for (const auto& row : matrix) {
            result.insert(result.end(), row.begin(), row.end());
        }
        return result;
    }
};

class Harness {
   public:
    BinaryMatrix data;
    std::vector<GPIO> pins;
    int rowIndex = 0;

    Harness(uint8_t devHarnessNum, uint16_t sysHarnessNum)
        : data(sysHarnessNum, devHarnessNum),
          devHarnessNum(devHarnessNum),
          sysHarnessNum(sysHarnessNum) {}

    void run() {
        for (size_t i = 0; i < data.cols; i++) {
            data.setValue(rowIndex, i, pins[i].input_bit_get());
        }
    }

    void init() {
        for (int i = 0; i < devHarnessNum; ++i) {
            pins.emplace_back(GPIO::Port::E,
                              static_cast<GPIO::Pin>(GPIO_PIN_0 << i),
                              GPIO::Mode::OUTPUT);
        }
    }

    void reload() { rowIndex = 0; }

    void deinit() { pins.clear(); }

   private:
    uint8_t devNum;
    uint8_t devHarnessNum;
    uint16_t sysHarnessNum;
    uint8_t master_pin_index = 0;
    uint8_t packed_data = 0;
    int bit_position = 0;
};