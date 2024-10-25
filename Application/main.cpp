
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef __cplusplus
extern "C" {
#endif

#include "cJSON.h"
#include "systick.h"

#ifdef __cplusplus
}
#endif
#include "com.h"
#include "led.h"
#include "timer.h"
#include "wht_protocol.h"

LED led0(RCU_GPIOC, GPIOC, GPIO_PIN_6);

extern SerialConfig usart1_config;
extern SerialConfig usart2_config;
Serial com1(usart1_config);

void ledTask() { led0.toggle(); }

static void process_serial_rx_frame(ChronoLink_frame_t *frame);
static void serial_rx_data_process(uint8_t *data, uint16_t len);

int main(void) {
    systick_config();

    uint8_t data_to_send[] = {0x01, 0x02, 0x03};

    com1.data_send(data_to_send, 1);
    com1.dma_tx(data_to_send, 2);

    Serial com2(usart2_config);
    com2.data_send(data_to_send, 1);
    com2.dma_tx(data_to_send, 2);

    Timer timer;
    timer.init(50, ledTask);

    while (1) {
        if (usart1_config.rx_count > 0) {
            serial_rx_data_process(com1.rxbuffer, usart1_config.rx_count);
            usart1_config.rx_count = 0;
        }
    }
}

void serial_rx_data_process(uint8_t *data, uint16_t len) {
    // 存储接收到的原始数据
    uint8_t serial_recv_buff[1024];
    // 当前缓存中的数据长度
    size_t serial_recv_buff_len = 0;
    /* 一包多帧解析 */
    if (serial_recv_buff_len + len <= 1024) {
        memcpy(serial_recv_buff + serial_recv_buff_len, data, len);
        serial_recv_buff_len += len;
    } else {
        // 缓冲区满，丢弃或处理溢出
        serial_recv_buff_len = 0;
    }

    /* 解析接收缓冲区中的所有帧 */
    size_t offset = 0;
    while (offset + WHT_HEADER_LEN <=
           serial_recv_buff_len) {  // 至少包含一个帧头
        ChronoLink_frame_t *frame =
            (ChronoLink_frame_t *)(serial_recv_buff + offset);

        /* 判断帧是否合法 */
        if (frame->frame_delimiter[0] == 0xAB &&
            frame->frame_delimiter[1] == 0xCD) {
            uint8_t frame_len = frame->frame_len;
            if (offset + frame_len <= serial_recv_buff_len) {
                /* 处理当前帧 */
                process_serial_rx_frame(frame);
                /* 移动偏移量，处理下一个帧 */
                offset += frame_len;
            } else {
                // 不完整帧，等待下次接收数据后继续处理
                break;
            }
        } else {
            // 非法帧，跳过
            offset++;
        }
    }

    /* 移动剩余未处理的数据到缓冲区起始处 */
    if (offset < serial_recv_buff_len) {
        memmove(serial_recv_buff, serial_recv_buff + offset,
                serial_recv_buff_len - offset);
    }
    serial_recv_buff_len -= offset;
}

static void process_serial_rx_frame(ChronoLink_frame_t *frame) {
    uint8_t frame_type = frame->frame_type;
    
    if (frame_type == TYPE_JSON) {
        // 处理JSON数据
        printf("receive json data\n");

        cJSON *result = cJSON_CreateObject();
        cJSON_AddStringToObject(result, "config", "success");
        cJSON_AddStringToObject(result, "control", "enable");

        cJSON *root = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "result", result);

        char *reply = cJSON_PrintUnformatted(root);
        printf("%s", reply);

        cJSON_Delete(root);
        free(reply);
    }
}