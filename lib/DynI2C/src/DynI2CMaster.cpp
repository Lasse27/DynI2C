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
void DynI2CMaster::init(dynI2C_master_cfg_t config)
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
    config = config;
    initialized = true;
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
    config = {};
    initialized = false;
    ESP_LOGD(TAG, "I2C master bus successfully deinitialized.");
}

/**
 * @brief Acquires the metadata for a specific address/client and key. Dynamically registers a I2C client if address was
 * never requested before. If specific address/client and key were requested before, only a std::map lookup is done to
 * acquire the metadata. Otherwise the metadata is requested via I2C from client and cached for later lookup.
 *
 * @param address The I2C address of the client that holds the required metadata.
 * @param key The key of the register that the metadata is required from.
 * @param metadata Contains the read metadata after successfull execution of this function.
 * @return Returns `ESP_OK` if on successfull execution, otherwise a different error code.
 */
esp_err_t DynI2CMaster::get_meta(uint8_t address, uint8_t key, dynI2C_meta_t& metadata)
{
    // If instance was not initialized we show exception and fail.
    if (!initialized)
    {
        ESP_LOGE(TAG, "Failed to request meta data. DynI2C-Master was not intialized.");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t error = register_client(address);

    // Get entry and check if there is metadata in the entry for the wanted key
    dynI2C_client_entry_t& client_entry = client_entries.at(address);
    auto meta_it = client_entry.metadata.find(key);
    if (meta_it != client_entry.metadata.end())
    {
        // Key was found so just return the cached metadata
        metadata = meta_it->second;
        return ESP_OK;
    }

    // Key was not found, so we have to request it from the client.
    esp_err_t error = transceive_metadata_with_client(client_entry.device_handle, key, metadata);
    if (error != ESP_OK)
        return error;

    // Only if the dynamic size flag is not set, we cache the response metadata
    if (!(metadata.data_flags & DYNAMIC_FLAG))
        client_entry.metadata[key] = metadata;

    return ESP_OK;
}

/**
 * @brief Acquires the data for a specific address/client and key over I2C. Calls `get_meta` to acquire the metadata for the
 * required register and therefore maybe performs additional I2C-transactions.
 *
 * @param address The I2C address of the client that holds the required data.
 * @param key The key of the register that the data is required from.
 * @param data Contains the read data after successfull execution of this function.
 * @return Returns `ESP_OK` if on successfull execution, otherwise a different error code.
 */
esp_err_t DynI2CMaster::get_data(uint8_t address, uint8_t key, std::vector<uint8_t>& data)
{
    // If instance was not initialized we show exception and fail.
    if (!initialized)
    {
        ESP_LOGE(TAG, "Failed to request meta data. DynI2C-Master was not intialized.");
        return ESP_ERR_INVALID_STATE;
    }

    // Get metadata from cache or request from client device
    dynI2C_meta_t metadata;
    esp_err_t error = get_meta(address, key, metadata);
    if (error != ESP_OK)
        return error;

    // Send request to client and get data result
    dynI2C_client_entry_t& client_entry = client_entries.at(address);
    return transceive_data_with_client(client_entry.device_handle, key, metadata, data);
}

/**
 * @brief Acquires the data for a specific address/client and key over I2C.
 *
 * @param address The I2C address of the client that holds the required data.
 * @param key The key of the register that the data is required from.
 * @param metadata Metadata for the register that the data is required from.
 * @param data Contains the read data after successfull execution of this function.
 * @return Returns `ESP_OK` if on successfull execution, otherwise a different error code.
 */
esp_err_t DynI2CMaster::get_data(uint8_t address, uint8_t key, dynI2C_meta_t metadata, std::vector<uint8_t>& data)
{
    // If instance was not initialized we show exception and fail.
    if (!initialized)
    {
        ESP_LOGE(TAG, "Failed to request meta data. DynI2C-Master was not intialized.");
        return ESP_ERR_INVALID_STATE;
    }

    // Send request to client and get data result
    register_client(address);
    dynI2C_client_entry_t& client_entry = client_entries.at(address);
    return transceive_data_with_client(client_entry.device_handle, key, metadata, data);
}

/**
 * @brief Registers a client in the client cache and creates the device handle for i2c communication. If the clients address is
 * already registered, this method does nothing.
 *
 * @param address The I2C-address of the new client that is registered.
 * @return Returns `ESP_OK` if on successfull execution, otherwise a different error code.
 */
esp_err_t DynI2CMaster::register_client(uint8_t address)
{
    // Check if the client has not been accessed before
    auto it = client_entries.find(address);
    if (it == client_entries.end())
    {
        // If not create client device_handle and sub_map for new client.
        i2c_master_dev_handle_t device_handle = nullptr;
        esp_err_t error = register_i2c_device(address, &device_handle);
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

/**
 * @brief Creates the device handle for i2c communication and adds the device to the i2c bus.
 *
 * @param address The I2C-address of the new client that is registered.
 * @return Returns `ESP_OK` if on successfull execution, otherwise a different error code.
 */
esp_err_t DynI2CMaster::register_i2c_device(uint8_t address, i2c_master_dev_handle_t* device_handle)
{
    // Create device config
    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = config.scl_speed_hz,
        .scl_wait_us = config.timeout_ms };

    // Add i2c device to bus
    esp_err_t error = i2c_master_bus_add_device(i2c_bus_handle, &device_config, device_handle);
    if (error != ESP_OK)
        ESP_LOGE(TAG, "%s occured while add device to master bus", esp_err_to_name(error));

    return error;
}

/**
 * @brief Runs an i2c interaction with the i2c client to obtain metadata information for a specified register.
 * @param device_handle The i2c device handle of the client that is spoken to.
 * @param key The key for the register whose metadata is required.
 * @param metadata Contains the read metadata after successfull execution of this function.
 * @return Returns `ESP_OK` if on successfull execution, otherwise a different error code.
 */
esp_err_t DynI2CMaster::transceive_metadata_with_client(i2c_master_dev_handle_t device_handle, uint8_t key, dynI2C_meta_t& metadata)
{
    // Build request for client device
    dynI2C_getmeta_packet_t request = build_getmeta_packet(key);

    // Build transmit and response buffers
    dynI2C_metamsg_packet_t response;
    const uint8_t* tx = reinterpret_cast<const uint8_t*>(&request);
    uint8_t* rx = reinterpret_cast<uint8_t*>(&response);

    // Send request at most 3 times to client.
    for (uint8_t attempt = 0; attempt < 3; ++attempt)
    {
        // Transceive metadata from client
        esp_err_t error = i2c_master_transmit_receive(
            device_handle,
            tx,
            sizeof(request),
            rx,
            sizeof(response),
            config.timeout_ms);

        if (error != ESP_OK)
        {
            ESP_LOGE(TAG, "Failes to transceive meta from client.");
            return error;
        }

        // Compare checksums
        if (response.crc_checksum != calculate_crc8(response))
        {
            ESP_LOGW(TAG, "Checksum mismatch, retrying!");
            continue;
        }

        metadata = response.meta;
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Continuous checksum mismatch. Reached maximum retries.");
    return ESP_ERR_INVALID_RESPONSE;
}

/**
 * @brief Runs an i2c interaction with the i2c client to obtain data for a specified register.
 * @param device_handle The i2c device handle of the client that is spoken to.
 * @param key The key for the register whose metadata is required.
 * @param metadata Metadata for the register that the data is required from.
 * @param data Contains the read data after successfull execution of this function.
 * @return Returns `ESP_OK` if on successfull execution, otherwise a different error code.
 */
esp_err_t DynI2CMaster::transceive_data_with_client(i2c_master_dev_handle_t device_handle, uint8_t key, dynI2C_meta_t& metadata, std::vector<uint8_t>& data)
{
    // Build request for client device
    dynI2C_getdata_packet_t request = build_getdata_packet(key);

    // Build response buffer
    uint16_t rx_size = sizeof(dynI2C_header_t) + metadata.data_len + sizeof(uint8_t);
    std::vector<uint8_t> rx(rx_size);

    // Send request at most 3 times to client.
    for (uint8_t attempt = 0; attempt < 3; ++attempt)
    {
        // Transceive metadata from client
        esp_err_t error = i2c_master_transmit_receive(
            device_handle,
            reinterpret_cast<const uint8_t*>(&request),
            sizeof(request),
            rx.data(),
            rx_size,
            config.timeout_ms);

        if (error != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to transceive data from client.");
            return error;
        }

        // Get checksum that was transmitted
        const uint8_t received_crc = rx.back();
        rx.pop_back();

        // Compare checksums
        if (received_crc != calculate_crc8(rx.data(), rx.size()))
        {
            ESP_LOGW(TAG, "Checksum mismatch, retrying!");
            continue;
        }

        rx.erase(rx.begin(), rx.begin() + sizeof(dynI2C_header_t));
        data = std::move(rx);
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Continuous checksum mismatch. Reached maximum retries.");
    return ESP_ERR_INVALID_RESPONSE;
}