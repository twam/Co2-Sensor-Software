#ifndef UI_H
#define UI_H

#include <ctime>
#include <functional>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "measurements.hpp"
#include "config.hpp"

class Ui {
public:
  using RestartCallback = std::function<void(void)>;

  Ui(Config& config, const RestartCallback& restartCallback);

  void setup(const Measurements* measurements);
  void loop();

  void showError(const std::string& text);

private:
  static constexpr uint8_t displayWidth{128};
  static constexpr uint8_t displayHeight{64};

  void drawDiagramm(const TimeDataInterface& data);

  Adafruit_SSD1306 _display;
  const Measurements* _measurements{};
  time_t _lastActivity{};
  Config& _config;
  RestartCallback _restartCallback;
};

#endif

