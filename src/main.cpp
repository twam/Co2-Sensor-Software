#include <array>
#include <ctime>
#include <cstdint>

#include <Arduino.h>
#include <Wire.h>
#include <Scd30.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "pins.h"

Scd30 scd30{};
Adafruit_BMP280 bmp280;

Adafruit_SSD1306 display{128, 64, pins::OledMosi, pins::OledClk, pins::OledDc, pins::OledReset, pins::OledCs};

uint16_t last_co2_ppm = 0u;

struct Measurement {
  time_t time;
  float scd30Co2;
  float scd30Temperature;
  float scd30Humidity;
  float bmp280Pressure;
  float bmp280Temperature;
};

constexpr size_t numberOfMeasurements = 100u;
std::array<Measurement, numberOfMeasurements> measurements{0};
size_t currentMeasurement = numberOfMeasurements-1;

// Pressure known to SCD30
float pressureScd30 = 0u;

void showError(const char* errorText) {
  display.clearDisplay();
  {
    const char *label = "Error";

    int16_t x1, y1;
    uint16_t w, h;

    display.setTextSize(2);
    display.getTextBounds(label, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(63-w/2, 0);
    display.printf(label);
  }

  display.setTextSize(1);
  display.setCursor(0, 16);
  display.printf(errorText);

  display.setCursor(95, 56);
  display.printf("Reset");

  display.display();

  Serial.printf("Error: %s", errorText);

  while (true) {
    if (digitalRead(pins::Button4) == false) {
      ESP.restart();
    }
    delay(10);
  }
}

void setupScd30() {
   uint8_t major, minor;
  if (scd30.getFirmwareVersion(major, minor)) {
    printf("SCD30 Firmware: %u.%u\n", major, minor);
  } else {
    showError("Read out of SCD30 firmware version failed. Please check wiring.");
  }

  // Configure measurement interval
  static constexpr uint16_t desiredMeasurementInterval = 15u;

  uint16_t measurementInterval = 0;
  if (not scd30.getMeasurementInterval(measurementInterval)) {
    showError("Read out of SCD30 measurement interval failed.");
  }

  if (measurementInterval != desiredMeasurementInterval) {
    if (not scd30.setMeasurementInterval(desiredMeasurementInterval)) {
      showError("Setting of SCD30 measurement interval failed.");
    }

    delay(200);
  }

  // Configure temperature offset
  static constexpr uint16_t desiredTemperatureOffset = 100u;

  uint16_t temperatureOffset = 0;
  if (not scd30.getTemperatureOffset(temperatureOffset)) {
    showError("Read out of SCD30 temperature offset failed.");
  }

  if (temperatureOffset != desiredTemperatureOffset) {
    if (not scd30.setTemperatureOffset(desiredTemperatureOffset)) {
      showError("Setting of SCD30 temperature offset failed.");
    }

    delay(200);
  }

  // Configure automatic self calibration
  static constexpr bool desiredAutomaticSelfCalibration = true;

  bool automaticSelfCalibration = 0;
  if (not scd30.getAutomaticSelfCalibration(automaticSelfCalibration)) {
    showError("Read out of SCD30 automatic self calibration failed.");
  }

  if (automaticSelfCalibration != desiredAutomaticSelfCalibration) {
    if (not scd30.setAutomaticSelfCalibration(desiredAutomaticSelfCalibration)) {
    showError("Setting of SCD30 automatic self calibration failed.");
    }

    delay(200);
  }

  // Start measurements
  if (not scd30.startContinousMeasurement(0)) {
    showError("Starting of SCD30 continous measurement of failed.");
  }
}

void setup()
{
  // Setup serial connection
  Serial.begin(115200);

  // Disable LEDs
  pinMode(pins::ledUser, OUTPUT);
  pinMode(pins::ledDisable, OUTPUT);
  digitalWrite(pins::ledUser, LOW);
  digitalWrite(pins::ledDisable, LOW);

  // Initialize Buttons
  pinMode(pins::Button1, INPUT);
  pinMode(pins::Button2, INPUT);
  pinMode(pins::Button3, INPUT);
  pinMode(pins::Button4, INPUT);

  // Initialize Display
  display.begin(SSD1306_SWITCHCAPVCC);
  display.dim(true);
  display.clearDisplay();
  display.setRotation(2);
  display.setTextColor(SSD1306_WHITE);
  display.display();

  // Initialize I2C
  Wire.begin(pins::Sda, pins::Scl);

  // Initialize SCD30
  setupScd30();

  // Initialize BMP280
  if (bmp280.begin(0x76) == false) {
    showError("Pressure sensor not detected. Please check wiring.");
  }

  // trafficlight::setup();
}

void loop()
{
  bool dataReady;
  if (scd30.getDataReady(dataReady) and dataReady) {
    currentMeasurement++;
    if (currentMeasurement == numberOfMeasurements) {
      currentMeasurement = 0;
    }
    Measurement& measurement = measurements[currentMeasurement];

    measurement.time = time(nullptr);
    if (not scd30.getMeasurement(measurement.scd30Co2, measurement.scd30Temperature, measurement.scd30Humidity)) {
      Serial.printf("getMeasurement failed\r\n");
    }

    measurement.bmp280Pressure = bmp280.readPressure() / 100.0;
    measurement.bmp280Temperature = bmp280.readTemperature();

    Serial.printf(" SDS30:      CO2: %5.0f ppm    Temperature: %5.1f °C   Humidity: %5.1f %%\r\n",
      measurement.scd30Co2,
      measurement.scd30Temperature,
      measurement.scd30Humidity);
    Serial.printf("BMP280: Pressure: %5.0f mbar   Temperature: %5.1f °C\r\n",
      measurement.bmp280Pressure,
      measurement.bmp280Temperature);

    // Check if pressure changed and update SCD30 if required
    if (fabs(pressureScd30 - measurement.bmp280Pressure) >= 1.0) {
      Serial.printf("Updating ambient pressure from %5.0f mbar to %5.0f mbar.\r\n", pressureScd30, measurement.bmp280Pressure);
      if (scd30.startContinousMeasurement(measurement.bmp280Pressure)) { // mbar
        pressureScd30 = measurement.bmp280Pressure;
      } else {
        Serial.printf("Update failed.\n");
      }
    }
  }

  Measurement& measurement = measurements[currentMeasurement];

  const auto button1 = digitalRead(pins::Button1) == LOW;
  const auto button2 = digitalRead(pins::Button2) == LOW;
  const auto button3 = digitalRead(pins::Button3) == LOW;
  const auto button4 = digitalRead(pins::Button4) == LOW;

  if (button4) {
    // Clear the buffer.
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setRotation(2);
    display.setTextSize(2);
    display.setCursor(0, 0);

    display.printf("%5.0f ppm\n", measurement.scd30Co2);
    display.printf("%5.1f C\n", measurement.scd30Temperature);
    display.printf("%5.1f %%\n", measurement.scd30Humidity);
    display.printf("%5.0f mBar\n", measurement.bmp280Pressure);

    display.display();
  } else {
    // Clear the buffer.
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setRotation(2);
    display.setTextSize(1);

    // Diagram
    const int16_t x = 25;
    const int16_t y = 0;

    const int16_t height = 41;
    const int16_t width = numberOfMeasurements;

    const int16_t minimum = 400;
    const int16_t maximum = 2000;

    display.drawRect(x, 0, width+2, height, SSD1306_WHITE);

    // Show maximum label
    display.drawLine(x-1, y, x, y, SSD1306_WHITE);
    {
      char *label;
      asprintf(&label, "%i", maximum);

      int16_t x1, y1;
      uint16_t w, h;

      display.getTextBounds(label, 0, 0, &x1, &y1, &w, &h);
      display.setCursor(x-1-w, y);
      display.printf(label);

      free(label);
    }

    // Show middle label
    display.drawLine(x-1, y+(height-1)/2, x, y+(height-1)/2, SSD1306_WHITE);
    {
      char *label;
      asprintf(&label, "%i", (maximum-minimum)/2+minimum);

      int16_t x1, y1;
      uint16_t w, h;

      display.getTextBounds(label, 0, 0, &x1, &y1, &w, &h);
      display.setCursor(x-1-w, y + (height-1)/2 - h/2);
      display.printf(label);

      free(label);
    }

    // Show minimum label
    display.drawLine(x-1, y+(height-1), x, y+(height-1), SSD1306_WHITE);
    {
      char *label;
      asprintf(&label, "%i", minimum);

      int16_t x1, y1;
      uint16_t w, h;

      display.getTextBounds(label, 0, 0, &x1, &y1, &w, &h);
      display.setCursor(x-1-w, y + (height-1) - h);
      display.printf(label);

      free(label);
    }

    for (size_t i = 0; i < width; ++i) {
      int pos = (currentMeasurement - i);
      if (pos < 0) {
        pos +=  numberOfMeasurements;
      }
      const int16_t co2 = round(((measurements[pos].scd30Co2)-minimum)*(height-1)/(maximum-minimum));
      if ((co2 >= 0) and (co2 <= (height-1))) {
        display.drawPixel(x+width-i, (height-1)-co2, SSD1306_WHITE);
      }
    }

    // text display tests
    display.setTextSize(2);
    {
      int16_t x1, y1;
      uint16_t w, h;

      display.getTextBounds("00000 ppm", 0, 0, &x1, &y1, &w, &h);
      display.setCursor(63-w/2, 63-h);
      display.printf("%5.f ppm", measurement.scd30Co2);
    }

    display.display();
  }

  delay(50);
}
