#include "json_interface.h"

#include <cstdint>
#include <cstdio>
#include <vector>

#include "cJSON.h"
#include "chronolink.h"

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
                FrameParser::FrameProc(frame);
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
                ChronoLink::sync_frame.clear();
                int config_size = cJSON_GetArraySize(config);
                for (int i = 0; i < config_size; i++) {
                    cJSON *config_item = cJSON_GetArrayItem(config, i);
                    cJSON *id = cJSON_GetObjectItem(config_item, "id");
                    cJSON *pinNum = cJSON_GetObjectItem(config_item, "pinNum");

                    // Check if "id" is an array of 4 elements and "pinNum" is a
                    // number
                    if (cJSON_IsArray(id) && cJSON_GetArraySize(id) == 4 &&
                        cJSON_IsNumber(pinNum)) {
                        ChronoLink::DeviceConfigInfo device;

                        for (int j = 0; j < 4; j++) {
                            cJSON *id_item = cJSON_GetArrayItem(id, j);
                            if (cJSON_IsString(id_item)) {
                                // Convert hex string to uint8_t
                                device.ID[j] = static_cast<uint8_t>(
                                    strtol(id_item->valuestring, nullptr, 16));
                            }
                        }

                        // Set the pin number
                        device.pin_num = static_cast<uint8_t>(pinNum->valueint);

                        // Store the device into sync_frame
                        ChronoLink::sync_frame.push_back(device);
                    }
                }

                // Add success message to JSON reply
                cJSON_AddStringToObject(jsonReply, "config", "success");
            }

            if (1) {
                // Print the received sync frame
                for (auto device : ChronoLink::sync_frame) {
                    printf("ID: %X%X%X%X, pinNum: %d\n", device.ID[0],
                           device.ID[1], device.ID[2], device.ID[3],
                           device.pin_num);
                }
            }
            // Parse "control"
            cJSON *control = cJSON_GetObjectItem(root, "control");
            if (cJSON_IsString(control) && (control->valuestring != NULL)) {
                // Check value valid
                if (strcmp(control->valuestring, "enable") == 0 ||
                    strcmp(control->valuestring, "disable") == 0) {
                    cJSON_AddStringToObject(jsonReply, "control",
                                            control->valuestring);
                } else {
                    cJSON_AddStringToObject(jsonReply, "control", "error");
                }
            }

        } else if (strcmp(instruction->valuestring, "unlock") == 0) {
            // Unlock instruction received
            ChronoLink::instruction_list.clear();
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
                        ChronoLink::instruction_list.push_back(ID);
                    }
                }
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

void FrameParser::FrameProc(Frame *frame) {
    uint8_t frame_type = frame->frame_type;

    if (frame_type == TYPE_JSON) {
        const char *json_data =
            reinterpret_cast<const char *>(frame->data_pad.data());

        cJSON *root = cJSON_Parse(json_data);

        if (root == NULL) {
            printf("Error before: [%s]\n", cJSON_GetErrorPtr());
            return;
        }

        cJSON *instruction = cJSON_GetObjectItem(root, "instruction");
        if (cJSON_IsString(instruction) && (instruction->valuestring != NULL)) {
            if (strcmp(instruction->valuestring, "acquisition") == 0) {
                // Acquisition instruction received
                // Parse "config"
                cJSON *config = cJSON_GetObjectItem(root, "config");
                if (cJSON_IsArray(config)) {
                    ChronoLink::sync_frame.clear();

                    int config_size = cJSON_GetArraySize(config);
                    for (int i = 0; i < config_size; i++) {
                        cJSON *config_item = cJSON_GetArrayItem(config, i);
                        cJSON *id = cJSON_GetObjectItem(config_item, "id");
                        cJSON *pinNum =
                            cJSON_GetObjectItem(config_item, "pinNum");

                        if (cJSON_IsArray(id) && cJSON_GetArraySize(id) == 4 &&
                            cJSON_IsNumber(pinNum)) {
                            ChronoLink::DeviceConfigInfo device;

                            for (int j = 0; j < 4; j++) {
                                device.ID[j] = static_cast<uint8_t>(
                                    cJSON_GetArrayItem(id, j)->valueint);
                            }
                            device.pin_num =
                                static_cast<uint8_t>(pinNum->valueint);

                            ChronoLink::sync_frame.push_back(device);
                        }
                    }
                }

                // Parse "control"
                cJSON *control = cJSON_GetObjectItem(root, "control");
                if (cJSON_IsString(control) && (control->valuestring != NULL)) {
                    printf("Control: %s\n", control->valuestring);
                }

                cJSON_Delete(root);

            } else if (strcmp(instruction->valuestring, "unlock") == 0) {
                // Unlock instruction received
            }
        }
    }
}