/**
 * @file DynI2CClient.h
 * @brief Common definitions and type as header for `DynI2CClient.h`.
 *
 * @author Lasse-Leander Hillen
 * @date 2026-09-23
 * @version 1.0.0
 */

#ifndef DYNI2C_CLIENT_H
#define DYNI2C_CLIENT_H

#include <cstdint>
#include <esp_err.h>
#include "DynI2CCommon.h"

 // Common constants (these are the same for all DynI2C Clients)
#define DYNI2C_CLIENT_MAX_REGISTERS     0xFF
#define DYNI2C_CLIENT_FIRST_USER_REG    0x06

// System registers (these are the same for all DynI2C Clients)
#define DYNI2C_CLIENT_REGISTER_ID       0x00
#define DYNI2C_CLIENT_REGISTER_VERSION  0x01
#define DYNI2C_CLIENT_REGISTER_BOOTID   0x02
#define DYNI2C_CLIENT_REGISTER_STATUS   0x03
#define DYNI2C_CLIENT_REGISTER_ERROR    0x04
#define DYNI2C_CLIENT_REGISTER_COUNT    0x05

// Status codes read from status register (these are the same for all DynI2C Clients)
#define DYNI2C_CLIENT_STATUS_ERROR      0x00
#define DYNI2C_CLIENT_STATUS_IDLE       0x01
#define DYNI2C_CLIENT_STATUS_BUSY       0x02
#define DYNI2C_CLIENT_STATUS_READY      0x03

// Error codes read from status register (these are the same for all DynI2C Clients)
#define DYNI2C_CLIENT_ERROR_NONE        0x00
#define DYNI2C_CLIENT_ERROR_INVALID_CMD 0x01
#define DYNI2C_CLIENT_ERROR_INVALID_REG 0x02
#define DYNI2C_CLIENT_ERROR_TIMEOUT     0x03
#define DYNI2C_CLIENT_ERROR_INTERNAL    0x04
#define DYNI2C_CLIENT_ERROR_BUSY        0x05

/**
 * @brief Configuration structure for initialization of `DynI2CClient`.
 */
typedef struct
{

} dyni2c_client_cfg_t;

typedef struct
{
    uint8_t data_flags;
    uint16_t data_len;
    uint8_t* data;

} dyni2c_client_reg_t;

/**
 * @brief Client object of the DynI2C Interaction.
 */
class DynI2CClient
{
public:
    DynI2CClient() = default;
    ~DynI2CClient() { deinit(); }
    DynI2CClient(const DynI2CClient&) = delete; // Prohibit copy
    DynI2CClient& operator=(const DynI2CClient&) = delete; // Prohibit copy

    void init(dyni2c_client_cfg_t config);
    void deinit();

    esp_err_t add_register(uint8_t key, uint8_t data_flags, uint16_t data_len);
    esp_err_t set_register_data(uint8_t key, uint16_t data_len, uint8_t* data);
    esp_err_t get_register_meta(uint8_t key, uint8_t& data_flags, uint16_t& data_len) const;
    esp_err_t get_register_data(uint8_t key, uint16_t buffer_size, uint8_t* buffer, uint16_t& buffer_len) const;

    esp_err_t set_error(uint8_t error);
    esp_err_t set_status(uint8_t status);


private:
    bool initialized = false;
    dyni2c_client_reg_t registers[DYNI2C_CLIENT_MAX_REGISTERS] = {};

    esp_err_t _add_register(uint8_t key, uint8_t data_flags, uint16_t data_len);
    esp_err_t _add_default_registers();
};



#endif // DYNI2C_CLIENT_H