#include "DynI2CClient.h"
#include <cstring>

static const char* TAG = "DynI2CClient";

/**
 * @brief Initialization method for the `DynI2CClient` class. Must be called, before using any of the
 * other methods provided by this class. Initializes the i2c client bus and registers.
 *
 * @param config   `dyni2c_client_cfg_t` that holds the configuration to apply to the `DynI2CClient`.
 */
void DynI2CClient::init(dyni2c_client_cfg_t config)
{
    if (initialized)
        deinit();

    ESP_ERROR_CHECK(_add_default_registers());
    initialized = true;
}


/**
 * @brief De-Initialization method for the `DynI2CClient` class. Resets the class instance to its un-initialized state.
 */
void DynI2CClient::deinit()
{
    if (!initialized)
        return;

    // Free all allocated space in the registers
    for (auto& reg : registers)
    {
        delete[] reg.data;
        reg = {};
    }
    initialized = false;
}


/**
 * @brief Adds a register to the internal registers of the DynI2C-Client. Checks wether the key/index is
 * valid. Checks wether the passed data_len and data reference are valid. Checks wether a register for the
 * passed key already exists.
 *
 * @param key The index/key the data is going to be added to.
 * @param data_flags Data flags for the new register.
 * @param data_len The length of the data in the new register.
 * @param data Data pointer to the data of the new register.
 * @return ESP_OK if the execution was successfull, otherwise another `esp_err_t` code.
 */
esp_err_t DynI2CClient::add_register(uint8_t key, uint8_t data_flags, uint16_t data_len)
{
    // Check wether the provided key is in an allowed area
    if (key < DYNI2C_CLIENT_FIRST_USER_REG || key > DYNI2C_CLIENT_MAX_REGISTERS)
        return ESP_ERR_INVALID_ARG;

    // Call internal function
    return _add_register(key, data_flags, data_len);
}


esp_err_t DynI2CClient::set_register_data(uint8_t key, uint16_t data_len, uint8_t* data)
{
    // Check wether provided data params are good to go
    if (data == nullptr || data_len == 0)
        return ESP_ERR_INVALID_ARG;

    // Get register reference from internal register list
    dyni2c_client_reg_t& reg = registers[key];

    // Check wether the register is valid or unknown.
    if (reg.data == nullptr)
        return ESP_ERR_NOT_FOUND;

    // Check wether the data_len is the same
    if (data_len != reg.data_len)
    {
        if (!(reg.data_flags & (DYNAMIC_FLAG)))
        {
            return ESP_ERR_INVALID_ARG;
        }

        // Free old space
        delete[] reg.data;
        reg.data = nullptr;

        // Create new space
        reg.data = new uint8_t[data_len];
        reg.data_len = data_len;
    }

    memcpy(reg.data, data, data_len);
    return ESP_OK;
}


esp_err_t DynI2CClient::get_register_meta(uint8_t key, uint8_t& data_flags, uint16_t& data_len) const
{
    const dyni2c_client_reg_t& reg = registers[key];
    if (reg.data == nullptr)
        return ESP_ERR_NOT_FOUND;

    data_flags = reg.data_flags;
    data_len = reg.data_len;
    return ESP_OK;
}


esp_err_t DynI2CClient::get_register_data(uint8_t key, uint16_t buffer_size, uint8_t* buffer, uint16_t& buffer_len) const
{
    if (buffer == nullptr)
        return ESP_ERR_INVALID_ARG;

    const dyni2c_client_reg_t& reg = registers[key];
    if (reg.data == nullptr)
        return ESP_ERR_NOT_FOUND;

    if (buffer_size < reg.data_len)
        return ESP_ERR_INVALID_SIZE;

    memcpy(buffer, reg.data, reg.data_len);
    buffer_len = reg.data_len;
    return ESP_OK;
}


/**
 * @brief Adds a register to the internal registers of the DynI2C-Client. Checks wether the passed data_len
 * and data reference are valid. Checks wether a register for the passed key already exists.
 *
 * @param key The index/key the data is going to be added to.
 * @param data_flags Data flags for the new register.
 * @param data_len The length of the data in the new register.
 * @return ESP_OK if the execution was successfull, otherwise another `esp_err_t` code.
 */
esp_err_t DynI2CClient::_add_register(uint8_t key, uint8_t data_flags, uint16_t data_len)
{
    // Check wether data_len makes sense
    if (data_len == 0)
        return ESP_ERR_INVALID_ARG;

    // Check wether field was already added
    if (registers[key].data != nullptr)
        return ESP_ERR_INVALID_STATE;

    // Add the key and the params
    registers[key] = {
        .data_flags = data_flags,
        .data_len = data_len,
        .data = new uint8_t[data_len],
    };
    return ESP_OK;
}


/**
 * @brief Adds the default registers, that every DynI2C-Client has to the internal registers.
 * @return ESP_OK if the execution was successfull, otherwise another `esp_err_t` code.
 */
esp_err_t DynI2CClient::_add_default_registers()
{
    // Device ID - Register
    ESP_RETURN_ON_ERROR(_add_register(
        DYNI2C_CLIENT_REGISTER_ID,
        READABLE_FLAG | TYPE_UINT_FLAG,
        sizeof(uint8_t)
    ), TAG, "Failed to add 'Device ID' Register.");

    // Version - Register
    ESP_RETURN_ON_ERROR(_add_register(
        DYNI2C_CLIENT_REGISTER_VERSION,
        READABLE_FLAG | TYPE_UINT_FLAG,
        sizeof(uint16_t)
    ), TAG, "Failed to add 'Version' Register.");

    // BootID - Register
    ESP_RETURN_ON_ERROR(_add_register(
        DYNI2C_CLIENT_REGISTER_BOOTID,
        READABLE_FLAG | TYPE_UINT_FLAG,
        sizeof(uint16_t)
    ), TAG, "Failed to add 'Boot-ID' Register.");

    // Status - Register
    ESP_RETURN_ON_ERROR(_add_register(
        DYNI2C_CLIENT_REGISTER_STATUS,
        READABLE_FLAG | TYPE_UINT_FLAG,
        sizeof(uint8_t)
    ), TAG, "Failed to add 'Status' Register.");

    // Error - Register
    ESP_RETURN_ON_ERROR(_add_register(
        DYNI2C_CLIENT_REGISTER_ERROR,
        READABLE_FLAG | TYPE_UINT_FLAG,
        sizeof(uint8_t)
    ), TAG, "Failed to add 'Error' Register.");

    // Count - Register
    ESP_RETURN_ON_ERROR(_add_register(
        DYNI2C_CLIENT_REGISTER_COUNT,
        READABLE_FLAG | TYPE_UINT_FLAG,
        sizeof(uint8_t)
    ), TAG, "Failed to add 'Count' Register.");

    return ESP_OK;
}
