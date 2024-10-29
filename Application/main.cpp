
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

int main(void) {
    systick_config();

    FrameParser parser;

    Timer timer;
    timer.init(50, ledTask);

    while (1) {
        if (usart1_config.rx_count > 0) {
            
            parser.JsonParse((char *)com1.rxbuffer);
            memset(com1.rxbuffer, 0, sizeof(com1.rxbuffer));

            usart1_config.rx_count = 0;
        }
    }
}
