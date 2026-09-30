/// @file DynI2CMaster.h
/// @brief Definitions and type as header file for DynI2CMaster.cpp
///
/// @author Lasse-Leander Hillen
/// @date 2026-09-22
/// @version 1.0.0

#ifndef DYNI2C_MASTER_H
#define DYNI2C_MASTER_H

#include <esp_err.h>
#include <map>
#include <vector>
#include "DynI2CCommon.h"
#include "DynI2CClient.h"
#include "freertos/FreeRTOS.h"


// Default values for DYNI2C_master_cfg_t
#define DYNI2C_MASTER_DEFAULT_PORT              I2C_NUM_0
#define DYNI2C_MASTER_DEFAULT_SDA_GPIO          GPIO_NUM_6
#define DYNI2C_MASTER_DEFAULT_SCL_GPIO          GPIO_NUM_7
#define DYNI2C_MASTER_DEFAULT_CLK_SRC           I2C_CLK_SRC_DEFAULT
#define DYNI2C_MASTER_DEFAULT_GLITCH_IGNORE     7
#define DYNI2C_MASTER_DEFAULT_INTERNAL_PULLUP   true
#define DYNI2C_MASTER_DEFAULT_INTR_PRIORITY     0
#define DYNI2C_MASTER_DEFAULT_TRANS_QUEUE_DEPTH 2
#define DYNI2C_MASTER_DEFAULT_SCL_SPEED_HZ      400000
#define DYNI2C_MASTER_DEFAULT_TIMEOUT_MS        1000


/// @brief Represents configuration parameters for a DynI2CMaster class.
typedef struct
{
    uint8_t i2c_port;
    gpio_num_t sda_gpio_num;
    i2c_clock_source_t clk_source;
    int32_t intr_priority;
    uint32_t trans_queue_depth;
    uint8_t glitch_ignore_cnt;
    uint32_t scl_speed_hz;
    uint32_t timeout_ms;
    gpio_num_t scl_gpio_num;
    bool use_internal_pullup;
    bool allow_power_down;

} dynI2C_master_cfg_t;


/// @brief Creates the a dynI2C_master_cfg with default values set.
/// @return A struct of type dynI2C_master_cfg_t.
dynI2C_master_cfg_t dynI2C_default_master_cfg()
{
    dynI2C_master_cfg_t config;
    config.i2c_port = DYNI2C_MASTER_DEFAULT_PORT;
    config.sda_gpio_num = DYNI2C_MASTER_DEFAULT_SDA_GPIO;
    config.scl_gpio_num = DYNI2C_MASTER_DEFAULT_SCL_GPIO;
    config.clk_source = DYNI2C_MASTER_DEFAULT_CLK_SRC;
    config.scl_speed_hz = DYNI2C_MASTER_DEFAULT_SCL_SPEED_HZ;
    config.timeout_ms = DYNI2C_MASTER_DEFAULT_TIMEOUT_MS;
    config.glitch_ignore_cnt = DYNI2C_MASTER_DEFAULT_GLITCH_IGNORE;
    config.use_internal_pullup = DYNI2C_MASTER_DEFAULT_INTERNAL_PULLUP;
    config.intr_priority = DYNI2C_MASTER_DEFAULT_INTR_PRIORITY;
    config.trans_queue_depth = DYNI2C_MASTER_DEFAULT_TRANS_QUEUE_DEPTH;
    return config;
}


/// @brief Represents a map entry for a DynI2C Client of this DynI2CMaster
typedef struct
{
    i2c_master_dev_handle_t device_handle;
    std::map<uint8_t, dynI2C_metadata_t> metadata;

} dynI2C_client_entry_t;


class DynI2CMaster
{
public:
    DynI2CMaster() = default;
    ~DynI2CMaster() { deinit(); }
    DynI2CMaster(const DynI2CMaster&) = delete; // Prohibit copy
    DynI2CMaster& operator=(const DynI2CMaster&) = delete; // Prohibit copy

    void init(const dynI2C_master_cfg_t& config);
    void deinit();

    esp_err_t get_id(uint8_t address, uint8_t* device_id);
    esp_err_t get_error(uint8_t address, uint8_t* error_code);
    esp_err_t get_status(uint8_t address, uint8_t* status);
    esp_err_t get_boot_id(uint8_t address, uint16_t* boot_id);
    esp_err_t get_version(uint8_t address, uint8_t* major, uint8_t* minor, uint8_t* patch);
    esp_err_t get_register_count(uint8_t address, uint8_t* register_count);
    esp_err_t get_metadata(uint8_t address, uint8_t key, dynI2C_metadata_t* metadata);
    esp_err_t get_data(uint8_t address, uint8_t key, dynI2C_metadata_t* metadata, std::vector<uint8_t>& data);
    esp_err_t set_data(uint8_t address, uint8_t key, std::vector<uint8_t>& data);

private:
    bool initialized;
    dynI2C_master_cfg_t config;
    i2c_master_bus_handle_t i2c_bus_handle;
    std::map<uint8_t, dynI2C_client_entry_t> client_entries;

    esp_err_t _register_client(uint8_t address);
    esp_err_t _register_i2c_device(uint8_t address, i2c_master_dev_handle_t* device_handle);

    esp_err_t _transmit_getmeta_to_client(const dynI2C_client_entry_t& client, uint8_t key);
    esp_err_t _receive_metadata_from_client(const dynI2C_client_entry_t& client, dynI2C_metadata_t* metadata);
    esp_err_t _get_metadata_from_client(const dynI2C_client_entry_t& client, uint8_t data_key, dynI2C_metadata_t* metadata, uint8_t max_retries, uint8_t delay_ms);

    esp_err_t _transmit_getdata_to_client(const dynI2C_client_entry_t& client, uint8_t key);
    esp_err_t _receive_data_from_client(const dynI2C_client_entry_t& client, uint16_t data_len, std::vector<uint8_t>& data);
    esp_err_t _get_data_from_client(const dynI2C_client_entry_t& client, uint8_t data_key, uint16_t data_len, std::vector<uint8_t>& data, uint8_t max_retries, uint8_t delay_ms);

    esp_err_t _transmit_setdata_to_client(const dynI2C_client_entry_t& client, uint8_t key, uint16_t data_len);
    esp_err_t _transmit_datamsg_to_client(const dynI2C_client_entry_t& client, uint8_t key, uint8_t* data, uint16_t data_len);
    esp_err_t _set_data_on_client(const dynI2C_client_entry_t& client, uint8_t key, dynI2C_metadata_t* metadata, std::vector<uint8_t>& data, uint8_t max_retries, uint8_t delay_ms);

    esp_err_t _preflight(uint8_t address);
};

#endif // DYNI2C_MASTER_H