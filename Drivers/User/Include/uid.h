#include <array>
#include <cstdint>
#include <cstdio>

#include "log.h"

class uid {
   public:
    static void get(std::array<uint8_t, 4> &uid_array) {
        uint32_t *uid_address = reinterpret_cast<uint32_t *>(0x1FFF7A10);
        uint32_t uid_value = *uid_address;
        for (int i = 0; i < 4; ++i) {
            uid_array[i] =
                static_cast<uint8_t>((uid_value >> (24 - i * 8)) & 0xFF);
        }
        DBGF("UID: %X-%X-%X-%X\n", uid_array[0], uid_array[1], uid_array[2],
             uid_array[3]);
    }
};