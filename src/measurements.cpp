#include <Wire.h>

#include "measurements.hpp"
#include "pins.hpp"
#include <type_traits>

void Measurements::setup() {
  // Initialize I2C
  Wire.begin(pins::Sda, pins::Scl);

  setupScd30();
  setupBmp280();
}

void Measurements::loop() {
  bool dataReady;
  if (_scd30.getDataReady(dataReady) and dataReady) {
    _dataLast.shiftMeasurements();
    Measurement& measurement = _dataLast.getMeasurement(0);

    measurement.time = time(nullptr);
    if (not _scd30.getMeasurement(measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Scd30Co2)], measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Scd30Temperature)], measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Scd30Humidity)])) {
      Serial.printf("getMeasurement failed\r\n");
    }

    measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Bmp280Pressure)] = _bmp280.readPressure() / 100.0;
    measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Bmp280Temperature)] = _bmp280.readTemperature();

    Serial.printf(" SDS30:      CO2: %5.0f ppm    Temperature: %5.1f °C   Humidity: %5.1f %%\r\n",
      measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Scd30Co2)],
      measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Scd30Temperature)],
      measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Scd30Humidity)]);
    Serial.printf("BMP280: Pressure: %5.0f mbar   Temperature: %5.1f °C\r\n",
      measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Bmp280Pressure)],
      measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Bmp280Temperature)]);

    // Check if pressure changed and update SCD30 if required
    if (fabs(_pressureScd30 - measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Bmp280Pressure)]) >= 1.0) {
      Serial.printf("Updating ambient pressure from %5.0f mbar to %5.0f mbar.\r\n", _pressureScd30, measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Bmp280Pressure)]);
      if (_scd30.startContinousMeasurement(measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Bmp280Pressure)])) { // mbar
        _pressureScd30 = measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Bmp280Pressure)];
      } else {
        Serial.printf("Update failed.\n");
      }
    }
  }
}

void Measurements::setupBmp280() {
  if (_bmp280.begin(0x76) == false) {
    _errorCallback("Pressure sensor not detected. Please check wiring.");
  }
}

void Measurements::setupScd30() {
   uint8_t major, minor;

  if (_scd30.getFirmwareVersion(major, minor)) {
    printf("SCD30 Firmware: %u.%u\n", major, minor);
  } else {
    _errorCallback("Read out of SCD30 firmware version failed. Please check wiring.");
  }

  // Configure measurement interval
  static constexpr uint16_t desiredMeasurementInterval = 15u;

  uint16_t measurementInterval = 0;
  if (not _scd30.getMeasurementInterval(measurementInterval)) {
    _errorCallback("Read out of SCD30 measurement interval failed.");
  }

  if (measurementInterval != desiredMeasurementInterval) {
    if (not _scd30.setMeasurementInterval(desiredMeasurementInterval)) {
      _errorCallback("Setting of SCD30 measurement interval failed.");
    }

    delay(200);
  }

  // Configure temperature offset
  static constexpr uint16_t desiredTemperatureOffset = 100u;

  uint16_t temperatureOffset = 0;
  if (not _scd30.getTemperatureOffset(temperatureOffset)) {
    _errorCallback("Read out of SCD30 temperature offset failed.");
  }

  if (temperatureOffset != desiredTemperatureOffset) {
    if (not _scd30.setTemperatureOffset(desiredTemperatureOffset)) {
      _errorCallback("Setting of SCD30 temperature offset failed.");
    }

    delay(200);
  }

  // Configure automatic self calibration
  static constexpr bool desiredAutomaticSelfCalibration = true;

  bool automaticSelfCalibration = 0;
  if (not _scd30.getAutomaticSelfCalibration(automaticSelfCalibration)) {
    _errorCallback("Read out of SCD30 automatic self calibration failed.");
  }

  if (automaticSelfCalibration != desiredAutomaticSelfCalibration) {
    if (not _scd30.setAutomaticSelfCalibration(desiredAutomaticSelfCalibration)) {
    _errorCallback("Setting of SCD30 automatic self calibration failed.");
    }

    delay(200);
  }

  // Start measurementsArray
  if (not _scd30.startContinousMeasurement(0)) {
    _errorCallback("Starting of SCD30 continous measurement of failed.");
  }
}
