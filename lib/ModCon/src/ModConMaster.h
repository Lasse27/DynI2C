/**
 * @file ModConMaster.h
 * @brief Definitions and type as header file for ModConMaster.cpp
 *
 * @author Lasse-Leander Hillen
 * @date 2026-09-22
 * @version 1.0.0
 */

#ifndef MODCON_MASTER_H
#define MODCON_MASTER_H

#include <esp_err.h>
#include "ModConCommon.h"

class ModConMaster
{
public:
    esp_err_t request_meta(uint8_t client_id, uint8_t key, modcon_metares_t *response);
    esp_err_t request_data(uint8_t client_id, uint8_t key, modcon_datares_t *response);

private:
    esp_err_t send_meta_request(modcon_metareq_t request);
    esp_err_t send_data_request(modcon_datareq_t request);
};

#endif // MODCON_MASTER_H