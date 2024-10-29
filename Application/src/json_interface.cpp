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
    // const char *json_data = reinterpret_cast<const char *>(data.data());

    // Define the JSON object
    cJSON *root = cJSON_Parse(data);
    // Define the JSON reply object
    cJSON *jsonReply = cJSON_CreateObject();

    if (root == NULL) {
        cJSON_AddStringToObject(jsonReply, "result", "error");
        return;
    }

    cJSON *instruction = cJSON_GetObjectItem(root, "instruction");
    if (cJSON_IsString(instruction) && (instruction->valuestring != NULL)) {
        if (strcmp(instruction->valuestring, "acquisition") == 0) {
            // Acquisition instruction received
            printf("Acquisition instruction received\n");
            cJSON *config = cJSON_GetObjectItem(root, "config");
            if (cJSON_IsArray(config)) {
                ChronoLink::sync_frame.clear();
                int config_size = cJSON_GetArraySize(config);
                for (int i = 0; i < config_size; i++) {
                    cJSON *config_item = cJSON_GetArrayItem(config, i);
                    cJSON *id = cJSON_GetObjectItem(config_item, "id");
                    cJSON *pinNum = cJSON_GetObjectItem(config_item, "pinNum");

                    if (cJSON_IsArray(id) && cJSON_GetArraySize(id) == 4 &&
                        cJSON_IsNumber(pinNum)) {
                        ChronoLink::DeviceInfo device;

                        for (int j = 0; j < 4; j++) {
                            device.ID[j] = static_cast<uint8_t>(
                                cJSON_GetArrayItem(id, j)->valueint);
                        }
                        device.pin_num = static_cast<uint8_t>(pinNum->valueint);

                        ChronoLink::sync_frame.push_back(device);
                    }
                }
                cJSON_AddStringToObject(jsonReply, "config", "success");
            }

            if (1) {
                // Print the received sync frame
                printf("Sync frame received:\n");
                for (auto device : ChronoLink::sync_frame) {
                    printf("ID: %X%X%X%X, pinNum: %d\n", device.ID[0],
                           device.ID[1], device.ID[2], device.ID[3],
                           device.pin_num);
                }
            }

            // Parse "control"
            cJSON *control = cJSON_GetObjectItem(root, "control");
            if (cJSON_IsString(control) && (control->valuestring != NULL)) {
                cJSON_AddStringToObject(jsonReply, "control",
                                        control->valuestring);
            }

            // Print the reply
            cJSON *jsonRootReply = cJSON_CreateObject();
            cJSON_AddItemToObject(jsonRootReply, "result", jsonReply);
            char *jsonReplyStr = cJSON_PrintUnformatted(jsonRootReply);
            printf("%s", jsonReplyStr);

            cJSON_Delete(jsonRootReply);
            free(jsonReplyStr);

        } else if (strcmp(instruction->valuestring, "unlock") == 0) {
            // Unlock instruction received
        }
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
                            ChronoLink::DeviceInfo device;

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