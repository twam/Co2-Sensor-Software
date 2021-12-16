#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstddef>
#include <array>
#include <variant>
#include <type_traits>
#include <string>
#include <functional>
#include <optional>

#include <ArduinoJson.h>
#include <tuple>

class ConfigEntry {
public:
  using ValueType = std::variant<std::string, int, bool>;

  ConfigEntry(const char* name, const ValueType& value) : _name{name}, _value{value} {};

  const char* _name;
  ValueType _value;
};

class Config {
public:
  using ValueType = ConfigEntry::ValueType;

  void setup();
  void finish();

  std::optional<ValueType> getValue(const char* name) const;
  void setValue(const char* name, const ValueType& value);

  std::optional<std::string> getValueAsString(const char* name) const;
  std::optional<int> getValueAsInt(const char* name) const;
  std::optional<bool> getValueAsBool(const char* name) const;

  auto begin() { return _entries.begin(); }
  auto end() { return _entries.end(); }

private:
  static constexpr std::size_t jsonBufferSize = 1000;
  static constexpr const char* configFileName = "/config.json";
  static constexpr const char* backupConfigFileName = "/config.json.backup";

  std::optional<std::reference_wrapper<ConfigEntry>> getEntry(const char* name);
  std::optional<std::reference_wrapper<const ConfigEntry>> getEntry(const char* name) const;

  // Entries _entries;

  void readFromFile();
  std::tuple<bool, DynamicJsonDocument> readJsonFromFile(const char* filename);

  void writeToFile();

  std::array<ConfigEntry, 10> _entries = {
    ConfigEntry{"wifiSsid", std::string{""}},
    ConfigEntry{"wifiPassword", std::string{""}},
    ConfigEntry("hostname", std::string{"Co2-Sensor"}),
    ConfigEntry{"sleepTimeout", int{60}},
    ConfigEntry{"ntpServer", std::string{"pool.ntp.org"}},
    ConfigEntry{"timeZoneOffset", int{3600}},
    ConfigEntry{"dstOffset", int{3600}},
    ConfigEntry{"webUserName", std::string{"admin"}},
    ConfigEntry{"webPassword", std::string{"password"}},
    ConfigEntry{"webAuthentification", bool{false}},
  };

};

#endif
