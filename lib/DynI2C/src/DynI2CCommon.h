/**
 * @file DynI2CCommon.h
 * @brief Common definitions and type as header for different files of the project.
 *
 * @author Lasse-Leander Hillen
 * @date 2026-09-23
 * @version 1.0.0
 */

#ifndef DYNI2C_COMMON_H
#define DYNI2C_COMMON_H

#include <cstdint>
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c_master.h"

#define DYNI2C_MAGIC_NUMBER 0xC4
#define DYNI2C_MAX_DATA_LEN 1024

uint8_t calculate_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

uint8_t calculate_crc8(dynI2C_getmeta_packet_t packet)
{
    return calculate_crc8(reinterpret_cast<const uint8_t *>(&packet),
                          sizeof(packet) - sizeof(packet.crc_checksum));
}

uint8_t calculate_crc8(dynI2C_meta_packet_t packet)
{
    return calculate_crc8(reinterpret_cast<const uint8_t *>(&packet),
                          sizeof(packet) - sizeof(packet.crc_checksum));
}

uint8_t calculate_crc8(dynI2C_getdata_packet_t packet)
{
    return calculate_crc8(reinterpret_cast<const uint8_t *>(&packet),
                          sizeof(packet) - sizeof(packet.crc_checksum));
}

uint8_t calculate_crc8(dynI2C_data_packet_t packet)
{
    return calculate_crc8(reinterpret_cast<const uint8_t *>(&packet),
                          sizeof(packet) - sizeof(packet.crc_checksum));
}

uint8_t calculate_crc8(dynI2C_setdata_packet_t packet)
{
    return calculate_crc8(reinterpret_cast<const uint8_t *>(&packet),
                          sizeof(packet) - sizeof(packet.crc_checksum));
}

typedef enum : uint8_t // Enum that represents the different possible packet types.
{
    GETMETA = 0x00,
    GETDATA = 0x01,
    SETDATA = 0x02,

    META = 0x03,
    DATA = 0x04,
} packet_type_t;

typedef enum : uint8_t
{
    DYNAMIC_SIZE_FLAG = (1 << 0)
} data_flag_t;

typedef struct __attribute__((packed)) // Structure that represents a DynI2C Header packet.
{
    uint8_t magic = DYNI2C_MAGIC_NUMBER;
    packet_type_t packet_type;
} dynI2C_header_t;

typedef struct __attribute__((packed)) // Structure that represents meta data of a field.
{
    uint16_t data_len;                 // Length of the contained data
    uint8_t data[DYNI2C_MAX_DATA_LEN]; // Contained data
} dynI2C_data_t;

typedef struct __attribute__((packed)) // Structure that represents meta data of a field.
{
    uint8_t data_flags; // Bit flags of the data field.
    uint16_t data_len;  // Length of the contained data
} dynI2C_meta_t;

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Definitions of the different packets
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

typedef struct __attribute__((packed)) // Structure that represents a GETMETA DynI2C packet.
{
    dynI2C_header_t header; // Header of the package
    uint8_t data_key;       // Key whose meta data is being requested
    uint8_t crc_checksum;   // Footer of the package
} dynI2C_getmeta_packet_t;

typedef struct __attribute__((packed)) // Structure that represents a META DynI2C packet.
{
    dynI2C_header_t header; // Header of the package
    dynI2C_meta_t meta;     // Meta data of a field
    uint8_t crc_checksum;   // Footer of the package
} dynI2C_meta_packet_t;

typedef struct __attribute__((packed)) //  Structure that represents a GETDATA DynI2C packet.
{
    dynI2C_header_t header; // Header of the package
    uint8_t data_key;       // Key whose data is being requested
    uint8_t crc_checksum;   // Footer of the package
} dynI2C_getdata_packet_t;

typedef struct __attribute__((packed)) //  Structure that represents a GETDATA DynI2C packet.
{
    dynI2C_header_t header; // Header of the package
    uint8_t data_key;       // Key whose data is being send
    dynI2C_data_t data;     // Contained data
    uint8_t crc_checksum;   // Footer of the package
} dynI2C_setdata_packet_t;

typedef struct __attribute__((packed)) // Structure that represents a DATA DynI2C packet.
{
    dynI2C_header_t header;            // Header of the package
    uint8_t data[DYNI2C_MAX_DATA_LEN]; // Contained data
    uint8_t crc_checksum;              // Footer of the package
} dynI2C_data_packet_t;

#endif // DYNI2C_COMMON_H