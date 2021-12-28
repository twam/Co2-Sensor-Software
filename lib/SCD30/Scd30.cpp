/**
 * @file SCD30.cpp
 */

#include <Arduino.h>
#include "Scd30.h"
#include <climits>

bool Scd30::startContinousMeasurement(uint16_t ambientPressure) {
  if ((ambientPressure != 0) and ((ambientPressure < 700) or (ambientPressure > 1400))) {
    return false;
  }

  return writeRegister(Scd30::Register::TriggerContinousMeasurement, ambientPressure);
}

bool Scd30::stopContinousMeasurement() {
  return writeRegister(Scd30::Register::StopContinousMeasurement);
}

bool Scd30::setMeasurementInterval(uint16_t measurementInterval) {
  if ((measurementInterval < 2) or (measurementInterval > 1800)) {
    return false;
  }
  return writeRegister(Scd30::Register::MeasurementInterval, measurementInterval);
}

bool Scd30::getMeasurementInterval(uint16_t& measurementInterval) {
  return readRegister(Scd30::Register::MeasurementInterval, measurementInterval);
}

bool Scd30::getDataReady(bool& dataReady) {
  uint16_t value;
  if (not readRegister(Scd30::Register::DataReadyStatus, value)) {
    return false;
  }

  if (value > 1) {
    return false;
  }

  dataReady = value;

  return true;
}

bool Scd30::getMeasurement(float& co2Concentration, float& temperature, float& humidity) {
  _wire.beginTransmission(i2CAddress);
  _wire.write(static_cast<uint16_t>(Register::Measurement) >> 8);
  _wire.write(static_cast<uint16_t>(Register::Measurement));
  if (_wire.endTransmission() != 0) {
    return false;
  }

  const size_t bytes = 18;

  _wire.requestFrom(i2CAddress, bytes);
  if (not (_wire.available() == bytes)) {
    return false;
  }

  // Get data
  uint8_t data[bytes/3*2];
  uint8_t crc[bytes/3];

  for (size_t i = 0; i < bytes; ++i) {
    const auto index = i / 3;
    const auto offset = i % 3;
    if (offset < 2) {
      data[2*(index^1)+(offset^1)] = _wire.read();
    } else {
      crc[index^1] = _wire.read();
    }
  }

  // Check CRCs
  for (size_t i = 0; i < bytes/3; ++i) {
    uint16_t value;
    memcpy(&value, &data[2*i], sizeof(uint16_t));
    if (calculateCrc8(value) != crc[i]) {
      return false;
    }
  }

  memcpy(&co2Concentration, &data[0], sizeof(float));
  memcpy(&temperature, &data[4], sizeof(float));
  memcpy(&humidity, &data[8], sizeof(float));

  return true;
}

bool Scd30::setAutomaticSelfCalibration(bool automaticSelfCalibration) {
  return writeRegister(Scd30::Register::AutomaticSelfCalibration, automaticSelfCalibration ? 1 : 0);
}

bool Scd30::getAutomaticSelfCalibration(bool& automaticSelfCalibration) {
  uint16_t value;
  if (not readRegister(Scd30::Register::AutomaticSelfCalibration, value)) {
    return false;
  }

  if (value > 1) {
    return false;
  }

  automaticSelfCalibration = value;

  return true;
}

bool Scd30::setForcedRecalibrationValue(uint16_t forcedRecalibrationValue) {
  return writeRegister(Scd30::Register::ForcedRecalibrationValue, forcedRecalibrationValue);
}

bool Scd30::getForcedRecalibrationValue(uint16_t& forcedRecalibrationValue) {
  return readRegister(Scd30::Register::ForcedRecalibrationValue, forcedRecalibrationValue);
}

bool Scd30::setTemperatureOffset(uint16_t temperatureOffset) {
  return writeRegister(Scd30::Register::TemperatureOffset, temperatureOffset);
}

bool Scd30::getTemperatureOffset(uint16_t& temperatureOffset) {
  return readRegister(Scd30::Register::TemperatureOffset, temperatureOffset);
}

bool Scd30::setAltitudeCompensation(uint16_t altitudeCompensation) {
  return writeRegister(Scd30::Register::AltitudeCompensation, altitudeCompensation);
}

bool Scd30::getAltitudeCompensation(uint16_t& altitudeCompensation) {
  return readRegister(Scd30::Register::AltitudeCompensation, altitudeCompensation);
}

bool Scd30::getFirmwareVersion(uint8_t& major, uint8_t& minor) {
  uint16_t value;
  if (not readRegister(Scd30::Register::FirmwareVersion, value)) {
    return false;
  }

  major = value >> 8;
  minor = value;

  return true;
}

bool Scd30::reset() {
  return writeRegister(Scd30::Register::SoftReset);
}

bool Scd30::readRegister(Register reg, uint16_t& value) {
  _wire.beginTransmission(i2CAddress);
  _wire.write(static_cast<uint16_t>(reg) >> 8);
  _wire.write(static_cast<uint16_t>(reg));
  if (_wire.endTransmission() != 0) {
    return false;
  }

  _wire.requestFrom(i2CAddress, sizeof(uint16_t)+1);
  if (not (_wire.available() == sizeof(uint16_t)+1)) {
    return false;
  }

  value = (_wire.read() << 8);
  value |= _wire.read();
  const auto crc = _wire.read();

  return calculateCrc8(value) == crc;
}

bool Scd30::writeRegister(Register reg, uint16_t value) {
  _wire.beginTransmission(i2CAddress);
  _wire.write(static_cast<uint16_t>(reg) >> 8);
  _wire.write(static_cast<uint16_t>(reg));
  _wire.write(static_cast<uint16_t>(value) >> 8);
  _wire.write(static_cast<uint16_t>(value));
  _wire.write(calculateCrc8(value));
  if (_wire.endTransmission() != 0) {
    return false;
  }

  return true;
}

bool Scd30::writeRegister(Register reg) {
  _wire.beginTransmission(i2CAddress);
  _wire.write(static_cast<uint16_t>(reg) >> 8);
  _wire.write(static_cast<uint16_t>(reg));
  if (_wire.endTransmission() != 0) {
    return false;
  }

  return true;
}

uint8_t Scd30::calculateCrc8(uint16_t value) {
  // CRC properties (see 1.1.3.):
  static constexpr uint8_t initialization = 0xFF;
  static constexpr uint8_t polynomial = 0x31;

  uint8_t crc = initialization;

  for (size_t byte = 0; byte < sizeof(uint16_t); byte++) {
    crc ^= reinterpret_cast<uint8_t*>(&value)[sizeof(uint16_t)-1-byte];

    for (uint8_t bit = 0; bit < CHAR_BIT; bit++) {
      if ((crc & 0x80) != 0) {
        crc = ((crc << 1) ^ polynomial);
      } else {
        crc <<= 1;
      }
    }
  }

  return crc;
}
