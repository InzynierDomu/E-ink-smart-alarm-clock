/**
 * @file clock_model.cpp
 * @brief Implementation of the clock model — storing the WiFi configuration.
 */

#include "clock_model.h"

/**
 * @brief Saves the WiFi configuration in the model.
 * @param _config Reference to the Wifi_Config structure with the data to save.
 */
void Clock_model::set_wifi_config(Wifi_Config& _config)
{
  wifi_config = _config;
}

/**
 * @brief Copies the WiFi configuration into the provided structure.
 * @param _config Reference to the structure that will receive the configuration.
 */
void Clock_model::get_wifi_config(Wifi_Config& _config) const
{
  _config = wifi_config;
}