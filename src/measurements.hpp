#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include <functional>
#include <string>
#include <ctime>
#include <utility>

#include <Scd30.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

struct Measurement {
  time_t time;
  float scd30Co2;
  float scd30Temperature;
  float scd30Humidity;
  float bmp280Pressure;
  float bmp280Temperature;
};

class TimeDataInterface {
public:
  virtual std::size_t getNumberOfMeasurements() const = 0;

  virtual const Measurement& getMeasurement(std::size_t i) const = 0;
  virtual Measurement& getMeasurement(std::size_t i) = 0;

  virtual void shiftMeasurements() = 0;
};

template <std::size_t N>
class TimeData : public TimeDataInterface {
public:
  std::size_t getNumberOfMeasurements() const override {
    return _numberOfMeasurements;
  }

  const Measurement& getMeasurement(std::size_t i) const override {
    return _data[getIndex(i)];
  }

  Measurement& getMeasurement(std::size_t i) override {
    return _data[getIndex(i)];
  }

  virtual void shiftMeasurements() override {
    _currentMeasurement++;
    if (_currentMeasurement == _numberOfMeasurements) {
      _currentMeasurement = 0;
    }
  }

private:
  std::size_t getIndex(std::size_t i) const {
    int index = (_currentMeasurement - i);
    if (index < 0) {
      index += _numberOfMeasurements;
    }
    return index;
  }

  const std::size_t _numberOfMeasurements = N;
  std::array<Measurement, N> _data;
  std::size_t _currentMeasurement{N-1};
};

class Measurements {
public:
  using ErrorCallback = std::function<void(std::string)>;
  using TimeDataLast = TimeData<100>;

  Measurements(const ErrorCallback& errorCallback) : _errorCallback(std::move(errorCallback)) {};

  void setup();
  void loop();

  const TimeDataInterface& dataLast() const { return _dataLast; };

private:
  void setupScd30();
  void setupBmp280();

  ErrorCallback _errorCallback;
  Scd30 _scd30{};
  Adafruit_BMP280 _bmp280{};

  // Pressure known to SCD30
  float _pressureScd30 = 0u;

  TimeDataLast _dataLast{};
};

#endif
