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

#define I2C_MASTER_TX_BUF_DISABLE 0 /* I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /* I2C master doesn't need buffer */

// Default values for DYNI2C_master_cfg_t
#define DEFAULT_DYNI2C_PORT I2C_NUM_0
#define DEFAULT_DYNI2C_SDA_IO GPIO_NUM_6
#define DEFAULT_DYNI2C_SCL_IO GPIO_NUM_7
#define DEFAULT_DYNI2C_PORT I2C_NUM_0
#define DEFAULT_DYNI2C_CLK_SRC I2C_CLK_SRC_DEFAULT
#define DEFAULT_DYNI2C_GLITCH_IGNORE 7
#define DEFAULT_DYNI2C_INTERNAL_PULLUP true
#define DEFAULT_DYNI2C_INTR_PRIORITY 0
#define DEFAULT_DYNI2C_TRANS_QUEUE_DEPTH 2

#define DEFAULT_DYNI2C_SCL_SPEED_HZ 400000
#define DEFAULT_DYNI2C_TIMEOUT_MS 1000

/// @brief Represents configuration parameters for a DynI2CMaster class.
typedef struct
{
    // I2C master bus
    i2c_port_t i2c_port;
    gpio_num_t sda_gpio_num;
    gpio_num_t scl_gpio_num;
    bool use_internal_pullup;
    soc_periph_i2c_clk_src_t clk_source;
    uint32_t intr_priority;
    size_t trans_queue_depth;
    uint8_t glitch_ignore_cnt;

    // I2C master device
    uint32_t scl_speed_hz;
    uint32_t timeout_ms;
} dynI2C_master_cfg_t;

/// @brief Creates the a dynI2C_master_cfg with default values set.
/// @return A struct of type dynI2C_master_cfg_t.
dynI2C_master_cfg_t default_dynI2C_master_cfg()
{
    dynI2C_master_cfg_t config;
    config.i2c_port = DEFAULT_DYNI2C_PORT;
    config.sda_gpio_num = DEFAULT_DYNI2C_SDA_IO;
    config.scl_gpio_num = DEFAULT_DYNI2C_SCL_IO;
    config.clk_source = DEFAULT_DYNI2C_CLK_SRC;
    config.scl_speed_hz = DEFAULT_DYNI2C_SCL_SPEED_HZ;
    config.timeout_ms = DEFAULT_DYNI2C_TIMEOUT_MS;
    config.glitch_ignore_cnt = DEFAULT_DYNI2C_GLITCH_IGNORE;
    config.use_internal_pullup = DEFAULT_DYNI2C_INTERNAL_PULLUP;
    config.intr_priority = DEFAULT_DYNI2C_INTR_PRIORITY;
    config.trans_queue_depth = DEFAULT_DYNI2C_TRANS_QUEUE_DEPTH;
}

/// @brief Represents a map entry for a DynI2C Client of this DynI2CMaster
typedef struct
{
    i2c_master_dev_handle_t device_handle;
    std::map<uint8_t, dynI2C_meta_t> metadata;
} dynI2C_client_entry_t;

class DynI2CMaster
{
public:
    ~DynI2CMaster();
    void init(dynI2C_master_cfg_t config);
    void deinit();
    esp_err_t get_meta(uint8_t address, uint8_t key, dynI2C_meta_t& metadata);
    esp_err_t get_data(uint8_t address, uint8_t key, std::vector<uint8_t>& data);
    esp_err_t get_data(uint8_t address, uint8_t key, dynI2C_meta_t metadata, std::vector<uint8_t>& data);
    esp_err_t register_client(uint8_t address);

private:
    bool initialized;
    dynI2C_master_cfg_t config;
    i2c_master_bus_handle_t i2c_bus_handle;
    std::map<uint8_t, dynI2C_client_entry_t> client_entries;
    esp_err_t register_i2c_device(uint8_t address, i2c_master_dev_handle_t* device_handle);
    esp_err_t transceive_metadata_with_client(i2c_master_dev_handle_t device_handle, uint8_t key, dynI2C_meta_t& metadata);
    esp_err_t transceive_data_with_client(i2c_master_dev_handle_t device_handle, uint8_t key, dynI2C_meta_t& metadata, std::vector<uint8_t>& data);
};

#endif // DYNI2C_MASTER_H