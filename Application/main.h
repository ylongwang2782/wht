#include <array>
#include <cstdlib>
#include <cstring>
extern "C" {
#include "systick.h"
}
#include "chronolink.h"
#include "com.h"
#include "json_interface.h"

class UIDReader {
   public:
    UIDReader(uint32_t address) {
        uidAddress = reinterpret_cast<uint32_t *>(address);
        readUID();
    }

    static std::array<uint8_t, 4> UID;

   private:
    uint32_t *uidAddress;

    void readUID() {
        uint32_t uid = *uidAddress;
        for (int i = 0; i < 4; ++i) {
            UID[i] = static_cast<uint8_t>((uid >> (24 - i * 8)) & 0xFF);
        }
    }
};
