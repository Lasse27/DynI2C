#include "DynI2CMaster.h"

static const char* TAG = "DynI2CMaster";

/**
 * @brief Destructor for the `DynI2CMaster` class. Calls the `deinit` method.
 */
DynI2CMaster::~DynI2CMaster()
{
    deinit();
}

/**
 * @brief Initialization method for the `DynI2CMaster` class. Must be called, before using any of the
 * other methods provided by this class. Initializes the i2c master bus based on the passed configuration
 * and saves it in the class instance. This method fails with ESP_ERROR_CHECK if the i2c master bus can not be initialized.
 *
 * @param config   `dynI2C_master_cfg_t` that holds the configuration to apply to the `DynI2CMaster`.
 */
void DynI2CMaster::init(const dynI2C_master_cfg_t& config)
{
    if (initialized)
        deinit();

    i2c_master_bus_config_t bus_config = {
        .i2c_port = config.i2c_port,
        .sda_io_num = config.sda_gpio_num,
        .scl_io_num = config.scl_gpio_num,
        .clk_source = config.clk_source,
        .glitch_ignore_cnt = config.glitch_ignore_cnt,
        .intr_priority = config.intr_priority,
        .trans_queue_depth = config.trans_queue_depth,
        .flags = {
            .enable_internal_pullup = config.use_internal_pullup,
        } };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus_handle));
    ESP_LOGD(TAG, "I2C master bus successfully initialized.");

    // Save used config in instance.
    this->config = config;
    this->initialized = true;
}

/**
 * @brief De-Initialization method for the `DynI2CMaster` class. Resets the class instance to its un-initialized state.
 * Removes all cached I2C clients from the I2C master bus and clears their references and entries. Deletes the master bus
 * and clears the master bus reference in the instance. Does nothing if `init` wasn't called beforehand.
 */
void DynI2CMaster::deinit()
{
    if (!initialized)
        return;

    // Remove all currently connected device handles from I2C master bus
    for (auto& entry : client_entries)
    {
        esp_err_t err = i2c_master_bus_rm_device(entry.second.device_handle);
        if (err != ESP_OK)
            ESP_LOGW(TAG, "Failed to remove I2C device: %s", esp_err_to_name(err));
    }

    // Clear references to device handles in client_entries;
    client_entries.clear();

    // Delete bus and clear reference.
    ESP_ERROR_CHECK(i2c_del_master_bus(i2c_bus_handle));
    i2c_bus_handle = nullptr;

    // Clear config
    this->config = {};
    this->initialized = false;
    ESP_LOGD(TAG, "I2C master bus successfully deinitialized.");
}

/* =========================================================
 * The following function act as the foundatation for all other methods
 * ========================================================= */

esp_err_t DynI2CMaster::_register_client(uint8_t address)
{
    // Check if the client has not been accessed before
    auto it = client_entries.find(address);
    if (it == client_entries.end())
    {
        // If not create client device_handle and sub_map for new client.
        i2c_master_dev_handle_t device_handle = nullptr;
        esp_err_t error = _register_i2c_device(address, &device_handle);
        if (error != ESP_OK)
            return error;

        dynI2C_client_entry_t new_entry = {
            .device_handle = device_handle,
            .metadata = std::map<uint8_t, dynI2C_meta_t>(),
        };
        client_entries.insert(std::make_pair(address, new_entry));
        ESP_LOGD(TAG, "Added new client entry to DynI2C-Master.");
    }
    return ESP_OK;
}


esp_err_t DynI2CMaster::_register_i2c_device(uint8_t address, i2c_master_dev_handle_t* device_handle)
{
    // Create device config
    i2c_device_config_t device_config =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = config.scl_speed_hz,
        .scl_wait_us = config.timeout_ms
    };

    // Add i2c device to bus
    esp_err_t error = i2c_master_bus_add_device(i2c_bus_handle, &device_config, device_handle);
    if (error != ESP_OK)
        ESP_LOGE(TAG, "%s occured while add device to master bus", esp_err_to_name(error));

    return error;
}


esp_err_t DynI2CMaster::_transmit_getmeta_to_client(const dynI2C_client_entry_t& client, uint8_t key)
{
    dynI2C_getmeta_packet_t packet = build_getmeta_packet(key); // Build packet
    const uint8_t* transmit_buffer = reinterpret_cast<const uint8_t*>(&packet);

    return i2c_master_transmit(
        client.device_handle,
        transmit_buffer,
        sizeof(packet),
        config.timeout_ms
    );
}

esp_err_t DynI2CMaster::_receive_metadata_from_client(const dynI2C_client_entry_t& client, dynI2C_metadata_t* metadata)
{
    dynI2C_metamsg_packet_t packet = {};
    uint8_t* response_buffer = reinterpret_cast<uint8_t*>(&packet);

    // Receive status response from client
    esp_err_t error = i2c_master_receive(
        client.device_handle,
        response_buffer,
        sizeof(packet),
        config.timeout_ms
    );
    // Check error code from driver
    if (error != ESP_OK)
        return error;

    // Check packet for valid bytes
    if (packet.magic != DYNI2C_MAGIC_NUMBER)
    {
        ESP_LOGE(TAG, "Invalid magic number in METAMSG, trying again...");
        return ESP_ERR_INVALID_CRC;
    }
    uint8_t expected_crc = calculate_crc8(packet);
    if (packet.crc_checksum != expected_crc)
    {
        ESP_LOGE(TAG, "CRC mismatch, trying again...");
        return ESP_ERR_INVALID_CRC;
    }
    if (packet.packet_type != DYNI2C_PACKET_TYPE_METAMSG)
    {
        ESP_LOGE(TAG, "Invalid packet type, expected METAMSG, got %d", packet.packet_type);
        return ESP_ERR_INVALID_RESPONSE;
    }

    metadata->data_flags = packet.data_flags;
    metadata->data_len = packet.data_len;
    return ESP_OK;
}


esp_err_t DynI2CMaster::_get_metadata_from_client(
    const dynI2C_client_entry_t& client,
    uint8_t data_key,
    dynI2C_metadata_t* metadata,
    uint8_t max_retries = 3,
    uint8_t delay_ms = 3)
{
    esp_err_t error = ESP_FAIL;
    for (uint8_t i = 0; i < max_retries; i++)
    {
        // Transmit request to client with client-handle from paramters
        error = _transmit_getmeta_to_client(client, data_key);
        if (error != ESP_OK)
        {
            ESP_LOGW(TAG, "Failed to transmit GETMETA packet to client (attempt %d/%d): %s", i + 1, max_retries, esp_err_to_name(error));
            continue;
        }

        // If request was transmitted, wait a tiny bit
        vTaskDelay(pdMS_TO_TICKS(delay_ms));

        // Receive response and write it into data vector reference
        error = _receive_metadata_from_client(client, metadata);
        if (error != ESP_OK)
        {
            ESP_LOGW(TAG, "Failed to receive DATAMSG packet from client (attempt %d/%d): %s", i + 1, max_retries, esp_err_to_name(error));
            continue;
        }
        return ESP_OK;
    }
    return error;
}


esp_err_t DynI2CMaster::_transmit_getdata_to_client(const dynI2C_client_entry_t& client, uint8_t key)
{
    dynI2C_getmeta_packet_t packet = build_getdata_packet(key); // Build packet
    const uint8_t* transmit_buffer = reinterpret_cast<const uint8_t*>(&packet);

    return i2c_master_transmit(
        client.device_handle,
        transmit_buffer,
        sizeof(packet),
        config.timeout_ms
    );
}


esp_err_t DynI2CMaster::_receive_data_from_client(const dynI2C_client_entry_t& client, uint16_t data_len, std::vector<uint8_t>& data)
{
    size_t response_buffer_size = sizeof(uint8_t) * 3 + data_len;
    uint8_t response_buffer[response_buffer_size];

    // Receive status response from client
    esp_err_t error = i2c_master_receive(
        client.device_handle,
        response_buffer,
        response_buffer_size,
        config.timeout_ms
    );
    // Check error code from driver
    if (error != ESP_OK)
        return error;

    // Check packet for valid bytes
    if (response_buffer[0] != DYNI2C_MAGIC_NUMBER)
    {
        ESP_LOGE(TAG, "Invalid magic number in DATAMSG, trying again...");
        return ESP_ERR_INVALID_CRC;
    }
    uint8_t expected_crc = calculate_crc8(response_buffer, response_buffer_size - 1);
    if (response_buffer[response_buffer_size - 1] != expected_crc)
    {
        ESP_LOGE(TAG, "CRC mismatch, trying again...");
        return ESP_ERR_INVALID_CRC;
    }
    if (response_buffer[1] != DYNI2C_PACKET_TYPE_DATAMSG)
    {
        ESP_LOGE(TAG, "Invalid packet type, expected DATAMSG, got %d", response_buffer[1]);
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Resize vector that will hold the function output
    data.resize(data_len);

    // Return data content from buffer
    memcpy(data.data(), &response_buffer[2], data_len);
    return ESP_OK;
}


esp_err_t DynI2CMaster::_get_data_from_client(
    const dynI2C_client_entry_t& client,
    uint8_t data_key,
    uint16_t data_len,
    std::vector<uint8_t>& data,
    uint8_t max_retries = 3,
    uint8_t delay_ms = 3)
{
    esp_err_t error = ESP_FAIL;
    for (uint8_t i = 0; i < max_retries; i++)
    {
        // Transmit request to client with client-handle from paramters
        error = _transmit_getdata_to_client(client, data_key);
        if (error != ESP_OK)
        {
            ESP_LOGW(TAG, "Failed to transmit GETDATA packet to client (attempt %d/%d): %s", i + 1, max_retries, esp_err_to_name(error));
            continue;
        }

        // If request was transmitted, wait a tiny bit
        vTaskDelay(pdMS_TO_TICKS(delay_ms));

        // Receive response and write it into data vector reference
        error = _receive_data_from_client(client, data_len, data);
        if (error != ESP_OK)
        {
            ESP_LOGW(TAG, "Failed to receive DATAMSG packet from client (attempt %d/%d): %s", i + 1, max_retries, esp_err_to_name(error));
            continue;
        }
        return ESP_OK;
    }
    return error;
}


/* =========================================================
 * The following function act as the intermediate functions for the exported functions
 * ========================================================= */

esp_err_t DynI2CMaster::_get_client_id(const dynI2C_client_entry_t& client, uint8_t* id)
{
    if (id == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    std::vector<uint8_t> register_data;
    esp_err_t error = _get_data_from_client(client, DYNI2C_CLIENT_REGISTER_ID, 1, register_data);
    if (error != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to get id from client: %s", esp_err_to_name(error));
        return error;
    }
    *id = register_data[0];
    return ESP_OK;
}


esp_err_t DynI2CMaster::_get_client_error(const dynI2C_client_entry_t& client, uint8_t* error_code)
{
    if (error_code == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    std::vector<uint8_t> register_data;
    esp_err_t error = _get_data_from_client(client, DYNI2C_CLIENT_REGISTER_ERROR, 1, register_data);
    if (error != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to get error from client: %s", esp_err_to_name(error));
        return error;
    }
    *error_code = register_data[0];
    return ESP_OK;
}


esp_err_t DynI2CMaster::_get_client_status(const dynI2C_client_entry_t& client, uint8_t* status)
{
    if (status == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    std::vector<uint8_t> register_data;
    esp_err_t error = _get_data_from_client(client, DYNI2C_CLIENT_REGISTER_STATUS, 1, register_data);
    if (error != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to get status from client: %s", esp_err_to_name(error));
        return error;
    }
    *status = register_data[0];
    return ESP_OK;
}


esp_err_t DynI2CMaster::_get_client_register_count(const dynI2C_client_entry_t& client, uint8_t* count)
{
    if (count == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    std::vector<uint8_t> register_data;
    esp_err_t error = _get_data_from_client(client, DYNI2C_CLIENT_REGISTER_COUNT, 1, register_data);
    if (error != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to get register count from client: %s", esp_err_to_name(error));
        return error;
    }
    *count = register_data[0];
    return ESP_OK;
}


esp_err_t DynI2CMaster::_get_client_version(const dynI2C_client_entry_t& client, uint8_t* version)
{
    if (version == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    std::vector<uint8_t> register_data;
    esp_err_t error = _get_data_from_client(client, DYNI2C_CLIENT_REGISTER_VERSION, 3, register_data);
    if (error != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to get version from client: %s", esp_err_to_name(error));
        return error;
    }
    version[0] = register_data[0];
    version[1] = register_data[1];
    version[2] = register_data[2];
    return ESP_OK;
}


esp_err_t DynI2CMaster::_get_client_boot_id(const dynI2C_client_entry_t& client, uint8_t* boot_id)
{
    if (boot_id == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    std::vector<uint8_t> register_data;
    esp_err_t error = _get_data_from_client(client, DYNI2C_CLIENT_REGISTER_BOOTID, 2, register_data);
    if (error != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to get version from client: %s", esp_err_to_name(error));
        return error;
    }
    boot_id[0] = register_data[0];
    boot_id[1] = register_data[1];
    return ESP_OK;
}

esp_err_t DynI2CMaster::_preflight(uint8_t address)
{
    // If instance was not initialized we show exception and fail.
    if (!initialized)
    {
        ESP_LOGE(TAG, "Failed to request data. DynI2C-Master was not intialized.");
        return ESP_ERR_INVALID_STATE;
    }

    // Register client (does only a check if already registered)
    esp_err_t error = _register_client(address);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register DynI2C-Client.");
        return error;
    }
    return ESP_OK;
}


/* Top level functions called by the user
 * ========================================================= */

esp_err_t DynI2CMaster::get_error(uint8_t address, uint8_t* error_code)
{
    // Common checks for data access
    esp_err_t error = _preflight(address);
    if (error != ESP_OK)
        return error;

    // Get client entry and call intermediate method
    dynI2C_client_entry_t& client_entry = client_entries.at(address);
    return _get_client_error(client_entry, error_code);
}


esp_err_t DynI2CMaster::get_status(uint8_t address, uint8_t* status)
{
    // Common checks for data access
    esp_err_t error = _preflight(address);
    if (error != ESP_OK)
        return error;

    // Get client entry and call intermediate method
    dynI2C_client_entry_t& client_entry = client_entries.at(address);
    return _get_client_status(client_entry, status);
}


esp_err_t DynI2CMaster::get_register_count(uint8_t address, uint8_t* register_count)
{
    // Common checks for data access
    esp_err_t error = _preflight(address);
    if (error != ESP_OK)
        return error;

    // Get client entry and call intermediate method
    dynI2C_client_entry_t& client_entry = client_entries.at(address);
    return _get_client_register_count(client_entry, register_count);
}


esp_err_t DynI2CMaster::get_id(uint8_t address, uint8_t* device_id)
{
    // Common checks for data access
    esp_err_t error = _preflight(address);
    if (error != ESP_OK)
        return error;

    // Get client entry and call intermediate method
    dynI2C_client_entry_t& client_entry = client_entries.at(address);
    return _get_client_id(client_entry, device_id);
}


esp_err_t DynI2CMaster::get_boot_id(uint8_t address, uint16_t* boot_id)
{
    // Common checks for data access
    esp_err_t error = _preflight(address);
    if (error != ESP_OK)
        return error;

    // Get client entry and call intermediate method
    dynI2C_client_entry_t& client_entry = client_entries.at(address);
    uint8_t bytes[2];
    error = _get_client_boot_id(client_entry, bytes);
    if (error != ESP_OK)
        return error;

    // Write bytes into uint16
    *boot_id = (bytes[0] << 8) | bytes[1];
    return ESP_OK;
}


esp_err_t DynI2CMaster::get_version(uint8_t address, uint8_t* major, uint8_t* minor, uint8_t* patch)
{
    // Common checks for data access
    esp_err_t error = _preflight(address);
    if (error != ESP_OK)
        return error;

    // Get client entry and call intermediate method
    dynI2C_client_entry_t& client_entry = client_entries.at(address);
    uint8_t bytes[3];
    error = _get_client_version(client_entry, bytes);
    if (error != ESP_OK)
        return error;

    // Write bytes into uint16
    *major = bytes[0];
    *minor = bytes[1];
    *patch = bytes[2];
    return ESP_OK;
}

esp_err_t DynI2CMaster::get_metadata(uint8_t address, uint8_t key, dynI2C_metadata_t* metadata)
{
    if (metadata == nullptr)
        return ESP_ERR_INVALID_ARG;

    // Common checks for data access
    esp_err_t error = _preflight(address);
    if (error != ESP_OK)
        return error;

    // Get entry and check if there is metadata in the entry for the wanted key
    dynI2C_client_entry_t& client_entry = client_entries.at(address);
    auto meta_it = client_entry.metadata.find(key);
    if (meta_it != client_entry.metadata.end())
    {
        // Key was found so just return the cached metadata
        *metadata = meta_it->second;
        return ESP_OK;
    }

    ESP_LOGD(TAG, "Metadata not cached, accessing client...");
    error = _get_metadata_from_client(client_entry, key, metadata);
    if (error != ESP_OK)
        return error;

    // Only if the dynamic size flag is not set, we cache the response metadata
    if (!(metadata->data_flags & DYNAMIC_FLAG))
        client_entry.metadata[key] = *metadata;

    return ESP_OK;
}