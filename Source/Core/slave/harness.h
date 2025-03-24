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
            pins.emplace_back(GPIO::Port::E,
                              static_cast<GPIO::Pin>(GPIO_PIN_0 << i),
                              GPIO::Mode::OUTPUT);
            pins[i].bit_reset();
            pins[i].mode_set(GPIO::Mode::INPUT);
        }
    }

    void reload() { rowIndex = 0; }

    void deinit() { pins.clear(); }

   private:
    uint8_t conductionNum;
    uint16_t totalConductionNum;
    uint16_t startConductionNum;    // 起始导通数量
};