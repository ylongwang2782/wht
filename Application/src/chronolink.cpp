#include "chronolink.h"

#include <cstdint>
#include <cstdio>
#include <vector>

#include "log.h"

std::vector<DeviceConfigInfo> ChronoLink::sync_frame;
std::vector<std::array<uint8_t, 4>> ChronoLink::instruction_list;
CommandFrame ChronoLink::command_frame;

void ChronoLink::receiveAndAssembleFrame(const FrameFragment& fragment) {
    if (fragment.delimiter[0] == 0xAB || fragment.delimiter[1] == 0xCD) {
        complete_frame.data.insert(complete_frame.data.end(),
                                   fragment.padding.begin(),
                                   fragment.padding.end());
                                   DBGF("Insert padding data");
    }

    // Check if this is the last fragment
    if (fragment.more_fragments_flag == 0) {
        complete_frame.slot = fragment.slot;
        complete_frame.type = fragment.type;
        complete_frame.is_complete = true;
        frameSorting(complete_frame);
        // Clear buffer after assembly
        complete_frame.data.clear();
    }
}

void ChronoLink::frameSorting(CompleteFrame complete_frame) {
    switch (complete_frame.type) {
        case SYNC:
            printf("RECV: SYNC FRAME\n");
            break;
        case COMMAND:
            printf("RECV: COMMAND FRAME\n");
            break;
        default:
            break;
    }
}

void ChronoLink::setBit(uint32_t& num, int n) { num |= (1 << n); }

std::vector<uint8_t> ChronoLink::serializeSyncFrame(
    const std::vector<DeviceConfigInfo>& sync_frame) {
    std::vector<uint8_t> serialized;
    serialized.reserve(sync_frame.size() * 5);  // 预留空间

    for (const auto& device : sync_frame) {
        serialized.insert(serialized.end(), device.ID.begin(), device.ID.end());
        serialized.push_back(device.pin_num);
    }

    return serialized;
}
void ChronoLink::packSyncFrame(uint8_t slot, uint8_t type,
                               std::vector<std::vector<uint8_t>>& output) {
    std::vector<uint8_t> serializedData =
        serializeSyncFrame(ChronoLink::sync_frame);
    uint16_t len = serializedData.size();
    ChronoLink::pack(slot, type, serializedData.data(), len, output);
}

std::vector<uint8_t> ChronoLink::serializeCommandFrame(
    const CommandFrame& command_frame) {
    std::vector<uint8_t> serialized;

    serialized.push_back(command_frame.type);
    serialized.push_back((command_frame.slot_index >> 24) & 0xFF);  // 高字节
    serialized.push_back((command_frame.slot_index >> 16) & 0xFF);
    serialized.push_back((command_frame.slot_index >> 8) & 0xFF);
    serialized.push_back(command_frame.slot_index & 0xFF);  // 低字节

    return serialized;
}
void ChronoLink::packCommandFrame(std::vector<std::vector<uint8_t>>& output) {
    uint8_t slot = 0;
    uint8_t type = 0x03;
    std::vector<uint8_t> serializedData =
        serializeCommandFrame(ChronoLink::command_frame);
    uint16_t len = serializedData.size();
    ChronoLink::pack(slot, type, serializedData.data(), len, output);
}

uint8_t ChronoLink::pack(uint8_t slot, uint8_t type, uint8_t* data,
                         uint16_t len,
                         std::vector<std::vector<uint8_t>>& output) {
    uint8_t fragments_num = (len + payload_size - 1) / payload_size;
    output.resize(fragments_num);

    for (uint8_t i = 0; i < fragments_num; i++) {
        FrameFragment frame;
        frame.delimiter = {0xAB, 0xCD};

        size_t current_payload_size =
            (i == fragments_num - 1) ? (len % payload_size) : payload_size;
        if (current_payload_size == 0) {
            current_payload_size = payload_size;
        }

        frame.len = 8 + current_payload_size;
        frame.slot = slot;
        frame.type = type;
        frame.fragment_sequence = i;
        frame.more_fragments_flag = 1;
        if (i == fragments_num - 1) {
            frame.more_fragments_flag = 0;
        }

        frame.padding.resize(current_payload_size);
        memcpy(frame.padding.data(), &data[i * payload_size],
               current_payload_size);

        // Append frame to output
        std::vector<uint8_t>& fragment = output[i];
        fragment.reserve(8 +
                         current_payload_size);  // Reserve enough space to
                                                 // avoid multiple allocations
        fragment.insert(fragment.end(), frame.delimiter.begin(),
                        frame.delimiter.end());
        fragment.push_back((frame.len >> 8) & 0xFF);  // High byte of len
        fragment.push_back(frame.len & 0xFF);         // Low byte of len
        fragment.push_back(frame.slot);
        fragment.push_back(frame.type);
        fragment.push_back(frame.fragment_sequence);
        fragment.push_back(frame.more_fragments_flag);
        fragment.insert(fragment.end(), frame.padding.begin(),
                        frame.padding.end());
    }

    return fragments_num;
}
