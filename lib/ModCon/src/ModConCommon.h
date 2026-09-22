/**
 * @file ModConCommon.h
 * @brief Common definitions and type as header for different files of the project.
 *
 * @author Lasse-Leander Hillen
 * @date 2026-09-22
 * @version 1.0.0
 */

#ifndef MODCON_COMMON_H
#define MODCON_COMMON_H

#include <cstdint>

#define MODCON_MAGIC_NUMBER 0xC4
#define MODCON_MAX_DATA_LEN 2048

typedef enum : uint8_t // Enum that represents the different possible packet types.
{
    METAREQ = 0x00,
    METARES = 0x01,
    DATAREQ = 0x02,
    DATARES = 0x03,
} packet_type_t;

typedef enum : uint8_t
{
    DYNAMIC_SIZE = 1 << 0
} data_flag_t;

typedef struct // Structure that represents a ModCon Header packet.
{
    uint8_t magic = MODCON_MAGIC_NUMBER;
    uint8_t receiver_id;
    uint8_t sender_id;
    packet_type_t packet_type;
    uint8_t session_id;
} modcon_header_t;

typedef struct // Structure that represents a METAREQ ModCon packet.
{
    modcon_header_t header; // Header of the package
    uint8_t data_key;       // Key whose meta data is being requested
    uint8_t crc_checksum;   // Footer of the package
} modcon_metareq_t;

typedef struct // Structure that represents a METARES ModCon packet.
{
    modcon_header_t header; // Header of the package
    uint8_t data_flags;     // Bit flags of the data field.
    uint16_t data_len;      // Length of the contained data
    uint8_t crc_checksum;   // Footer of the package
} modcon_metares_t;

typedef struct //  Structure that represents a DATAREQ ModCon packet.
{
    modcon_header_t header; // Header of the package
    uint8_t data_key;       // Key whose data is being requested
    uint8_t crc_checksum;   // Footer of the package
} modcon_datareq_t;

typedef struct // Structure that represents a DATARES ModCon packet.
{
    modcon_header_t header;            // Header of the package
    uint8_t data_len;                  // Body of the package
    uint8_t data[MODCON_MAX_DATA_LEN]; // Body of the package
    uint8_t crc_checksum;              // Footer of the package
} modcon_datares_t;

#endif // MODCON_COMMON_H