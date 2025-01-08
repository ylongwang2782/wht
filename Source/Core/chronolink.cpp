#include "chronolink.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "bsp_log.h"
#include "bsp_uid.h"
#include "harness.h"

extern Harness harness;
extern Logger Log;
extern USART_DMA_Handler uartDMA;
std::vector<ChronoLink::DevConf> ChronoLink::sync_frame;
std::vector<std::array<uint8_t, 4>> ChronoLink::instruction_list;
CommandFrame ChronoLink::command_frame;

void ChronoLink::push_back(const uint8_t* data, size_t length) {
    receive_buffer.insert(receive_buffer.end(), data, data + length);
}

bool ChronoLink::parseFrameFragment(FrameFragment& fragment) {
    size_t min_packet_size = 8;    // Minimum packet size (2 bytes header + 2
                                   // bytes len + 4 bytes fixed fields)
    size_t index = 0;
    while (index + min_packet_size <= receive_buffer.size()) {
        // 检查起始标志 0xAB 0xCD
        if (receive_buffer[index] != 0xAB ||
            receive_buffer[index + 1] != 0xCD) {
            Log.w("Invalid header delimiter at index %d\n", index);
            ++index;    // 跳过无效字节，继续检查下一个可能的帧
            continue;
        }

        // Parse header
        fragment.delimiter[0] = receive_buffer[index++];
        fragment.delimiter[1] = receive_buffer[index++];

        // // Check if the buffer contains the full packet
        // if (receive_buffer.size() < min_packet_size + fragment.len) {
        //     WARNF("Incomplete packet\n");
        //     return false;    // Incomplete packet
        // }

        // Parse slot, type, fragment_sequence, and more_fragments_flag
        fragment.slot = receive_buffer[index++];
        fragment.type = receive_buffer[index++];
        fragment.fragment_sequence = receive_buffer[index++];
        fragment.more_fragments_flag = receive_buffer[index++];

        // Parse length (little-endian)
        fragment.len = static_cast<uint16_t>(receive_buffer[index] |
                                             (receive_buffer[index + 1] << 8));
        index += 2;

        // Parse padding (data field)
        fragment.padding.assign(receive_buffer.begin() + index,
                                receive_buffer.begin() + index + fragment.len);

        // Remove parsed data from buffer
        receive_buffer.erase(receive_buffer.begin(),
                             receive_buffer.begin() + index + fragment.len);
        return true;
    }
    return false;
}

void ChronoLink::receiveAndAssembleFrame(
    const FrameFragment& fragment,
    void (*frameSorting)(ChronoLink::CompleteFrame complete_frame)) {
    CompleteFrame complete_frame;

    // Extract data from fragment
    complete_frame.data.insert(complete_frame.data.end(),
                               fragment.padding.begin(),
                               fragment.padding.end());

    // Check if this is the last fragment
    if (fragment.more_fragments_flag == 0) {
        complete_frame.slot = fragment.slot;
        complete_frame.type = fragment.type;

        // 调用传入的 frameSorting 函数指针
        frameSorting(complete_frame);

        // Clear buffer after assembly
        complete_frame.data.clear();
    }
}

ChronoLink::Instruction ChronoLink::parseInstruction(
    const std::vector<uint8_t>& rawData) {
    Instruction instruction;
    size_t index = 0;

    // 解析 type
    instruction.type = rawData[index++];

    // 解析 targetID（假设长度固定为 4 字节）
    for (size_t i = 0; i < 4; ++i) {
        instruction.targetID.push_back(rawData[index++]);
    }

    // 根据 type 解析 context
    if (instruction.type == 0x00) {
        // DeviceConfig
        DeviceConfig config;
        config.timeslot = rawData[index++];
        config.totalHarnessNum = rawData[index++] | (rawData[index++] << 8);
        config.startHarnessNum = rawData[index++] | (rawData[index++] << 8);
        config.harnessNum = rawData[index++];
        config.clipNum = rawData[index++];

        // 剩余字节作为 resNum 列表
        while (index < rawData.size()) {
            config.resNum.push_back(rawData[index++]);
        }
        instruction.context = config;
    } else if (instruction.type == 0x02) {
        // DeviceUnlock
        DeviceUnlock unlock;
        unlock.lockStatus = rawData[index++];
        instruction.context = unlock;
    } else {
    }

    return instruction;
}

void ChronoLink::setBit(uint32_t& num, int n) { num |= (1 << n); }

std::vector<uint8_t> ChronoLink::serializeSyncFrame(
    const std::vector<DevConf>& sync_frame) {
    std::vector<uint8_t> serialized;
    serialized.reserve(sync_frame.size() * 5);    // 预留空间

    for (const auto& device : sync_frame) {
        serialized.insert(serialized.end(), device.ID.begin(), device.ID.end());
        serialized.push_back(device.harnessNum);
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
    serialized.push_back((command_frame.slot_index >> 24) & 0xFF);    // 高字节
    serialized.push_back((command_frame.slot_index >> 16) & 0xFF);
    serialized.push_back((command_frame.slot_index >> 8) & 0xFF);
    serialized.push_back(command_frame.slot_index & 0xFF);    // 低字节

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

// 生成回复数据帧
std::vector<uint8_t> ChronoLink::generateReplyFrame(
    uint8_t type, uint8_t ackStatus,
    const std::variant<DeviceConfig, DataReplyContext, DeviceUnlock>& context) {
    std::vector<uint8_t> frame;

    // 添加指令类型
    frame.push_back(type);

    // 添加应答状态
    frame.push_back(ackStatus);

    // 根据上下文类型填充数据
    if (std::holds_alternative<DeviceConfig>(context)) {
    } else if (std::holds_alternative<DataReplyContext>(context)) {
        const auto& dataReply = std::get<DataReplyContext>(context);

        // 添加设备状态
        uint16_t status = 0;
        memcpy(&status, &dataReply.deviceStatus, sizeof(DeviceStatus));
        frame.push_back(static_cast<uint8_t>(status & 0xFF));
        frame.push_back(static_cast<uint8_t>((status >> 8) & 0xFF));

        // 添加线束数据
        frame.push_back(dataReply.harnessLength);
        frame.insert(frame.end(), dataReply.harnessData.begin(),
                     dataReply.harnessData.end());

        // 添加卡钉数据
        frame.push_back(dataReply.clipLength);
        frame.insert(frame.end(), dataReply.clipData.begin(),
                     dataReply.clipData.end());

    } else if (std::holds_alternative<DeviceUnlock>(context)) {
        const auto& unlock = std::get<DeviceUnlock>(context);

        frame.push_back(unlock.lockStatus);
    }

    return frame;
}

// 回复指令
void ChronoLink::sendReply(
    uint8_t slot, uint8_t type, uint8_t instructionType, uint8_t ackStatus,
    const std::variant<DeviceConfig, DataReplyContext, DeviceUnlock>& context) {
    // 生成数据帧
    std::vector<uint8_t> frame =
        generateReplyFrame(instructionType, ackStatus, context);

    std::vector<std::vector<uint8_t>> fragments;

    int fragmentNum = pack(slot, type, frame.data(), frame.size(), fragments);

    for (int i = 0; i < fragmentNum; i++) {
        std::vector<uint8_t>& fragment = fragments[i];
        uartDMA.dma_tx(fragment.data(), fragment.size());
    }
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
                         current_payload_size);    // Reserve enough space to
                                                   // avoid multiple allocations
        fragment.insert(fragment.end(), frame.delimiter.begin(),
                        frame.delimiter.end());
        fragment.push_back(frame.len & 0xFF);           // Low byte of len
        fragment.push_back((frame.len >> 8) & 0xFF);    // High byte of len
        fragment.push_back(frame.slot);
        fragment.push_back(frame.type);
        fragment.push_back(frame.fragment_sequence);
        fragment.push_back(frame.more_fragments_flag);
        fragment.insert(fragment.end(), frame.padding.begin(),
                        frame.padding.end());
    }

    return fragments_num;
}
