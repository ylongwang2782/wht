#pragma once
#include <cstdint>

#include "bsp_adc.hpp"


class resistance {
   public:
    Adc adc;
    void init(uint8_t conductionNum, uint16_t totalConductionNum,
        uint16_t startConductionNum) {
        adc.init();
    }
};