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
#include "DynI2CPackets.h"


 /**
  * @brief Enumeration of the different flags that are specified in the header of the packets
  * that are being sent.
  */
typedef enum : uint8_t
{
    DYNAMIC_FLAG = (1 << 0),    // Flags and data lenght vary, DONT CACHE
    READABLE_FLAG = (1 << 1),   // Register can be read
    WRITABLE_FLAG = (1 << 2),   // Register can be written
    TYPE_BLOB_FLAG = (1 << 3),  // Register contains unspecified bytes.
    TYPE_BOOL_FLAG = (1 << 4),  // Register contains boolean.
    TYPE_UINT_FLAG = (1 << 5),  // Register contains unsigned integer.
    TYPE_INT_FLAG = (1 << 6),   // Register contains signed integer.
    TYPE_FLOAT_FLAG = (1 << 7), // Register contains floating point number.
} dynI2C_data_flag_t;



inline uint8_t calculate_crc8(const uint8_t* data, size_t len)
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

inline uint8_t calculate_crc8(dynI2C_getmeta_packet_t packet)
{
    return calculate_crc8(reinterpret_cast<const uint8_t*>(&packet),
        sizeof(packet) - sizeof(packet.crc_checksum));
}

inline uint8_t calculate_crc8(dynI2C_metamsg_packet_t packet)
{
    return calculate_crc8(reinterpret_cast<const uint8_t*>(&packet),
        sizeof(packet) - sizeof(packet.crc_checksum));
}

inline uint8_t calculate_crc8(dynI2C_getdata_packet_t packet)
{
    return calculate_crc8(reinterpret_cast<const uint8_t*>(&packet),
        sizeof(packet) - sizeof(packet.crc_checksum));
}

inline uint8_t calculate_crc8(dynI2C_setdata_packet_t packet)
{
    return calculate_crc8(reinterpret_cast<const uint8_t*>(&packet),
        sizeof(packet) - sizeof(packet.crc_checksum));
}


/**
 * @brief Creates a GETMETA packet for a specific data key.
 * @param key The uint8_t key for the data to access.
 * @return A struct of type dynI2C_getmeta_packet_t
 */
inline dynI2C_getmeta_packet_t build_getmeta_packet(uint8_t key)
{
    dynI2C_getmeta_packet_t packet = {
        .header = {
            .magic = DYNI2C_MAGIC_NUMBER,
            .packet_type = GETMETA,
        },
        .data_key = key,
    };
    packet.crc_checksum = calculate_crc8(packet);
    return packet;
}

/// @brief Creates a GETDATA packet for a specific data key.
/// @param key The uint8_t key for the data to access.
/// @return A struct of type dynI2C_getdata_packet_t
inline dynI2C_getdata_packet_t build_getdata_packet(uint8_t key)
{
    dynI2C_getdata_packet_t packet = {
        .header = {
            .magic = DYNI2C_MAGIC_NUMBER,
            .packet_type = GETDATA,
        },
        .data_key = key,
    };
    packet.crc_checksum = calculate_crc8(packet);
    return packet;
}

#endif // DYNI2C_COMMON_H