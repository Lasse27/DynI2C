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

#define DYNI2C_MAGIC_NUMBER 0xC4


 /**
 * @brief Enumeration of the different types of packets that are being sent in DynI2C transactions.
 * Each packet expresses its type via a field in the header.
 */
typedef enum : uint8_t
{
    GETMETA = 0x00,
    GETDATA = 0x01,
    SETDATA = 0x02,
    METAMSG = 0x03,
    DATAMSG = 0x04,
} dynI2C_packet_type_t;



/**
 * @brief Struct that represents the header of the packets that are being sent. This header
 * is the same for every DynI2C packet and not dependend on the transmitted packet type.
 *
 * @note `__attribute__((packed))` so that the conversion into a byte array is possible and padding is ignored.
 */
typedef struct __attribute__((packed))
{
    uint8_t magic = DYNI2C_MAGIC_NUMBER;
    dynI2C_packet_type_t packet_type;

} dynI2C_header_t;


/**
 * @brief Struct that represents the metadata of a client register. `data_flags` corresponds to the flags specified in
 * `dynI2C_data_flag_t`.
 *
 * @note `__attribute__((packed))` so that the conversion into a byte array is possible and padding is ignored.
 */
typedef struct __attribute__((packed))
{
    uint8_t data_flags;
    uint16_t data_len;

} dynI2C_meta_t;



// ==========================================================================================================
// Packet types that have a definitive size:
// ==========================================================================================================

/**
 * @brief Struct that represents a GETMETA packet. `data_key` is the register key whose meta data is being requested.
 *
 * @note `__attribute__((packed))` so that the conversion into a byte array is possible and padding is ignored.
 */
typedef struct __attribute__((packed))
{
    dynI2C_header_t header;
    uint8_t data_key; // Key whose meta data is being requested
    uint8_t crc_checksum;

} dynI2C_getmeta_packet_t;


/**
 * @brief Struct that represents a METAMSG packet. `meta` is the packet body and contains the metadata requested.
 *
 * @note `__attribute__((packed))` so that the conversion into a byte array is possible and padding is ignored.
 */
typedef struct __attribute__((packed))
{
    dynI2C_header_t header;
    dynI2C_meta_t meta;
    uint8_t crc_checksum;

} dynI2C_metamsg_packet_t;


/**
 * @brief Struct that represents a GETDATA packet. `data_key` is the register key whose data is being requested.
 *
 * @note `__attribute__((packed))` so that the conversion into a byte array is possible and padding is ignored.
 */
typedef struct __attribute__((packed))
{
    dynI2C_header_t header;
    uint8_t data_key; // Key whose data is being requested
    uint8_t crc_checksum;

} dynI2C_getdata_packet_t;


/**
 * @brief Struct that represents a SETDATA packet. `data_key` is the register key whose data is being sent.
 *
 * @note `__attribute__((packed))` so that the conversion into a byte array is possible and padding is ignored.
 */
typedef struct __attribute__((packed)) //  Structure that represents a GETDATA DynI2C packet.
{
    dynI2C_header_t header;
    uint8_t data_key;       // Key whose data is being send
    uint16_t data_len;     // Contained data
    uint8_t* data;     // Contained data
    uint8_t crc_checksum;

} dynI2C_setdata_packet_t;


#endif // DYNI2C_PACKETS_H