#ifndef NETWORK_HPP
#define NETWORK_HPP

#include "config.hpp"
#include "measurements.hpp"

#include <DNSServer.h>
#include <WebServer.h>

class Network {
public:
  using RestartCallback = std::function<void(void)>;

  enum class State {
    INITIAL,
    CONFIGURATION_MODE,
    CONFIGURED,
    NOT_CONFIGURED
  };

  Network(Config& config, const RestartCallback& restartCallback) : _config{config}, _restartCallback(restartCallback) {};

  void setup(const Measurements* measurements);
  void loop();

  long getWifiRssi() const;
  bool isWifiConnected() const;

  State getState() const { return _state; }

private:

  static constexpr const char* contentTypeHtmlUtf8 = "text/html; charset=utf-8";
  static constexpr const char* contentTypePlain = "text/plain";

  void setupWebserver();

  void gotoState(State state);

  bool isWifiConfigured() const;

  void enterConfigurationMode();
  void exitConfigurationMode();
  void enterConfiguredMode();
  void exitConfiguredMode();
  void enterNotConfiguredMode();
  void exitNotConfiguredMode();

  void onWebServerRoot();
  void onWebServerConfig();
  void onWebServerNotFound();

  void onWifiConnect();

  bool requestWebServerAuthentication();
  void sendHttpRedirect(const char* url);

  Config &_config;
  DNSServer _dnsServer{};
  WebServer _webServer{80};
  State _state{State::INITIAL};
  RestartCallback _restartCallback;
  const Measurements* _measurements{};
  bool _onConnectHandled{false};
};

#endif
