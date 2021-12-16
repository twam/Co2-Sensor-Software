#include <functional>
#include <array>
#include <ctime>
#include <cstdint>

#include <Arduino.h>
#include <SPIFFS.h>

#include "pins.hpp"
#include "measurements.hpp"
#include "ui.hpp"
#include "config.hpp"
#include "network.hpp"

static void restart();

Config config{};

Ui ui{config, restart};

Measurements measurements{[](const std::string& text) {
  ui.showError(text);
  Serial.printf("Error: %s", text.c_str());
}};

Network network{config, restart};

void setup() {
  // Setup serial connection
  Serial.begin(115200);

  // Initialize File System
  if (not SPIFFS.begin(true)) {
    Serial.printf("File system init failed.");
    restart();
  }
  Serial.printf("File system: %u/%u bytes used.\r\n", SPIFFS.usedBytes(), SPIFFS.totalBytes());

  config.setup();

  // Disable LEDs
  pinMode(pins::ledUser, OUTPUT);
  pinMode(pins::ledDisable, OUTPUT);
  digitalWrite(pins::ledUser, LOW);
  digitalWrite(pins::ledDisable, LOW);

  ui.setup(&measurements);
  network.setup(&measurements);
  measurements.setup();
}

void loop() {
  network.loop();
  ui.loop();
  measurements.loop();
}

static void restart() {
  config.finish();

  SPIFFS.end();

  ESP.restart();
}
