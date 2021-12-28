#ifndef UI_H
#define UI_H

#include <ctime>
#include <functional>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "measurements.hpp"
#include "config.hpp"
#include "network.hpp"

class Ui {
public:
  using RestartCallback = std::function<void(void)>;

  Ui(Config& config, const RestartCallback& restartCallback);

  void setup(const Measurements* measurements, Network *network);
  void loop();

  void showError(const std::string& text);

private:
  enum class Screen : uint8_t {
    Co2Current,
    Co2History,
    TemperatureHistory,
    HumidityHistory,
    PressureHistory,
    CurrentMeasurements,
    Time,
    Version,
    NumberOfScreens
  };

  static constexpr uint8_t displayWidth{128};
  static constexpr uint8_t displayHeight{64};

  void drawStatusbar(const char* title);
  void drawNavigation(const char* text1 = nullptr, const char* text2 = nullptr, const char* text3 = nullptr, const char* text4 = nullptr);
  void drawDiagramm(const TimeDataInterface& data, int16_t y, Quantity quantity);

  Adafruit_SSD1306 _display;
  const Measurements* _measurements{};
  Network* _network{};
  time_t _lastActivity{};
  Config& _config;
  RestartCallback _restartCallback;
  Screen _screen{};

  std::array<bool, 4u> _lastButtonStates{};
  unsigned long _lastUpdate{};
  bool _sleeping{false};
};

#endif

