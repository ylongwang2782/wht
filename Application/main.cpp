
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef __cplusplus
extern "C" {
#endif

#include "systick.h"

#ifdef __cplusplus
}
#endif
#include "chronolink.h"
#include "com.h"
#include "json_interface.h"
#include "led.h"
#include "timer.h"

LED led0(RCU_GPIOC, GPIOC, GPIO_PIN_6);

extern SerialConfig usart1_config;
Serial com1(usart1_config);

void ledTask() { led0.toggle(); }

class UIDReader {
   public:
    UIDReader(uint32_t address) {
        uidAddress = reinterpret_cast<uint32_t *>(address);
        readUID();
    }

    const std::array<uint8_t, 4> &getUID() const { return uidArray; }

   private:
    uint32_t *uidAddress;
    std::array<uint8_t, 4> uidArray;

    void readUID() {
        uint32_t uid = *uidAddress;
        for (int i = 0; i < 4; ++i) {
            uidArray[i] = static_cast<uint8_t>((uid >> (24 - i * 8)) & 0xFF);
        }
    }
};

int main(void) {
    systick_config();

    FrameParser parser;

    Timer timer(50, ledTask);

    UIDReader uidReader(0x1FFF7A10);
    const auto &uid = uidReader.getUID();
    printf("UID: %X%X%X%X\n", uid[0], uid[1], uid[2], uid[3]);

    while (1) {
        if (usart1_config.rx_count > 0) {
            parser.JsonParse((char *)com1.rxbuffer);
            memset(com1.rxbuffer, 0, sizeof(com1.rxbuffer));

            usart1_config.rx_count = 0;
        }
    }
}
