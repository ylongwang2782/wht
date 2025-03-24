/*
example：
class ClipTask : public TaskClassS<256> {
   public:
    ClipTask()
        : TaskClassS<256>("clipTask", TaskPrio_Low){}

    void task() override {
        ClipInterface clipInterface(uart6); //初始化需要带目标串口
        for (;;) {
            uint16_t clipStatData = clipInterface.clipIORead(); // 读IO
            clipInterface.clipModeLock(); // 自锁
            clipInterface.clipModeUnlock(); // 非自锁
            clipInterface.clipModeClear(); // 清除锁定（清除LED，恢复为自锁）
            clipInterface.clipLedWrite(0x1234); // 写LED
            TaskBase::delay(100);
        }
    }
};

ClipTask clipTask;

***在DMAtask中还需添加
if (xSemaphoreTake(uart6_info.dmaRxDoneSema, portMAX_DELAY) ==
                pdPASS) {
                vTaskDelay(10);
            }
*/

#include <array>
#include <cstdint>
#include <cstdio>

#include "TaskCPP.h"
#include "bsp_gpio.hpp"
#include "bsp_log.hpp"
#include "gd32f4xx.h"
#include "gd32f4xx_dma.h"
#include "gd32f4xx_usart.h"

extern Logger Log;
class ClipInterface {
   public:
    ClipInterface(Uart &targetUart)
        : clip485ctl(GPIO::Port::F, GPIO::Pin::PIN_4, GPIO::Mode::OUTPUT),
          uart(targetUart) {}
    uint16_t clipStatRead(uint8_t tarNum) {
        clipStatReadCmd(tarNum);
        clip485ctl.bit_set();
        sendWithDMA();

        constexpr uint32_t DMA_TIMEOUT = 100;
        uint32_t dmaStart = xTaskGetTickCount();
        while (!clipSendStatGet()) {
            if (xTaskGetTickCount() - dmaStart > pdMS_TO_TICKS(DMA_TIMEOUT)) {
                break;
            }
            vTaskDelay(1);
        }
        clip485ctl.bit_reset();

        uint32_t startTime = xTaskGetTickCount();
        uint16_t responseLen = 0;
        uint8_t *response = nullptr;

        constexpr uint16_t MIN_RESPONSE_LEN = 16;    // 最小响应长度
        constexpr uint32_t MAX_WAIT_MS = 50;         // 超时时间50ms
        constexpr uint8_t DATA_START_POS = 6;    // 数据开始位置，取两位拼成 u16

        do {
            response = clipGetResponse(responseLen);
            if (responseLen > MIN_RESPONSE_LEN) break;
            TaskBase::delay(5);    // 5ms轮询,至多50ms
        } while (xTaskGetTickCount() - startTime < pdMS_TO_TICKS(MAX_WAIT_MS));

        if (!response || responseLen < MIN_RESPONSE_LEN) {
            Log.e("No response (len:%d)", responseLen);
            return 0;
        }

        // 校验帧头与帧尾
        if (response[0] != 0xDD || response[1] != 0x98 || response[2] != 0x89) {
            Log.e("Invalid frame header");
            return 0;
        }
        if (response[responseLen - 2] != 0xA8 ||
            response[responseLen - 1] != 0x86) {
            Log.e("Invalid frame footer");
            return 0;
        }

        // ID
        if (response[3] != tarNum) {
            Log.e("Device ID mismatch (req:%d vs resp:%d)", tarNum,
                  response[3]);
            std::fill_n(recvBuffer.begin(), recvBuffer.size(), 0);
            return 0;
        }

        // CRC
        uint16_t calculatedCRC = Modbus_CRC16(response + 3, responseLen - 7);
        uint16_t receivedCRC =
            (response[responseLen - 4] << 8) | response[responseLen - 3];

        if (calculatedCRC != receivedCRC) {
            Log.e("CRC error (calc:0x%04X vs recv:0x%04X)", calculatedCRC,
                  receivedCRC);
            return 0;
        }

        // 解析数据
        uint16_t result =
            (response[DATA_START_POS] << 8) | response[DATA_START_POS + 1];
        Log.d("lastClipData : 0x%04X", result);

        responseLen = 0;
        return result;
    }

    bool clipStatWrite(uint8_t tarNum, uint16_t address, uint16_t data) {
        clipStatWriteCmd(tarNum, address, data);
        clip485ctl.bit_set();
        sendWithDMA();

        constexpr uint32_t DMA_TIMEOUT = 100;
        uint32_t dmaStart = xTaskGetTickCount();
        while (!clipSendStatGet()) {
            if (xTaskGetTickCount() - dmaStart > pdMS_TO_TICKS(DMA_TIMEOUT)) {
                break;
            }
            vTaskDelay(1);
        }
        clip485ctl.bit_reset();

        uint32_t startTime = xTaskGetTickCount();
        uint16_t responseLen = 0;
        uint8_t *response = nullptr;

        constexpr uint16_t MIN_RESPONSE_LEN = 13;
        constexpr uint32_t MAX_WAIT_MS = 50;

        do {
            response = clipGetResponse(responseLen);
            if (responseLen >= MIN_RESPONSE_LEN) break;
            TaskBase::delay(5);
        } while (xTaskGetTickCount() - startTime < pdMS_TO_TICKS(MAX_WAIT_MS));

        // 校验响应
        if (!response || responseLen < MIN_RESPONSE_LEN) {
            Log.e("Write no response(len:%d<8)", responseLen);
            return false;
        }

        // 校验帧头与帧尾
        if (response[0] != 0xDD || response[1] != 0x98 || response[2] != 0x89) {
            Log.e("Write invalid frame header");
            return false;
        }
        if (response[responseLen - 2] != 0xA8 ||
            response[responseLen - 1] != 0x86) {
            Log.e("Invalid frame footer");
            return 0;
        }

        // ID
        if (response[3] != tarNum || response[4] != 0x06) {
            Log.e("Write response error");
            return false;
        }

        uint16_t respAddress = (response[5] << 8) | response[6];
        uint16_t respData = (response[7] << 8) | response[8];

        // 校验地址和数据
        if ((respAddress != address) || (respData != data)) {
            Log.e("Data mismatch (sent:%04X/%04X vs resp:%04X/%04X)", address,
                  data, respAddress, respData);
            return false;
        }

        // CRC
        uint16_t calcCRC = Modbus_CRC16(response + 3, responseLen - 7);
        uint16_t recvCRC = (response[9] << 8) | response[10];

        if (calcCRC == recvCRC) {
            Log.d("Write 0x%04X to address 0x%04X success", data, address);
        } else {
            Log.e("CRC mismatch (calc:%04X vs recv:%04X)", calcCRC, recvCRC);
        }
        return calcCRC == recvCRC;
    }    // 写结束

    uint16_t clipIORead() {
        clipStatReadCmd(0x01);
        clip485ctl.bit_set();
        sendWithDMA();

        constexpr uint32_t DMA_TIMEOUT = 100;
        uint32_t dmaStart = xTaskGetTickCount();
        while (!clipSendStatGet()) {
            if (xTaskGetTickCount() - dmaStart > pdMS_TO_TICKS(DMA_TIMEOUT)) {
                break;
            }
            vTaskDelay(1);
        }
        clip485ctl.bit_reset();

        uint32_t startTime = xTaskGetTickCount();
        uint16_t responseLen = 0;
        uint8_t *response = nullptr;

        constexpr uint16_t MIN_RESPONSE_LEN = 16;    // 最小响应长度
        constexpr uint32_t MAX_WAIT_MS = 50;         // 超时时间50ms
        constexpr uint8_t DATA_START_POS = 6;    // 数据开始位置，取两位拼成 u16

        do {
            response = clipGetResponse(responseLen);
            if (responseLen > MIN_RESPONSE_LEN) break;
            TaskBase::delay(5);    // 5ms轮询,至多50ms
        } while (xTaskGetTickCount() - startTime < pdMS_TO_TICKS(MAX_WAIT_MS));

        if (!response || responseLen < MIN_RESPONSE_LEN) {
            Log.e("No response (len:%d)", responseLen);
            return 0;
        }

        // 校验帧头与帧尾
        if (response[0] != 0xDD || response[1] != 0x98 || response[2] != 0x89) {
            Log.e("Invalid frame header");
            return 0;
        }
        if (response[responseLen - 2] != 0xA8 ||
            response[responseLen - 1] != 0x86) {
            Log.e("Invalid frame footer");
            return 0;
        }

        // ID
        if (response[3] != 0x01) {
            Log.e("Device ID mismatch (req:%d vs resp:%d)", 0x01, response[3]);
            std::fill_n(recvBuffer.begin(), recvBuffer.size(), 0);
            return 0;
        }

        // CRC
        uint16_t calculatedCRC = Modbus_CRC16(response + 3, responseLen - 7);
        uint16_t receivedCRC =
            (response[responseLen - 4] << 8) | response[responseLen - 3];

        if (calculatedCRC != receivedCRC) {
            Log.e("CRC error (calc:0x%04X vs recv:0x%04X)", calculatedCRC,
                  receivedCRC);
            return 0;
        }

        // 解析数据
        uint16_t result =
            (response[DATA_START_POS] << 8) | response[DATA_START_POS + 1];
        Log.d("lastClipData : 0x%04X", result);

        responseLen = 0;
        return result;
    }

    bool clipLedWrite(uint16_t data) {
        clipStatWriteCmd(0x01, 0x01, data);
        clip485ctl.bit_set();
        sendWithDMA();

        constexpr uint32_t DMA_TIMEOUT = 100;
        uint32_t dmaStart = xTaskGetTickCount();
        while (!clipSendStatGet()) {
            if (xTaskGetTickCount() - dmaStart > pdMS_TO_TICKS(DMA_TIMEOUT)) {
                break;
            }
            vTaskDelay(1);
        }
        clip485ctl.bit_reset();

        uint32_t startTime = xTaskGetTickCount();
        uint16_t responseLen = 0;
        uint8_t *response = nullptr;

        constexpr uint16_t MIN_RESPONSE_LEN = 13;
        constexpr uint32_t MAX_WAIT_MS = 50;

        do {
            response = clipGetResponse(responseLen);
            if (responseLen >= MIN_RESPONSE_LEN) break;
            TaskBase::delay(5);
        } while (xTaskGetTickCount() - startTime < pdMS_TO_TICKS(MAX_WAIT_MS));

        // 校验响应
        if (!response || responseLen < MIN_RESPONSE_LEN) {
            Log.e("Write no response(len:%d<8)", responseLen);
            return false;
        }

        // 校验帧头与帧尾
        if (response[0] != 0xDD || response[1] != 0x98 || response[2] != 0x89) {
            Log.e("Write invalid frame header");
            return false;
        }
        if (response[responseLen - 2] != 0xA8 ||
            response[responseLen - 1] != 0x86) {
            Log.e("Invalid frame footer");
            return false;
        }

        // ID
        if (response[3] != 0x01 || response[4] != 0x06) {
            Log.e("Write response error");
            return false;
        }

        uint16_t respAddress = (response[5] << 8) | response[6];
        uint16_t respData = (response[7] << 8) | response[8];

        // 校验地址和数据
        if ((respAddress != 0x01) || (respData != data)) {
            Log.e("Data mismatch (sent:%04X/%04X vs resp:%04X/%04X)", 0x01,
                  data, respAddress, respData);
            return false;
        }

        // CRC
        uint16_t calcCRC = Modbus_CRC16(response + 3, responseLen - 7);
        uint16_t recvCRC = (response[9] << 8) | response[10];

        if (calcCRC == recvCRC) {
            Log.d("Write 0x%04X to address 0x%04X success", data, 0x01);
        } else {
            Log.e("CRC mismatch (calc:%04X vs recv:%04X)", calcCRC, recvCRC);
        }
        return calcCRC == recvCRC;
    }    // 写结束

    bool clipModeWrite(uint16_t mode) {
        clipStatWriteCmd(0x01, 0x02, mode);
        clip485ctl.bit_set();
        sendWithDMA();

        constexpr uint32_t DMA_TIMEOUT = 100;
        uint32_t dmaStart = xTaskGetTickCount();
        while (!clipSendStatGet()) {
            if (xTaskGetTickCount() - dmaStart > pdMS_TO_TICKS(DMA_TIMEOUT)) {
                break;
            }
            vTaskDelay(1);
        }
        clip485ctl.bit_reset();

        uint32_t startTime = xTaskGetTickCount();
        uint16_t responseLen = 0;
        uint8_t *response = nullptr;

        constexpr uint16_t MIN_RESPONSE_LEN = 13;
        constexpr uint32_t MAX_WAIT_MS = 50;

        do {
            response = clipGetResponse(responseLen);
            if (responseLen >= MIN_RESPONSE_LEN) break;
            TaskBase::delay(5);
        } while (xTaskGetTickCount() - startTime < pdMS_TO_TICKS(MAX_WAIT_MS));

        // 校验响应
        if (!response || responseLen < MIN_RESPONSE_LEN) {
            Log.e("Write no response(len:%d<8)", responseLen);
            return false;
        }

        // 校验帧头与帧尾
        if (response[0] != 0xDD || response[1] != 0x98 || response[2] != 0x89) {
            Log.e("Write invalid frame header");
            return false;
        }
        if (response[responseLen - 2] != 0xA8 ||
            response[responseLen - 1] != 0x86) {
            Log.e("Invalid frame footer");
            return false;
        }

        // ID
        if (response[3] != 0x01 || response[4] != 0x06) {
            Log.e("Write response error");
            return false;
        }

        uint16_t respAddress = (response[5] << 8) | response[6];
        uint16_t respData = (response[7] << 8) | response[8];

        // 校验地址和数据
        if ((respAddress != 0x02) || (respData != mode)) {
            Log.e("Data mismatch (sent:%04X/%04X vs resp:%04X/%04X)", 0x02,
                  mode, respAddress, respData);
            return false;
        }

        // CRC
        uint16_t calcCRC = Modbus_CRC16(response + 3, responseLen - 7);
        uint16_t recvCRC = (response[9] << 8) | response[10];

        if (calcCRC == recvCRC) {
            Log.d("Write 0x%04X to address 0x%04X success", mode, 0x02);
        } else {
            Log.e("CRC mismatch (calc:%04X vs recv:%04X)", calcCRC, recvCRC);
        }
        return calcCRC == recvCRC;
    }

    bool clipModeLock() { return clipModeWrite(0); }

    bool clipModeUnlock() { return clipModeWrite(1); }

    bool clipModeClear() { return clipModeWrite(2); }

   private:
    GPIO clip485ctl;
    Uart &uart;
    uint16_t sendLength = 0;
    std::array<uint8_t, 20> sendBuffer;
    std::array<uint8_t, 20> recvBuffer{};

    // 直接调用 DMA 发送
    void sendWithDMA() {
        if (sendLength > 0) {
            uart.send(sendBuffer.data(), sendLength);
        }
    }

    bool clipSendStatGet() {
        FlagStatus dmaFlag = dma_flag_get(DMA0, DMA_CH1, DMA_FLAG_FTF);
        FlagStatus usartFlag = usart_flag_get(UART6, USART_FLAG_TC);

        if (dmaFlag == SET && usartFlag == SET) {
            dma_flag_clear(DMA0, DMA_CH1, DMA_FLAG_FTF);
            usart_flag_clear(UART6, USART_FLAG_TC);
            return true;
        }
        return false;
    }

    uint8_t *clipGetResponse(uint16_t &responseLen) {
        responseLen = 0;
        uint16_t availableData =
            uart.getReceivedData(recvBuffer.data(), recvBuffer.size());
        if (availableData > 0) {
            responseLen = availableData;
            return recvBuffer.data();
        }
        return nullptr;
    }

    void clipStatReadCmd(uint8_t tarNum) {
        std::fill(sendBuffer.begin(), sendBuffer.end(), 0);    // 清空缓冲区
        uint8_t i = 0;
        sendBuffer[i++] = 0xDD;
        sendBuffer[i++] = 0x98;
        sendBuffer[i++] = 0x89;

        sendBuffer[i++] = tarNum;
        sendBuffer[i++] = 0x03;
        sendBuffer[i++] = 0x00;
        sendBuffer[i++] = 0x00;
        sendBuffer[i++] = 0x00;
        sendBuffer[i++] = 0x03;

        uint16_t crc = Modbus_CRC16(sendBuffer.data() + 3, 6);
        sendBuffer[i++] = (uint8_t)(crc >> 8);
        sendBuffer[i++] = (uint8_t)(crc & 0xFF);

        sendBuffer[i++] = 0xA8;
        sendBuffer[i++] = 0x86;
        sendLength = i;
    };

    void clipStatWriteCmd(uint8_t tarNum, uint16_t address, uint16_t data) {
        uint8_t i = 0;
        sendBuffer[i++] = 0xDD;
        sendBuffer[i++] = 0x98;
        sendBuffer[i++] = 0x89;

        sendBuffer[i++] = tarNum;
        sendBuffer[i++] = 0x06;
        sendBuffer[i++] = (address >> 8) & 0xFF;
        sendBuffer[i++] = address & 0xFF;
        sendBuffer[i++] = (data >> 8) & 0xFF;
        sendBuffer[i++] = data & 0xFF;

        uint16_t crc = Modbus_CRC16(sendBuffer.data() + 3, 6);
        sendBuffer[i++] = (uint8_t)(crc >> 8);
        sendBuffer[i++] = (uint8_t)(crc & 0xFF);

        sendBuffer[i++] = 0xA8;
        sendBuffer[i++] = 0x86;
        sendLength = i;
    }

    uint16_t Modbus_CRC16(uint8_t *puchMsg, uint16_t usDataLen) {
        uint8_t uchCRCHi = 0xFF;
        uint8_t uchCRCLo = 0xFF;
        unsigned long uIndex;

        while (usDataLen--) {
            uIndex = uchCRCHi ^ *puchMsg++;
            uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex];
            uchCRCLo = auchCRCLo[uIndex];
        }

        return (uchCRCHi << 8 | uchCRCLo);
    };

    const uint8_t auchCRCHi[256] = {
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40};
    /* CRC低位字节值表*/
    const uint8_t auchCRCLo[256] = {
        0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
        0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
        0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9,
        0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
        0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
        0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
        0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D,
        0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
        0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF,
        0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
        0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
        0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
        0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB,
        0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
        0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
        0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
        0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97,
        0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
        0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89,
        0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
        0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
        0x41, 0x81, 0x80, 0x40};
};
