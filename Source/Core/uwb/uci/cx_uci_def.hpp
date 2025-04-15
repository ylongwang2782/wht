#ifndef UCI_DEF_HPP_
#define UCI_DEF_HPP_

/* -------------------- < UCI Control Packet Head Size > -------------------- */
#define UCI_CTRL_PKT_HDR_SIZE 0x04

/* --------------------------- < Max Payload len> -------------------------- */
#define MAX_PAYLOAD_LEN 127
#define CX_APP_DATA_TX_MAX_PAYLOAD_LEN 122

/* ------------------------- < Message type (MT) > ------------------------- */
#define MT_CMD 0x01
#define MT_RSP 0x02
#define MT_NTF 0x03

/* -------------------- < Packet Boundary Flag (PBF) > --------------------- */
#define PBF_COMPLETE 0x00
#define PBF_SEGMENT  0x01

/* --------------- < GID = 0b0000, Message name definition > --------------- */
#define GID0x00               0x00
#define CORE_DEVICE_RESET     0x0000
#define CORE_DEVICE_RESET_CMD CORE_DEVICE_RESET
#define CORE_DEVICE_RESET_RSP CORE_DEVICE_RESET

#define CORE_DEVICE_STATUS_NTF 0x0001

#define CORE_GET_DEVICE_INFO     0x0002
#define CORE_GET_DEVICE_INFO_CMD CORE_GET_DEVICE_INFO
#define CORE_GET_DEVICE_INFO_RSP CORE_GET_DEVICE_INFO

#define CORE_GET_CAPS_INFO     0x0003
#define CORE_GET_CAPS_INFO_CMD CORE_GET_CAPS_INFO
#define CORE_GET_CAPS_INFO_RSP CORE_GET_CAPS_INFO

#define CORE_SET_CONFIG     0x0004
#define CORE_SET_CONFIG_CMD CORE_SET_CONFIG
#define CORE_SET_CONFIG_RSP CORE_SET_CONFIG

#define CORE_GET_CONFIG     0x0005
#define CORE_GET_CONFIG_CMD CORE_GET_CONFIG
#define CORE_GET_CONFIG_RSP CORE_GET_CONFIG

#define CORE_GENERIC_ERROR_NTF 0x0007

#define CORE_QUERY_UWBS_TIMESTAMP     0x0008
#define CORE_QUERY_UWBS_TIMESTAMP_CMD CORE_QUERY_UWBS_TIMESTAMP
#define CORE_QUERY_UWBS_TIMESTAMP_RSP CORE_QUERY_UWBS_TIMESTAMP

/* --------------- < GID = 0b0011, Message name definition > --------------- */

#define GID0x03         0x03
#define CX_APP_DATA_TX     0x0000
#define CX_APP_DATA_TX_CMD CX_APP_DATA_TX
#define CX_APP_DATA_TX_RSP CX_APP_DATA_TX
#define CX_APP_DATA_TX_NTF CX_APP_DATA_TX

#define CX_APP_DATA_RX     0x0001
#define CX_APP_DATA_RX_CMD CX_APP_DATA_RX
#define CX_APP_DATA_RX_RSP CX_APP_DATA_RX
#define CX_APP_DATA_RX_NTF CX_APP_DATA_RX

#define CX_APP_DATA_STOP_RX     0x0002
#define CX_APP_DATA_STOP_RX_CMD CX_APP_DATA_STOP_RX
#define CX_APP_DATA_STOP_RX_RSP CX_APP_DATA_STOP_RX

/* --------------------- <Device Status definition> --------------------- */
#define DEVICE_STATE_READY 0x01
#define DEVICE_STATE_ACTIVE 0x02

#define DEVICE_STATE_ERROR 0xFF

/* ------------------------- <Status definition> ------------------------- */
#define STATUS_OK                   0x00
#define STATUS_REJECTED             0x01
#define STATUS_FAILED               0x02
#define STATUS_SYNTAX_ERROR         0x03
#define STATUS_INVALID_PARAM        0x04
#define STATUS_INVALID_RANGE        0x05
#define STATUS_INVALID_MESSAGE_SIZE 0x06
#define STATUS_UNKNOWN_GID          0x07
#define STATUS_UNKNOWN_OID          0x08
#define STATUS_READ_ONLY            0x09
#define STATUS_COMMAND_RETRY        0x0A

#endif    // UCI_DEF_HPP_
