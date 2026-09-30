/**
 * @file main.h
 * @brief Definitions and type as header file for main.cpp
 *
 * @author Lasse-Leander Hillen
 * @date 2026-09-23
 * @version 1.0.0
 */

#ifndef MAIN_H
#define MAIN_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "sdkconfig.h"


#define I2C_MASTER_SCL_IO GPIO_NUM_4
#define I2C_MASTER_SDA_IO GPIO_NUM_5
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 400000 // Hz
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0
#define I2C_MASTER_TIMEOUT_MS 1000

#endif // MAIN_H