
#include <WiFi.h>
// #include <WiFiClient.h>
// #include <WiFiClientSecure.h>
// #include <HardwareSerial.h>
// #include <hwcrypto/sha.h>
// #include <ESPmDNS.h>

#include "network.hpp"
#include "pins.hpp"
#include "html.hpp"

void Network::setup(const Measurements* measurements) {
  _measurements = measurements;

  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.persistent(false);

  setupWebserver();

  pinMode(pins::Button1, INPUT);
  const auto button1 = digitalRead(pins::Button1) == LOW;
  if (button1) {
    Serial.printf("Button 1 pressed. Going into configuration mode.\r\n");
    gotoState(State::CONFIGURATION_MODE);
  } else if (isWifiConfigured()) {
    gotoState(State::CONFIGURED);
  } else {
    gotoState(State::NOT_CONFIGURED);
  }
}

void Network::setupWebserver() {
  _webServer.on("/", [this]() { onWebServerRoot(); });
  _webServer.on("/config", [this]() { onWebServerConfig(); });
}

void Network::loop() {
  switch (_state) {
    case State::INITIAL:
      break;

    case State::CONFIGURATION_MODE:
      _dnsServer.processNextRequest();
      _webServer.handleClient();
      break;

    case State::CONFIGURED:
      if ((_onConnectHandled == false) and (isWifiConnected())) {
        _onConnectHandled = true;
        onWifiConnect();
      } else if ((_onConnectHandled == true) and (not isWifiConnected())) {
        _onConnectHandled = false;
      }

      _webServer.handleClient();
      break;

    case State::NOT_CONFIGURED:
      break;
  }
}

void Network::gotoState(State state) {
  switch (_state) {
    case State::INITIAL:
      break;
    case State::CONFIGURATION_MODE:
      exitConfigurationMode();
      break;
    case State::CONFIGURED:
      exitConfiguredMode();
      break;
    case State::NOT_CONFIGURED:
      exitNotConfiguredMode();
      break;
  }

  _state = state;

  switch (_state) {
    case State::INITIAL:
      break;
    case State::CONFIGURATION_MODE:
      enterConfigurationMode();
      break;
    case State::CONFIGURED:
      enterConfiguredMode();
      break;
    case State::NOT_CONFIGURED:
      enterNotConfiguredMode();
      break;
  }
}

void Network::enterConfigurationMode() {
  Serial.println(__FUNCTION__);

  WiFi.mode(WIFI_AP);
  // WiFi.persistent(false);

  const IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("Co2-Sensor", nullptr, 1, false, 1);//, "", selectChannelForAp());

  _dnsServer.setTTL(0);
  _dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  _dnsServer.start(53, "*", apIP);

  _webServer.begin();
}

void Network::exitConfigurationMode() {
  Serial.println(__FUNCTION__);

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_MODE_NULL);

  _dnsServer.stop();
  _webServer.stop();
}

void Network::enterConfiguredMode() {
  if (WiFi.getAutoConnect()) {
    WiFi.setAutoConnect(false);
  }
  if (!WiFi.getAutoReconnect()) {
    WiFi.setAutoReconnect(true);
  }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(_config.getValueAsString("hostname").value_or("").c_str());

  _webServer.begin();

  WiFi.begin(
    _config.getValueAsString("wifiSsid").value_or("").c_str(),
    _config.getValueAsString("wifiPassword").value_or("").c_str()
  );
}

void Network::exitConfiguredMode() {
  Serial.println(__FUNCTION__);

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_MODE_NULL);

  _webServer.stop();
}

void Network::enterNotConfiguredMode() {
}

void Network::exitNotConfiguredMode() {
}

bool Network::isWifiConfigured() const {
  for (auto key : {"wifiSsid", "wifiPassword"}) {
    auto value = _config.getValueAsString(key);
    if ((not value) or (value.value().empty())) {
      return false;
    }
  }

  return true;
}

bool Network::isWifiConnected() const {
  return (WiFi.status() == WL_CONNECTED);
}

void Network::onWifiConnect() {
  const auto ntpServer = _config.getValueAsString("ntpServer").value_or("");
  const auto tzInfo = _config.getValueAsString("tzInfo").value_or("");

  if (not ntpServer.empty()) {
    Serial.printf("Getting time from %s.\r\n", ntpServer.c_str());
    configTzTime(tzInfo.c_str(), ntpServer.c_str());

    // Call getLocalTime to trigger update of time. Unclear why this is required.
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
      return;
    }
    Serial.println(&timeinfo, "Time is %A, %B %d %Y %H:%M:%S.");
  }
}


long Network::getWifiRssi() const {
  return WiFi.RSSI();
}

void Network::onWebServerRoot() {
  if (not requestWebServerAuthentication()) {
    return;
  }

  _webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  _webServer.send(200, "text/html", html::header);
  _webServer.sendContent("<meta http-equiv='refresh' content='15'>");
  _webServer.sendContent(html::body);

  char* s  = nullptr;
  auto& measurement = _measurements->dataLast().getMeasurement(0);


  _webServer.sendContent("<div><table>");

  if ((asprintf(&s,"<tr><td>Co2</td><td>%5.0f ppm</td></tr>", measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Scd30Co2)]) != -1) and s) {
    _webServer.sendContent(s);
    free(s);
  }

  if ((asprintf(&s,"<tr><td>Temperature</td><td>%5.1f °C</td></tr>", measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Scd30Temperature)]) != -1) and s) {
    _webServer.sendContent(s);
    free(s);
  }

  if ((asprintf(&s,"<tr><td>Humidity</td><td>%5.1f %%</td></tr>", measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Scd30Humidity)]) != -1) and s) {
    _webServer.sendContent(s);
    free(s);
  }

  if ((asprintf(&s,"<tr><td>Pressure</td><td>%5.0f mBar</td></tr>", measurement.data[static_cast<std::underlying_type_t<Quantity>>(Quantity::Bmp280Pressure)]) != -1) and s) {
    _webServer.sendContent(s);
    free(s);
  }

// #if 1
//   struct tm timeinfo;
//   if(!getLocalTime(&timeinfo)){
//     Serial.println("Failed to obtain time");
//     return;
//   }
//   Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
// #endif

  _webServer.sendContent("</table></div>");

  _webServer.sendContent(html::footer);
  _webServer.client().stop();
}

bool Network::requestWebServerAuthentication() {
  if (_config.getValueAsBool("webAuthentification").value_or(false) and (_state != State::CONFIGURATION_MODE)) {
    if (!_webServer.authenticate(_config.getValueAsString("webUserName").value_or("").c_str(), _config.getValueAsString("webPassword").value_or("").c_str())) {
      _webServer.requestAuthentication(BASIC_AUTH, "Sensor Login", "Authentication failed");
      return false;
    }
  }
  return true;
}

void Network::onWebServerConfig() {
  if (not requestWebServerAuthentication()) {
    return;
  }

  _webServer.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  _webServer.sendHeader(F("Pragma"), F("no-cache"));
  _webServer.sendHeader(F("Expires"), F("0"));

  // Enable Pagination (Chunked Transfer)
  _webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);

  _webServer.send(200, "text/html", html::header);
  _webServer.sendContent(html::body);

  if (_webServer.method() == HTTP_GET) {
    _webServer.sendContent("<form method='POST' action='/config'>");

    for (const auto &entry : _config) {
      char* s  = nullptr;
      int ret = 0;

      _webServer.sendContent("<div>");

      if (auto* value = std::get_if<std::string>(&entry._value)) {
        ret = asprintf(&s,
          "<label for='%s'>%s</label>"
          "<input type='text' name='%s' id='%s' value='%s'/>"
          , entry._name, entry._name, entry._name, entry._name, value->c_str());
      } else if (auto* value = std::get_if<int>(&entry._value)) {
        ret = asprintf(&s,
          "<label for='%s'>%s</label>"
          "<input type='number' name='%s' id='%s' value='%i'/>"
          , entry._name, entry._name, entry._name, entry._name, *value);
      } else if (auto* value = std::get_if<bool>(&entry._value)) {
        ret = asprintf(&s,
          "<label for='%s'>%s</label>"
          "<input type='checkbox' name='%s' id='%s' value='1'%s/><input type='hidden' name='%s' value='0'/>"
          , entry._name, entry._name, entry._name, entry._name, *value ? " checked='checked'" : "", entry._name);
      }

      if ((ret != -1) and s) {
        _webServer.sendContent(s);
        free(s);
      }

      _webServer.sendContent("</div>");

    }

    _webServer.sendContent(
      "<div>"
      "<input type='submit' name='submit' value='Save' />"
      "</div>"
      "</form>"
    );
  } else {
    for (auto &entry : _config) {
      if (not _webServer.hasArg(entry._name)) {
        continue;
      }

      auto arg = _webServer.arg(entry._name);

      if (auto* value = std::get_if<std::string>(&entry._value)) {
        *value = std::string(arg.c_str());
      } else if (auto* value = std::get_if<int>(&entry._value)) {
        *value = arg.toInt();
      } else if (auto* value = std::get_if<bool>(&entry._value)) {
        *value = (arg == "1");
      }

    }
  }

  _webServer.sendContent(html::footer);
  _webServer.client().stop();

  if (_webServer.method() == HTTP_POST) {
    _restartCallback();
  }
}

