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

private:

  void setupWebserver();

  void gotoState(State state);

  bool isWifiConfigured();
  bool isWifiConnected();

  void enterConfigurationMode();
  void exitConfigurationMode();
  void enterConfiguredMode();
  void exitConfiguredMode();
  void enterNotConfiguredMode();
  void exitNotConfiguredMode();

  void onWebServerRoot();
  void onWebServerConfig();

  void onWifiConnect();

  bool requestWebServerAuthentication();

  Config &_config;
  DNSServer _dnsServer{};
  WebServer _webServer{80};
  State _state{State::INITIAL};
  RestartCallback _restartCallback;
  const Measurements* _measurements{};
  bool _onConnectHandled{false};
};

#endif
