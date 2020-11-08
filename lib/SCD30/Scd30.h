/**
 * @file SCD30.h
 */

#ifndef SCD30_H
#define SCD30_H

#include <cstdint>
#include <Wire.h>

class Scd30
{
public:
  /// Commands
  enum class Register : uint16_t {
    TriggerContinousMeasurement = 0x0010u,
    StopContinousMeasurement = 0x0104u,
    MeasurementInterval = 0x4600u,
    DataReadyStatus = 0x0202u,
    Measurement = 0x0300u,
    AutomaticSelfCalibration = 0x5306u,
    ForcedRecalibrationValue = 0x5204u,
    TemperatureOffset = 0x5403u,
    AltitudeCompensation = 0x5102u,
    FirmwareVersion = 0xD100u,
    SoftReset = 0xD304u
  };

  static constexpr uint8_t i2CAddress = 0x61u;

  Scd30(TwoWire &wire = Wire) : _wire{wire} {}

  /**
   * @brief Starts the continuous measurement
   *
   * @param[in] ambientPressure optional ambient pressure between 700 and 1400 mBar, 0 to disable
   * @retval true start of continuous measurement successful
   * @retval false start of continuous measurement failed
   */
  bool startContinousMeasurement(uint16_t ambientPressure);

  /**
   * @brief Stops the continuous measurement
   *
   * @retval true stop of continuous measurement successful
   * @retval false stop of continuous measurement write failed
   */
  bool stopContinousMeasurement();

  /**
   * @brief Sets the measurement interval
   *
   * Unit is seconds.
   *
   * @param[in] measurementInterval measurement interval between 2 and 1800 s
   * @retval true measurement interval write successful
   * @retval false measurement interval write failed
   */
  bool setMeasurementInterval(uint16_t measurementInterval);

  /**
   * @brief Gets the measurement interval
   *
   * Unit is seconds.
   *
   * @param[out] measurementInterval measurement interval
   * @retval true measurement interval read-out successful
   * @retval false measurement interval read-out failed
   */
  bool getMeasurementInterval(uint16_t& measurementInterval);

  /**
   * @brief Gets the data ready status
   *
   * @param[out] dataReady data ready
   * @retval true data ready read-out successful
   * @retval false data ready read-out failed
   */
  bool getDataReady(bool& dataReady);

  /**
   * @brief Gets the measurement
   *
   * @param[out] co2Concentration Co2 concentration in ppm
   * @param[out] temperature Co2 concentration in °C
   * @param[out] humidity Co2 concentration in %RH
   * @retval true measurement read-out successful
   * @retval false measurement read-out failed
   */
  bool getMeasurement(float& co2Concentration, float& temperature, float& humidity);

  /**
   * @brief Sets the automatic self calibration
   *
   * @param[in] automaticSelfCalibration automatic self calibration
   * @retval true automatic self calibration write successful
   * @retval false automatic self calibration write failed
   */
  bool setAutomaticSelfCalibration(bool automaticSelfCalibration);

  /**
   * @brief Gets the automatic self calibration
   *
   * @param[out] automaticSelfCalibration automatic self calibration
   * @retval true automatic self calibration read-out successful
   * @retval false automatic self calibration read-out failed
   */
  bool getAutomaticSelfCalibration(bool& automaticSelfCalibration);

  /**
   * @brief Sets the forced recalibration value
   *
   * Unit is ppm.
   *
   * @param[in] forcedRecalibrationValue forced recalibration value
   * @retval true forced recalibration value write successful
   * @retval false forced recalibration value write failed
   */
  bool setForcedRecalibrationValue(uint16_t forcedRecalibrationValue);

  /**
   * @brief Gets the forced recalibration value
   *
   * Unit is ppm.
   *
   * @param[out] forcedRecalibrationValue forced recalibration value
   * @retval true forced recalibration value read-out successful
   * @retval false forced recalibration value read-out failed
   */
  bool getForcedRecalibrationValue(uint16_t& forcedRecalibrationValue);

  /**
   * @brief Sets the temperature offset
   *
   * Unit is 0.01 °C.
   * This value is persisted in non-volatile memory.
   *
   * @param[in] temperatureOffset temperature offset
   * @retval true temperature offset write successful
   * @retval false temperature offset write failed
   */
  bool setTemperatureOffset(uint16_t temperatureOffset);

  /**
   * @brief Gets the temperature offset
   *
   * Unit is 0.01 °C.
   *
   * @param[out] temperatureOffset temperature offset
   * @retval true temperature offset read-out successful
   * @retval false temperature offset read-out failed
   */
  bool getTemperatureOffset(uint16_t& temperatureOffset);

  /**
   * @brief Sets the altitude compensation
   *
   * This is the height above mean sea level in m.
   * Altitude compensation is disregarded by then sensor when an ambient pressure is given (see 1.4.1.).
   * This value is persisted in non-volatile memory.
   *
   * @param[in] altitudeCompensation altitude compensation
   * @retval true altitude compensation write successful
   * @retval false altitude compensation write failed
   */
  bool setAltitudeCompensation(uint16_t altitudeCompensation);

  /**
   * @brief Gets the altitude compensation
   *
   * This is the height above mean sea level in m.
   * Altitude compensation is disregarded by then sensor when an ambient pressure is given (see 1.4.1.).
   *
   * @param[out] altitudeCompensation altitude compensation
   * @retval true altitude compensation read-out successful
   * @retval false altitude compensation read-out failed
   */
  bool getAltitudeCompensation(uint16_t& altitudeCompensation);

  /**
   * @brief Gets the firmware version
   *
   * @param[out] major major part of firmware version
   * @param[out] minor minor part of firmware version
   * @retval true firmware version read-out successful
   * @retval false firmware version read-out failed
   */
  bool getFirmwareVersion(uint8_t& major, uint8_t& minor);

  /**
   * @brief Reset the sensor
   *
   * @retval true reset successful
   * @retval false reset failed
   */
  bool reset();

protected:
  /**
   * @brief Reads a register from the sensor
   *
   * @param[in] reg register to read
   * @param[out] value value read from register
   * @retval true read-out successful
   * @retval false read-out failed
   */
  bool readRegister(Register reg, uint16_t& value);

  /**
   * @brief Writes a register with an uint16_t value to the sensor
   *
   * @param[in] reg register to write
   * @param[in] value value write to register
   * @retval true write successful
   * @retval false write failed
   */
  bool writeRegister(Register reg, uint16_t value);

  /**
   * @brief Writes a register without value to the sensor
   *
   * @param[in] reg register to write
   * @retval true write successful
   * @retval false write failed
   */
  bool writeRegister(Register reg);

  /**
   * @brief Calculates the CRC8 of a value
   *
   * @param[in] value value to calculate crc over
   * @return crc
   */
  static uint8_t calculateCrc8(uint16_t value);

private:
  TwoWire& _wire;

};


#endif
