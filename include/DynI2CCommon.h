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


#define DYNI2C_FLAG_NO_CACHE    (1 << 0)
#define DYNI2C_FLAG_READABLE    (1 << 1)
#define DYNI2C_FLAG_WRITABLE    (1 << 2)
#define DYNI2C_FLAG_TYPE_BLOB   (1 << 3)
#define DYNI2C_FLAG_TYPE_BOOL   (1 << 4)
#define DYNI2C_FLAG_TYPE_UINT   (1 << 5)
#define DYNI2C_FLAG_TYPE_INT    (1 << 6)
#define DYNI2C_FLAG_TYPE_FLOAT  (1 << 7)


bool dyni2c_flags_no_caching(uint8_t data_flags) { return data_flags & DYNI2C_FLAG_NO_CACHE; }
bool dyni2c_flags_readable(uint8_t data_flags) { return data_flags & DYNI2C_FLAG_READABLE; }
bool dyni2c_flags_writable(uint8_t data_flags) { return data_flags & DYNI2C_FLAG_WRITABLE; }
bool dyni2c_flags_type_blob(uint8_t data_flags) { return data_flags & DYNI2C_FLAG_TYPE_BLOB; }
bool dyni2c_flags_type_bool(uint8_t data_flags) { return data_flags & DYNI2C_FLAG_TYPE_BOOL; }
bool dyni2c_flags_type_uint(uint8_t data_flags) { return data_flags & DYNI2C_FLAG_TYPE_UINT; }
bool dyni2c_flags_type_int(uint8_t data_flags) { return data_flags & DYNI2C_FLAG_TYPE_INT; }
bool dyni2c_flags_type_float(uint8_t data_flags) { return data_flags & DYNI2C_FLAG_TYPE_FLOAT; }


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
    return calculate_crc8(
        reinterpret_cast<const uint8_t*>(&packet),
        sizeof(packet) - sizeof(packet.crc_checksum));
}


inline uint8_t calculate_crc8(dynI2C_metamsg_packet_t packet)
{
    return calculate_crc8(
        reinterpret_cast<const uint8_t*>(&packet),
        sizeof(packet) - sizeof(packet.crc_checksum));
}


inline uint8_t calculate_crc8(dynI2C_getdata_packet_t packet)
{
    return calculate_crc8(
        reinterpret_cast<const uint8_t*>(&packet),
        sizeof(packet) - sizeof(packet.crc_checksum));
}


inline uint8_t calculate_crc8(dynI2C_setdata_packet_t packet)
{
    return calculate_crc8(
        reinterpret_cast<const uint8_t*>(&packet),
        sizeof(packet) - sizeof(packet.crc_checksum));
}


inline dynI2C_getmeta_packet_t build_getmeta_packet(uint8_t key)
{
    dynI2C_getmeta_packet_t packet = { .data_key = key };
    packet.crc_checksum = calculate_crc8(packet);
    return packet;
}


inline dynI2C_getdata_packet_t build_getdata_packet(uint8_t key)
{
    dynI2C_getdata_packet_t packet = { .data_key = key, };
    packet.crc_checksum = calculate_crc8(packet);
    return packet;
}


inline dynI2C_setdata_packet_t build_setdata_packet(uint8_t key, uint16_t data_len)
{
    dynI2C_setdata_packet_t packet = { .data_key = key, .data_len = data_len, };
    packet.crc_checksum = calculate_crc8(packet);
    return packet;
}

#endif // DYNI2C_COMMON_H