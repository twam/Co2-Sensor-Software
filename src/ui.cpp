
#include <utility>
#include <type_traits>

#include "ui.hpp"
#include "pins.hpp"

Ui::Ui(Config& config, const RestartCallback& restartCallback) :
   _display{displayWidth, displayHeight, pins::OledMosi, pins::OledClk, pins::OledDc, pins::OledReset, pins::OledCs},
   _config{config},
  _restartCallback{std::move(restartCallback)} {
}

void Ui::setup(const Measurements* measurements, Network *network) {
  _measurements = measurements;
  _network = network;

  // Initialize Buttons
  pinMode(pins::Button1, INPUT);
  pinMode(pins::Button2, INPUT);
  pinMode(pins::Button3, INPUT);
  pinMode(pins::Button4, INPUT);

  // Initialize Display
  _display.begin(SSD1306_SWITCHCAPVCC);
  _display.dim(false);
  _display.clearDisplay();
  _display.setRotation(2);
  _display.setTextColor(SSD1306_WHITE);
  _display.display();
}

void Ui::loop() {
  // Only do something every 50 ms
  if ((millis() < _lastUpdate) or ((millis() - _lastUpdate) < 50)) {
    return;
  }
  _lastUpdate = millis();

  const std::array<bool, 4u> buttonStates{
    digitalRead(pins::Button1) == LOW,
    digitalRead(pins::Button2) == LOW,
    digitalRead(pins::Button3) == LOW,
    digitalRead(pins::Button4) == LOW,
  };
  std::array<bool, 4u> buttonEvent{};

  time_t now;
  time(&now);

  for (size_t i = 0u; i < buttonEvent.size(); ++i) {
    buttonEvent[i] = (_lastButtonStates[i] == false) & (buttonStates[i] == true);
  }
  _lastButtonStates = buttonStates;
  if (((now - _lastActivity) > 60*60*24*365*10) or (buttonEvent[0] or buttonEvent[1] or buttonEvent[2] or buttonEvent[3])) {
    _lastActivity = now;
  }

  const auto sleepTimeOut = _config.getValueAsInt("sleepTimeout").value_or(0);
  const auto shouldSleep = (sleepTimeOut > 0) and ((now - _lastActivity) >= sleepTimeOut);

  if (shouldSleep) {
    if (_sleeping) {
      // Already sleeping. Nothing left to do.
      return;
    } else {
      // We're start sleep, so clean display.
      _display.clearDisplay();
      _display.display();
      _sleeping = true;
      return;
    }
  } else if ((not shouldSleep) and (_sleeping)) {
    // Reset button press events to avoid 'double' actions (wake-up and following act ion)
    _sleeping = false;
    std::fill(buttonEvent.begin(), buttonEvent.end(), false);
  }

  _display.clearDisplay();
  _display.setTextColor(SSD1306_WHITE);
  _display.setRotation(2);

  switch (_screen) {
    case Screen::Co2Current: {
      drawStatusbar("Co2");
      drawNavigation("\x1B", "", "", "\x1A");

      _display.setCursor(0, 16);
      // _display.setTextSize(3);
      _display.setTextColor(SSD1306_WHITE);

      const auto& measurement = _measurements->dataLast().getMeasurement(0);
      const int16_t offsetBottom = 17;

      uint16_t unit_w = 0;
      _display.setTextSize(2);
      {
        int16_t x1, y1;
        uint16_t w, h;

        _display.getTextBounds("ppm", 0, 0, &x1, &y1, &w, &h);
        _display.setCursor(displayWidth - w, displayHeight - offsetBottom - h);
        _display.printf("ppm");
        unit_w = w;
      }

      _display.setTextSize(3);
      {
        int16_t x1, y1;
        uint16_t w, h;
        _display.getTextBounds("00000", 0, 0, &x1, &y1, &w, &h);
        _display.setCursor(displayWidth - (w + unit_w + 2), displayHeight - offsetBottom - h);
        _display.printf("%5.f", measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Scd30Co2)]);
      }

      break;
    }

    case Screen::CurrentMeasurements: {
      drawStatusbar("Akt. Messwerte");
      drawNavigation("\x1B", "", "", "\x1A");

      _display.setTextSize(1);
      _display.setTextColor(SSD1306_WHITE);

      const auto& measurement = _measurements->dataLast().getMeasurement(0);

      _display.setCursor(0, 16);
      _display.setTextSize(1);
      _display.printf("Co2         %5.0f ppm\n", measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Scd30Co2)]);
      _display.printf("Temperature %5.1f \xF9" "C \n", measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Scd30Temperature)]);
      _display.printf("Humidity     %4.1f %%  \n", measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Scd30Humidity)]);
      _display.printf("Pressure   %5.0f mBar\n", measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Bmp280Pressure)]);

      break;
    }

    case Screen::Co2History: {
      drawStatusbar("Co2: 25 minutes"); // 15s * 100 = 25 min
      drawNavigation("\x1B", "", "", "\x1A");

      drawDiagramm(_measurements->dataLast(), 11, Quantity::Scd30Co2);

      break;
    }

    case Screen::TemperatureHistory: {
      drawStatusbar("Temperature: 25 minutes"); // 15s * 100 = 25 min
      drawNavigation("\x1B", "", "", "\x1A");

      drawDiagramm(_measurements->dataLast(), 11, Quantity::Scd30Temperature);

      break;
    }

    case Screen::HumidityHistory: {
      drawStatusbar("Humidity: 25 minutes"); // 15s * 100 = 25 min
      drawNavigation("\x1B", "", "", "\x1A");

      drawDiagramm(_measurements->dataLast(), 11, Quantity::Scd30Humidity);

      break;
    }

    case Screen::PressureHistory: {
      drawStatusbar("Pressure: 25 minutes"); // 15s * 100 = 25 min
      drawNavigation("\x1B", "", "", "\x1A");

      drawDiagramm(_measurements->dataLast(), 11, Quantity::Bmp280Pressure);

      break;
    }

    case Screen::Time: {
      drawStatusbar("Date/Time"); // 15s * 100 = 25 min
      drawNavigation("\x1B", "", "", "\x1A");

      // Time
      _display.setCursor(0, 16);
      _display.setTextSize(2);
      _display.setTextColor(SSD1306_WHITE);

      struct tm timeinfo;
      localtime_r(&now, &timeinfo);
      if(timeinfo.tm_year > (2020 - 1900)) {
        char buff[20];
        strftime(buff, sizeof(buff), "%Y-%m-%d\n%H:%M:%S", &timeinfo);
        _display.printf(buff);
      } else {
        _display.printf("No time configured");
      }

      break;
    }

    case Screen::Version: {
      drawStatusbar("Version"); // 15s * 100 = 25 min
      drawNavigation("\x1B", "", "", "\x1A");
      _display.setCursor(0, 16);
      _display.setTextSize(1);
      _display.setTextColor(SSD1306_WHITE);
      _display.printf(GIT_DESCRIBE);
      break;
    }

    default:
      break;
  }

  if (buttonEvent[0]) {
    _screen = static_cast<Screen>(static_cast<std::underlying_type_t<Screen>>(_screen) > 0u ? static_cast<std::underlying_type_t<Screen>>(_screen) - 1u : static_cast<std::underlying_type_t<Screen>>(Screen::NumberOfScreens) - 1u);
  } else if (buttonEvent[3]) {
    _screen = static_cast<Screen>(static_cast<std::underlying_type_t<Screen>>(_screen) < (static_cast<std::underlying_type_t<Screen>>(Screen::NumberOfScreens) - 1u) ? static_cast<std::underlying_type_t<Screen>>(_screen) + 1u : 0u);
  }

  _display.display();
}

void Ui::drawNavigation(const char* text1, const char* text2, const char* text3, const char* text4) {
  _display.setTextSize(1);
  _display.setTextColor(SSD1306_BLACK);

  for (uint8_t button = 0; button < 4; button ++) {
    const char* text = nullptr;
    switch (button) {
      case 0:
        text = text1;
        break;
      case 1:
        text = text2;
        break;
      case 2:
        text = text3;
        break;
      case 3:
        text = text4;
        break;
    }

    if (text == nullptr) {
      continue;
    }

    _display.fillRect(32*button, displayHeight-9, 31, 9, SSD1306_WHITE);
    int16_t x1, y1;
    uint16_t w, h;
    _display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    _display.setCursor(32*button+1+(31-w)/2, displayHeight-8);
    _display.printf(text);
  }
}

void Ui::drawStatusbar(const char* title) {
  _display.drawFastHLine(0, 8, displayWidth, SSD1306_WHITE);

  _display.setTextSize(1);
  _display.setTextColor(SSD1306_WHITE);

  _display.setCursor(0, 0);
  _display.printf("%.19s", title);

  // RSSI
  switch (_network->getState()) {
    case Network::State::INITIAL:
      break;

    case Network::State::CONFIGURATION_MODE:
      _display.setCursor(displayWidth-7, 0);
      _display.printf("C");
      break;

    case Network::State::CONFIGURED: {
      uint8_t rssi_bars = 0u;
      if (_network) {
        if (_network->isWifiConnected()) {
          const auto rssi = _network->getWifiRssi();
          if (rssi > -80) {
            rssi_bars = 5;
          } else if (rssi > -90) {
            rssi_bars = 4;
          } else if (rssi > -100) {
            rssi_bars = 3;
          } else if (rssi > -106) {
            rssi_bars = 2;
          } else {
            rssi_bars = 1;
          }
        }
      }

      for (uint8_t rssi_bar = 0; rssi_bar < rssi_bars; rssi_bar++) {
        _display.drawFastVLine(displayWidth-9+rssi_bar*2, 7-(rssi_bar+1), rssi_bar+1, SSD1306_WHITE);
      }
      break;
    }

    case Network::State::NOT_CONFIGURED:
      _display.setCursor(displayWidth-7, 0);
      _display.printf("-");
      break;
  }
}

void Ui::drawDiagramm(const TimeDataInterface& data, int16_t y, Quantity quantity) {
  // Diagram
  const int16_t height = 41;
  const int16_t width = data.getNumberOfMeasurements();

  const int16_t x = displayWidth - width - 2;  // 25;

  int16_t minimum;
  int16_t maximum;

  switch (quantity) {
    case Quantity::Scd30Co2:
      minimum = 400;
      maximum = 2000;
      break;
    case Quantity::Scd30Temperature:
      minimum = 0;
      maximum = 40;
      break;
    case Quantity::Scd30Humidity:
      minimum = 0;
      maximum = 100;
      break;
    case Quantity::Bmp280Pressure:
      minimum = 950;
      maximum = 1050;
      break;
    default:
      return;
  }

  _display.drawRect(x, y, width + 2, height, SSD1306_WHITE);

  _display.setTextSize(1);
  _display.setTextColor(SSD1306_WHITE);

  // Show maximum label
  _display.drawLine(x - 1, y, x, y, SSD1306_WHITE);
  {
    char* label;
    asprintf(&label, "%i", maximum);

    int16_t x1, y1;
    uint16_t w, h;

    _display.getTextBounds(label, 0, 0, &x1, &y1, &w, &h);
    _display.setCursor(x - 1 - w, y);
    _display.printf(label);

    free(label);
  }

  // Show middle label
  _display.drawLine(x - 1, y + (height - 1) / 2, x, y + (height - 1) / 2, SSD1306_WHITE);
  {
    char* label;
    asprintf(&label, "%i", (maximum - minimum) / 2 + minimum);

    int16_t x1, y1;
    uint16_t w, h;

    _display.getTextBounds(label, 0, 0, &x1, &y1, &w, &h);
    _display.setCursor(x - 1 - w, y + (height - 1) / 2 - h / 2);
    _display.printf(label);

    free(label);
  }

  // Show minimum label
  _display.drawLine(x - 1, y + (height - 1), x, y + (height - 1), SSD1306_WHITE);
  {
    char* label;
    asprintf(&label, "%i", minimum);

    int16_t x1, y1;
    uint16_t w, h;

    _display.getTextBounds(label, 0, 0, &x1, &y1, &w, &h);
    _display.setCursor(x - 1 - w, y + (height) - h);
    _display.printf(label);

    free(label);
  }

  for (size_t i = 0; i < width; ++i) {
    const int16_t co2 = round(((data.getMeasurement(i).data[static_cast<std::underlying_type_t<Quantity>>(quantity)]) - minimum) * (height - 1) / (maximum - minimum));
    if ((co2 >= 0) and (co2 <= (height - 1))) {
      _display.drawPixel(x + width - i, y + (height - 1) - co2, SSD1306_WHITE);
    }
  }
}

void Ui::showError(const std::string& text) {
  _display.clearDisplay();
  {
    const char* label = "Error";

    int16_t x1, y1;
    uint16_t w, h;

    _display.setTextSize(2);
    _display.getTextBounds(label, 0, 0, &x1, &y1, &w, &h);
    _display.setCursor(63 - w / 2, 0);
    _display.printf(label);
  }

  _display.setTextSize(1);
  _display.setCursor(0, 16);
  _display.printf(text.c_str());

  _display.setCursor(95, 56);
  _display.printf("Reset");

  _display.display();

  while (true) {
    if (digitalRead(pins::Button4) == false) {
      _restartCallback();
    }
    delay(10);
  }
}
