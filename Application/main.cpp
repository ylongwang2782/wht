#include "main.h"

LED led0(RCU_GPIOC, GPIOC, GPIO_PIN_6);
void ledTask() { led0.toggle(); }

extern SerialConfig usart1_config;
Serial com1(usart1_config);

std::array<uint8_t, 4> UIDReader::UID;

int main(void) {
    systick_config();

    FrameParser parser;

    Timer timer(50, ledTask);

    UIDReader uidReader(0x1FFF7A10);
    printf("UID: %X%X%X%X\n", UIDReader::UID[0], UIDReader::UID[1],
           UIDReader::UID[2], UIDReader::UID[3]);

    while (1) {
        if (usart1_config.rx_count > 0) {
            parser.JsonParse((char *)com1.rxbuffer);
            memset(com1.rxbuffer, 0, sizeof(com1.rxbuffer));

            usart1_config.rx_count = 0;
        }
    }
}
