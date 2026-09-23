#include "DynI2CMaster.h"

static const char *TAG = "DynI2CMaster";

DynI2CMaster::~DynI2CMaster()
{
    deinit();
}

void DynI2CMaster::init(dynI2C_master_cfg_t config)
{
    if (this->initialized)
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
        }};

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &this->i2c_bus_handle));
    ESP_LOGD(TAG, "I2C master bus successfully initialized.");

    // Save used config in instance.
    this->config = config;
    this->initialized = true;
}

void DynI2CMaster::deinit()
{
    if (!this->initialized)
        return;

    // Remove all currently connected device handles from i2c master bus
    for (auto &entry : this->client_entries)
    {
        esp_err_t err = i2c_master_bus_rm_device(entry.second.handle);
        if (err != ESP_OK)
            ESP_LOGW(TAG, "Failed to remove I2C device: %s", esp_err_to_name(err));
    }

    // Clear references to device handles in client_entries;
    this->client_entries.clear();

    // Delete bus and clear reference.
    ESP_ERROR_CHECK(i2c_del_master_bus(this->i2c_bus_handle));
    this->i2c_bus_handle = nullptr;

    // Clear config
    this->config = {};
    this->initialized = false;
    ESP_LOGD(TAG, "I2C master bus successfully deinitialized.");
}

/// @brief Gets meta information about a field of a DynI2C-Client.
/// @param address  The id of the client to address.
/// @param key      The key of the field to address.
/// @param response A pointer to the read response.
/// @return         ESP_OK if the execution worked, another esp_err_t-Value otherwise.
esp_err_t DynI2CMaster::get_meta(uint8_t address, uint8_t key, dynI2C_meta_t *metadata)
{
    // If instance was not initialized we show exception and fail.
    if (!this->initialized)
    {
        ESP_LOGE(TAG, "Failes to request meta data. DynI2CMaster was not intialized.");
        return ESP_ERR_INVALID_STATE;
    }

    // Check if the client has not been accessed before
    if (!this->client_entries.contains(address))
    {
        // If not create client handle and sub_map for new client.
        i2c_master_dev_handle_t device_handle;
        esp_err_t error = this->create_dynamic_i2c_device(address, &device_handle);
        if (error != ESP_OK)
            return error;

        dynI2C_client_entry_t new_entry = {
            .handle = device_handle,
            .metadata = std::map<uint8_t, dynI2C_meta_t>(),
        };
        this->client_entries.insert(std::make_pair(address, new_entry));
        ESP_LOGD(TAG, "Added new dynI2C_client_entry to master instance.");
    }

    // Get entry and check if there is metadata in the entry for the wanted key
    dynI2C_client_entry_t &client_entry = this->client_entries.at(address);
    if (!client_entry.metadata.contains(key))
    {
        dynI2C_meta_packet_t response;
        esp_err_t error = transceive_metadata_with_client(client_entry.handle, key, &response);
        if (error != ESP_OK)
            return error;

        // If dynamic size flag is not set, save to cache
        if (!response.meta.data_flags & (DYNAMIC_SIZE_FLAG))
        {
            client_entry.metadata[key] = response.meta;
            ESP_LOGD(TAG, "Metadata savd to client entry.");
        }

        *metadata = response.meta;
        return ESP_OK;
    }

    // Wrap it all in dynI2C_metares_t and return it.
    *metadata = client_entry.metadata.at(key);
    return ESP_OK;
}

/// @brief Requests data from a field of a DynI2C-Client. Calls get_meta if the address-key-combination is not cached in the instance.
/// @param address        The id of the client to address.
/// @param key            The key of the field to address.
/// @param response       A pointer to the read response.
/// @return               ESP_OK if the execution worked, another esp_err_t-Value otherwise.
esp_err_t DynI2CMaster::get_data(uint8_t address, uint8_t key, dynI2C_data_t *data)
{
    dynI2C_meta_t metadata;
    esp_err_t error = this->get_meta(address, key, &metadata);
    if (error != ESP_OK)
        return error;

    // Send request to client and get data result
    dynI2C_data_packet_t response;
    dynI2C_client_entry_t &client_entry = this->client_entries.at(address);
    error = transceive_data_with_client(client_entry.handle, key, &response);
    if (error != ESP_OK)
        return error;

    memcpy(data->data, response.data, metadata.data_len);
    data->data_len = metadata.data_len;
    return ESP_OK;
}

esp_err_t DynI2CMaster::create_dynamic_i2c_device(uint8_t address, i2c_master_dev_handle_t *device_handle)
{
    // Create device config
    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = this->config.scl_speed_hz,
        .scl_wait_us = this->config.timeout_ms};

    // Add i2c device to bus
    esp_err_t error = i2c_master_bus_add_device(this->i2c_bus_handle, &device_config, device_handle);
    if (error != ESP_OK)
        ESP_LOGE(TAG, "%s occured while add device to master bus", esp_err_to_name(error));

    return error;
}

esp_err_t DynI2CMaster::transceive_metadata_with_client(i2c_master_dev_handle_t handle, uint8_t key, dynI2C_meta_packet_t *response)
{
    // Build request for client device
    dynI2C_getmeta_packet_t request = {
        .header = {
            .magic = DYNI2C_MAGIC_NUMBER,
            .packet_type = GETMETA,
        },
        .data_key = key,
    };
    request.crc_checksum = calculate_crc8(request);

    // Send request at most 3 times to client.
    uint8_t retries = 0;
    do
    {
        // Transceive metadata from client
        esp_err_t error = i2c_master_transmit_receive(
            handle,
            reinterpret_cast<const uint8_t *>(&request),
            sizeof(request),
            reinterpret_cast<uint8_t *>(response),
            sizeof(*response),
            this->config.timeout_ms);

        if (error != ESP_OK)
        {
            ESP_LOGE(TAG, "Failes to transceive meta from client.");
            return error;
        }

        // Compare checksums
        if (response->crc_checksum == calculate_crc8(*response))
            return ESP_OK;

        ESP_LOGW(TAG, "Checksum mismatch, retrying!");
        retries++;
    } while (retries < 3);

    ESP_LOGE(TAG, "Continuous checksum mismatch. Reached maximum retries.");
    return ESP_ERR_INVALID_RESPONSE;
}

esp_err_t DynI2CMaster::transceive_data_with_client(i2c_master_dev_handle_t handle, uint8_t key, dynI2C_data_packet_t *response)
{
    // Build request for client device
    dynI2C_getdata_packet_t request = {
        .header = {
            .magic = DYNI2C_MAGIC_NUMBER,
            .packet_type = GETDATA,
        },
        .data_key = key,
    };
    request.crc_checksum = calculate_crc8(request);

    // Send request at most 3 times to client.
    uint8_t retries = 0;
    do
    {
        // Transceive metadata from client
        esp_err_t error = i2c_master_transmit_receive(
            handle,
            reinterpret_cast<const uint8_t *>(&request),
            sizeof(request),
            reinterpret_cast<uint8_t *>(response),
            sizeof(*response),
            this->config.timeout_ms);

        if (error != ESP_OK)
        {
            ESP_LOGE(TAG, "Failes to transceive meta from client.");
            return error;
        }

        // Compare checksums
        if (response->crc_checksum == calculate_crc8(*response))
            return ESP_OK;

        ESP_LOGW(TAG, "Checksum mismatch, retrying!");
        retries++;
    } while (retries < 3);

    ESP_LOGE(TAG, "Continuous checksum mismatch. Reached maximum retries.");
    return ESP_ERR_INVALID_RESPONSE;
}