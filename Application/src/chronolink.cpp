#include "chronolink.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "conduction.h"
#include "log.h"
#include "uid.h"

extern Conduction conduction;

std::vector<ChronoLink::DevConf> ChronoLink::sync_frame;
std::vector<std::array<uint8_t, 4>> ChronoLink::instruction_list;
CommandFrame ChronoLink::command_frame;

void ChronoLink::receiveData(const uint8_t* data, size_t length) {
    receive_buffer.insert(receive_buffer.end(), data, data + length);
}

bool ChronoLink::parseFrameFragment(FrameFragment& fragment) {
    size_t min_packet_size = 8;  // Minimum packet size (2 bytes header + 2
                                 // bytes len + 4 bytes fixed fields)

    // Check if we have enough data to parse the minimum structure
    if (receive_buffer.size() < min_packet_size) {
        receive_buffer.clear();
        return false;
    }

    // Initialize index for parsing
    size_t index = 0;

    // Parse header
    fragment.delimiter[0] = receive_buffer[index++];
    fragment.delimiter[1] = receive_buffer[index++];

    // Parse length (little-endian)
    fragment.len = static_cast<uint16_t>(receive_buffer[index] |
                                         (receive_buffer[index + 1] << 8));
    index += 2;

    // Check if the buffer contains the full packet
    if (receive_buffer.size() < min_packet_size + fragment.len) {
        WARNF("Incomplete packet\n");
        return false;  // Incomplete packet
    }

    // Parse slot, type, fragment_sequence, and more_fragments_flag
    fragment.slot = receive_buffer[index++];
    fragment.type = receive_buffer[index++];
    fragment.fragment_sequence = receive_buffer[index++];
    fragment.more_fragments_flag = receive_buffer[index++];

    // Parse padding (data field)
    fragment.padding.assign(receive_buffer.begin() + index,
                            receive_buffer.begin() + index + fragment.len);

    // Remove parsed data from buffer
    receive_buffer.erase(receive_buffer.begin(),
                         receive_buffer.begin() + index + fragment.len);

    return true;
}

void ChronoLink::receiveAndAssembleFrame(const FrameFragment& fragment) {
    CompleteFrame complete_frame;
    if (fragment.delimiter[0] == 0xAB || fragment.delimiter[1] == 0xCD) {
        complete_frame.data.insert(complete_frame.data.end(),
                                   fragment.padding.begin(),
                                   fragment.padding.end());
        // DBGF("Insert padding data\n");
    }

    // Check if this is the last fragment
    if (fragment.more_fragments_flag == 0) {
        complete_frame.slot = fragment.slot;
        complete_frame.type = fragment.type;
        frameSorting(complete_frame);
        // Clear buffer after assembly
        complete_frame.data.clear();
    }
}

/**
 * @brief frameSorting Sorts the received frame and excute corresponding actions
 *
 * @param complete_frame
 */
void ChronoLink::frameSorting(CompleteFrame complete_frame) {
    std::vector<DevConf> device_configs;
    switch (complete_frame.type) {
        case DEVICE_CONFIG:
            DBGF("RECV: SYNC FRAME\n");

            // Convert u8 byte array to DevConf struct
            parseDeviceConfigInfo(complete_frame.data, device_configs);
            // find self device config in device_configs
            DeviceConfigInfo localDevInfo;
            uid::get(localDevInfo.ID);

            localDevInfo.deviceCount = device_configs.size();
            for (const auto& device : device_configs) {
                localDevInfo.sysConductionPinNum += device.enabled_pin_num;
                if (device.ID == localDevInfo.ID) {
                    DBGF("ID match\n");
                    localDevInfo.devConductionPinNum = device.enabled_pin_num;
                }
            }

            if (localDevInfo.devConductionPinNum != 0) {
                conduction.config(localDevInfo);
            }

            break;
        case SYNC_SIGNAL:
            conduction.start();
            DBGF("RECV: SYNC SIGNAL\n");
            break;
        case CONDUCTION_DATA:
            DBGF("RECV: CONDUCTION DATA\n");
            break;
        case COMMAND:
            printf("RECV: COMMAND FRAME\n");
            conduction.start();
            break;
        case COMMAND_REPLY:
            printf("RECV: COMMAND REPLY\n");
            break;
        default:
            break;
    }
}

// Parse u8 byte vector to DevConf struct
ChronoLink::status ChronoLink::parseDeviceConfigInfo(
    const std::vector<uint8_t>& data, std::vector<DevConf>& device_configs) {
    constexpr size_t deviceConfigSize =
        sizeof(DevConf::ID) + sizeof(DevConf::enabled_pin_num);

    if (data.size() % deviceConfigSize != 0) {
        ERRF("Invalid device config data size\n");
        return status::ERROR;
    }

    for (size_t i = 0; i < data.size(); i += deviceConfigSize) {
        DevConf config;
        std::copy(data.begin() + i, data.begin() + i + 4, config.ID.begin());
        config.enabled_pin_num = data[i + 4];
        DBGF("ID: %X%X%X%X, pin_num: %d\n", config.ID[0], config.ID[1],
             config.ID[2], config.ID[3], config.enabled_pin_num);
        device_configs.push_back(config);
    }

    return status::OK;
}

void ChronoLink::setBit(uint32_t& num, int n) { num |= (1 << n); }

std::vector<uint8_t> ChronoLink::serializeSyncFrame(
    const std::vector<DevConf>& sync_frame) {
    std::vector<uint8_t> serialized;
    serialized.reserve(sync_frame.size() * 5);  // 预留空间

    for (const auto& device : sync_frame) {
        serialized.insert(serialized.end(), device.ID.begin(), device.ID.end());
        serialized.push_back(device.enabled_pin_num);
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
        fragment.push_back(frame.len & 0xFF);         // Low byte of len
        fragment.push_back((frame.len >> 8) & 0xFF);  // High byte of len
        fragment.push_back(frame.slot);
        fragment.push_back(frame.type);
        fragment.push_back(frame.fragment_sequence);
        fragment.push_back(frame.more_fragments_flag);
        fragment.insert(fragment.end(), frame.padding.begin(),
                        frame.padding.end());
    }

    return fragments_num;
}
