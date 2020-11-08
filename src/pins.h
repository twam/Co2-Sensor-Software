/**
 * @file pins.h
 */

#ifndef PINS_H
#define PINS_H

#include <cstdint>

namespace pins {

/// User Led
constexpr uint8_t ledUser = 2u;

/// Disable 3.3V/5V status led
constexpr uint8_t ledDisable = 21u;

/// OLED MOSI
constexpr uint8_t OledMosi = 26u;

/// OLED CLK
constexpr uint8_t OledClk = 27u;

/// OLED DC
constexpr uint8_t OledDc = 33u;

/// OLED CS
constexpr uint8_t OledCs = 32u;

/// OLED Reset
constexpr uint8_t OledReset = 25u;

/// Button 1
constexpr uint8_t Button1 = 34u;

/// Button 2
constexpr uint8_t Button2 = 35u;

/// Button 3
constexpr uint8_t Button3 = 15u;

/// Button 4
constexpr uint8_t Button4 = 0u;

/// I2C SDA
constexpr uint8_t Sda = 23u;

/// I2C SCL
constexpr uint8_t Scl = 22u;

/// LED1 Data
constexpr uint8_t Led1Data = 4u;

/// LED1 Clock
constexpr uint8_t Led1Clk = 5u;

/// LED2 Data
constexpr uint8_t Led2Data = 18u;

/// LED2 Clock
constexpr uint8_t Led2Clk = 19u;

};

#endif
