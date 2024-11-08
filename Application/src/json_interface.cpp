#include "json_interface.h"

#include <cstdint>
#include <cstdio>
#include <vector>

#include "cJSON.h"
#include "chronolink.h"

std::vector<std::vector<uint8_t>> serializedSyncFrame;
std::vector<std::vector<uint8_t>> serializedCommandFrame;

ChronoLink chronoLink;
LED led0(RCU_GPIOC, GPIOC, GPIO_PIN_6);

Timer timer(50, []() {
    led0.toggle();
    if (!serializedSyncFrame.empty()) {
        for (size_t i = 0; i < serializedSyncFrame.size(); ++i) {
            for (uint8_t byte : serializedSyncFrame[i]) {
                printf("%02X ", byte);
            }
        }
    }

    if (!serializedCommandFrame.empty()) {
        for (size_t i = 0; i < serializedCommandFrame.size(); ++i) {
            for (uint8_t byte : serializedCommandFrame[i]) {
                printf("%02X ", byte);
            }
        }
        serializedCommandFrame.clear();
    }
});

FrameParser::FrameParser() {}

void FrameParser::FrameParse(std::vector<uint8_t> data) {
    static std::vector<uint8_t> serial_recv_buff;
    static size_t serial_recv_buff_len = 0;

    serial_recv_buff.insert(serial_recv_buff.end(), data.begin(), data.end());
    serial_recv_buff_len += data.size();

    // parse all frames in the buffer
    size_t offset = 0;
    while (offset + FrameParser::header_len <=
           serial_recv_buff_len) {  // At least one frame header
        auto *frame =
            reinterpret_cast<Frame *>(serial_recv_buff.data() + offset);

        if (frame->frame_delimiter[0] == 0xAB &&
            frame->frame_delimiter[1] == 0xCD) {
            uint8_t frame_len = frame->frame_len;
            if (offset + frame_len <= serial_recv_buff_len) {
                offset += frame_len;
            } else {
                break;
            }
        } else {
            offset++;
        }
    }

    // move remaining data to the beginning of the buffer
    if (offset < serial_recv_buff_len) {
        std::memmove(serial_recv_buff.data(), serial_recv_buff.data() + offset,
                     serial_recv_buff_len - offset);
    }
    serial_recv_buff_len -= offset;
    serial_recv_buff.resize(serial_recv_buff_len);
}

void FrameParser::JsonParse(const char *data) {
    // Define the JSON object
    cJSON *root = cJSON_Parse(data);
    // Define the JSON reply object
    cJSON *jsonReply = cJSON_CreateObject();

    if (root == NULL) {
        // Add error message to JSON reply
        cJSON_AddStringToObject(jsonReply, "result", "error");
        return;
    }

    cJSON *instruction = cJSON_GetObjectItem(root, "instruction");
    if (cJSON_IsString(instruction) && (instruction->valuestring != NULL)) {
        cJSON *jsonRootReply = cJSON_CreateObject();
        if (strcmp(instruction->valuestring, "acquisition") == 0) {
            // Acquisition instruction received
            cJSON *config = cJSON_GetObjectItem(root, "config");
            if (cJSON_IsArray(config)) {
                chronoLink.sync_frame.clear();
                int config_size = cJSON_GetArraySize(config);
                for (int i = 0; i < config_size; i++) {
                    cJSON *config_item = cJSON_GetArrayItem(config, i);
                    cJSON *id = cJSON_GetObjectItem(config_item, "id");
                    cJSON *pinNum = cJSON_GetObjectItem(config_item, "pinNum");

                    if (cJSON_IsString(id) && strlen(id->valuestring) == 8 &&
                        cJSON_IsNumber(pinNum)) {
                        ChronoLink::DeviceConfigInfo device;

                        // Parse the 8-character hex string into four bytes
                        for (int j = 0; j < 4; j++) {
                            std::string byteString =
                                std::string(id->valuestring).substr(j * 2, 2);
                            device.ID[j] = static_cast<uint8_t>(
                                strtol(byteString.c_str(), nullptr, 16));
                        }

                        // Set the pin number
                        device.pin_num = static_cast<uint8_t>(pinNum->valueint);

                        // Store the device into sync_frame
                        chronoLink.sync_frame.push_back(device);
                    }
                }

                // Action according to sync_frame
                // Get sum of total pinNum of all devices
                int total_pin_num = 0;
                for (auto device : chronoLink.sync_frame) {
                    total_pin_num += device.pin_num;
                }

                // Set timer trigger count to total pinNum of all devices
                Timer::trigger_count_ = total_pin_num;
                DBGF("Trigger count: %d\n", Timer::trigger_count_);

                serializedSyncFrame.clear();
                chronoLink.packSyncFrame(0, 0, serializedSyncFrame);

                // Add success message to JSON reply
                cJSON_AddStringToObject(jsonReply, "config", "success");
            }

            if (1) {
                // Print the received sync frame
                for (auto device : chronoLink.sync_frame) {
                    DBGF("ID: %X%X%X%X, pinNum: %d\n", device.ID[0],
                         device.ID[1], device.ID[2], device.ID[3],
                         device.pin_num);
                }
            }
            // Parse "control"
            cJSON *control = cJSON_GetObjectItem(root, "control");
            if (cJSON_IsString(control) && (control->valuestring != NULL)) {
                // Check value valid
                if (strcmp(control->valuestring, "enable") == 0) {
                    cJSON_AddStringToObject(jsonReply, "control",
                                            control->valuestring);
                    timer.start();
                } else if (strcmp(control->valuestring, "disable") == 0) {
                    cJSON_AddStringToObject(jsonReply, "control",
                                            control->valuestring);
                    timer.stop();
                } else {
                    cJSON_AddStringToObject(jsonReply, "control", "error");
                }
            }
        } else if (strcmp(instruction->valuestring, "unlock") == 0) {
            // Unlock instruction received
            chronoLink.instruction_list.clear();
            chronoLink.command_frame.type = 0;
            cJSON *param = cJSON_GetObjectItem(root, "param");
            if (cJSON_IsArray(param)) {
                int arraySize = cJSON_GetArraySize(param);
                for (int i = 0; i < arraySize; i++) {
                    cJSON *subArray = cJSON_GetArrayItem(param, i);
                    if (cJSON_IsArray(subArray)) {
                        int subArraySize = cJSON_GetArraySize(subArray);
                        std::array<uint8_t, 4> ID;
                        for (int j = 0; j < subArraySize && j < 4; j++) {
                            cJSON *value = cJSON_GetArrayItem(subArray, j);
                            if (cJSON_IsString(value)) {
                                // Convert hex string to integer
                                ID[j] = static_cast<uint8_t>(
                                    strtol(value->valuestring, nullptr, 16));
                            }
                        }

                        // compare the ID with the ID in sync_frame then add ID
                        // index to the paired list
                        chronoLink.command_frame.slot_index = 0;
                        for (int j = 0; j < chronoLink.sync_frame.size(); j++) {
                            if (chronoLink.sync_frame[j].ID[0] == ID[0] &&
                                chronoLink.sync_frame[j].ID[1] == ID[1] &&
                                chronoLink.sync_frame[j].ID[2] == ID[2] &&
                                chronoLink.sync_frame[j].ID[3] == ID[3]) {
                                // Set corresponding bit to 1 in slot_index
                                chronoLink.setBit(
                                    chronoLink.command_frame.slot_index, j);
                            }
                        }
                        chronoLink.instruction_list.push_back(ID);
                    }
                }
                chronoLink.packCommandFrame(serializedCommandFrame);

                cJSON_AddStringToObject(jsonReply, instruction->valuestring,
                                        "success");
            } else if (cJSON_IsString(param) && (param->valuestring != NULL)) {
                if (strcmp(param->valuestring, "ALL") == 0) {
                    cJSON_AddStringToObject(jsonReply, instruction->valuestring,
                                            "success");
                    cJSON_AddStringToObject(jsonReply, "param", "ALL");
                }
            }
        } else {
            cJSON_AddStringToObject(jsonReply, "instruction", "error");
        }
        // Print the reply
        cJSON_AddItemToObject(jsonRootReply, "result", jsonReply);
        char *jsonReplyStr = cJSON_PrintUnformatted(jsonRootReply);
        printf("%s", jsonReplyStr);
        free(jsonReplyStr);
        cJSON_Delete(jsonRootReply);
    }
    cJSON_Delete(root);
}