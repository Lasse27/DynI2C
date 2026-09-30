/**
 * @file DynI2CPackets.h
 * @brief Common definitions and types of the packets of the protocol.
 *
 * @author Lasse-Leander Hillen
 * @date 2026-09-24
 * @version 1.0.0
 */
#include <cstdint>

#ifndef DYNI2C_PACKETS_H
#define DYNI2C_PACKETS_H

 // Common magic number at the start of each packet
#define DYNI2C_MAGIC_NUMBER 0xC4

 // Different packet types that are possible in the communication
#define DYNI2C_PACKET_TYPE_GETMETA 0x00
#define DYNI2C_PACKET_TYPE_GETDATA 0x01
#define DYNI2C_PACKET_TYPE_SETDATA 0x02
#define DYNI2C_PACKET_TYPE_METAMSG 0x03
#define DYNI2C_PACKET_TYPE_DATAMSG 0x04


/**
 * @brief Struct that represents the metadata of a client register.
 * @note `__attribute__((packed))` so that the conversion into a byte array is possible and padding is ignored.
 */
typedef struct __attribute__((packed))
{
    uint8_t data_flags;
    uint16_t data_len;
} dynI2C_metadata_t;


/**
 * @brief Struct that represents a GETMETA packet. `data_key` is the register key whose meta data is being requested.
 * @note `__attribute__((packed))` so that the conversion into a byte array is possible and padding is ignored.
 */
typedef struct __attribute__((packed))
{
    uint8_t magic = DYNI2C_MAGIC_NUMBER;
    uint8_t packet_type = DYNI2C_PACKET_TYPE_GETMETA;
    uint8_t data_key = 0; // Key whose meta data is being requested
    uint8_t crc_checksum = 0;
} dynI2C_getmeta_packet_t;

/**
 * @brief Struct that represents a GETINFO packet. `data_key` is the register key whose meta data is being requested.
 * @note `__attribute__((packed))` so that the conversion into a byte array is possible and padding is ignored.
 */
typedef struct __attribute__((packed))
{
    uint8_t magic = DYNI2C_MAGIC_NUMBER;
    uint8_t packet_type = DYNI2C_PACKET_TYPE_GETMETA;
    uint8_t data_key = 0; // Key whose meta data is being requested
    uint8_t crc_checksum = 0;
} dynI2C_geinfo_packet_t;

/**
 * @brief Struct that represents a GETDATA packet. `data_key` is the register key whose data is being requested.
 * @note `__attribute__((packed))` so that the conversion into a byte array is possible and padding is ignored.
 */
typedef struct __attribute__((packed))
{
    uint8_t magic = DYNI2C_MAGIC_NUMBER;
    uint8_t packet_type = DYNI2C_PACKET_TYPE_GETDATA;
    uint8_t data_key = 0; // Key whose data is being requested
    uint8_t crc_checksum = 0;
} dynI2C_getdata_packet_t;


/**
 * @brief Struct that represents a SETDATA packet. `data_key` is the register key whose data is being sent.
 * @note `__attribute__((packed))` so that the conversion into a byte array is possible and padding is ignored.
 */
typedef struct __attribute__((packed))
{
    uint8_t magic = DYNI2C_MAGIC_NUMBER;
    uint8_t packet_type = DYNI2C_PACKET_TYPE_SETDATA;
    uint8_t data_key = 0;      // Key whose data is being send
    uint16_t data_len = 0;     // Length of upcoming data
    uint8_t crc_checksum = 0;
} dynI2C_setdata_packet_t;


/**
 * @brief Struct that represents a METAMSG packet. `meta` is the packet body and contains the metadata requested.
 * @note `__attribute__((packed))` so that the conversion into a byte array is possible and padding is ignored.
 */
typedef struct __attribute__((packed))
{
    uint8_t magic = DYNI2C_MAGIC_NUMBER;
    uint8_t packet_type = DYNI2C_PACKET_TYPE_METAMSG;
    uint8_t data_flags = 0;
    uint16_t data_len = 0;
    uint8_t crc_checksum = 0;
} dynI2C_metamsg_packet_t;


#endif // DYNI2C_PACKETS_H