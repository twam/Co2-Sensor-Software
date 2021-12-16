
#include <utility>
#include "ui.hpp"
#include "pins.hpp"

Ui::Ui(Config& config, const RestartCallback& restartCallback) : _display{displayWidth, displayHeight, pins::OledMosi, pins::OledClk, pins::OledDc, pins::OledReset, pins::OledCs}, _config{config},
                                          _restartCallback{std::move(restartCallback)} {
}

void Ui::setup(const Measurements* measurements) {
  _measurements = measurements;

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
#if 1
  auto& measurement = _measurements->dataLast().getMeasurement(0);

  const auto button1 = digitalRead(pins::Button1) == LOW;
  const auto button2 = digitalRead(pins::Button2) == LOW;
  const auto button3 = digitalRead(pins::Button3) == LOW;
  const auto button4 = digitalRead(pins::Button4) == LOW;

  if (button1 or button2 or button3 or button4) {
    _lastActivity = time(nullptr);
  }

  const auto displayEnabled = (time(nullptr) - _lastActivity) < _config.getValueAsInt("sleepTimeout").value_or(0);

  if (not displayEnabled) {
    _display.clearDisplay();
    _display.display();
    return;
  }

  if (button4) {
    // Clear the buffer.
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);
    _display.setRotation(2);
    _display.setTextSize(2);
    _display.setCursor(0, 0);

    _display.printf("%5.0f ppm\n", measurement.scd30Co2);
    _display.printf("%5.1f C\n", measurement.scd30Temperature);
    _display.printf("%5.1f %%\n", measurement.scd30Humidity);
    _display.printf("%5.0f mBar\n", measurement.bmp280Pressure);

    _display.display();
  } else {
    // Clear the buffer.
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);
    _display.setRotation(2);
    _display.setTextSize(1);

    drawDiagramm(_measurements->dataLast());

    uint16_t unit_w = 0;
    _display.setTextSize(1);
    {
      int16_t x1, y1;
      uint16_t w, h;

      _display.getTextBounds("ppm", 0, 0, &x1, &y1, &w, &h);
      _display.setCursor(displayWidth - w, displayHeight - h);
      _display.printf("ppm");

      unit_w = w;
    }

    _display.setTextSize(2);
    {
      int16_t x1, y1;
      uint16_t w, h;

      _display.getTextBounds("00000", 0, 0, &x1, &y1, &w, &h);
      _display.setCursor(displayWidth - (w + unit_w + 2), displayHeight - h);
      _display.printf("%5.f", measurement.scd30Co2);
    }

    _display.display();
  }
#endif

  delay(50);
}

void Ui::drawDiagramm(const TimeDataInterface& data) {
  // Diagram
  const int16_t height = 41;
  const int16_t width = data.getNumberOfMeasurements();

  const int16_t x = displayWidth - width - 2;  // 25;
  const int16_t y = 0;

  const int16_t minimum = 400;
  const int16_t maximum = 2000;

  _display.drawRect(x, 0, width + 2, height, SSD1306_WHITE);

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
    _display.setCursor(x - 1 - w, y + (height - 1) - h);
    _display.printf(label);

    free(label);
  }

  for (size_t i = 0; i < width; ++i) {
    const int16_t co2 = round(((data.getMeasurement(i).scd30Co2) - minimum) * (height - 1) / (maximum - minimum));
    if ((co2 >= 0) and (co2 <= (height - 1))) {
      _display.drawPixel(x + width - i, (height - 1) - co2, SSD1306_WHITE);
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
