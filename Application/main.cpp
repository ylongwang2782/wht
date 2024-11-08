#include "main.h"

// LED led0(RCU_GPIOC, GPIOC, GPIO_PIN_6);

extern SerialConfig usart1_config;
extern Serial com1;

std::array<uint8_t, 4> UIDReader::UID;

int main(void) {
    systick_config();

    FrameParser parser;

    UIDReader uidReader(0x1FFF7A10);
    printf("UID: %X%X%X%X\n", UIDReader::UID[0], UIDReader::UID[1],
           UIDReader::UID[2], UIDReader::UID[3]);

    while (1) {
        if (usart1_config.rx_count > 0) {
            parser.JsonParse((char *)com1.rxbuffer);
            usart1_config.rx_count = 0;
        }
    }
}
