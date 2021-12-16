#include "config.hpp"

#include <SPIFFS.h>

void Config::setup() {
  readFromFile();
}

void Config::finish() {
  writeToFile();
}

void Config::readFromFile() {
  auto resultAndJson = readJsonFromFile(configFileName);

  if (not std::get<0>(resultAndJson)) {
    auto resultAndJson = readJsonFromFile(backupConfigFileName);
  }
  if (not std::get<0>(resultAndJson)) {
    return;
  }

  auto& json = std::get<1>(resultAndJson);

  for (auto& entry : _entries) {
    auto jsonEntry = json[entry._name];

    if (not jsonEntry.isNull()) {
      if (auto* value = std::get_if<std::string>(&entry._value)) {
        *value = jsonEntry.as<std::string>();
      } else if (auto* value = std::get_if<int>(&entry._value)) {
        *value = jsonEntry.as<int>();
      } else if (auto* value = std::get_if<bool>(&entry._value)) {
        *value = jsonEntry.as<bool>();
      } else {
        assert(false);
      }
    }
  }
}

std::tuple<bool, DynamicJsonDocument> Config::readJsonFromFile(const char* filename) {
  DynamicJsonDocument json(jsonBufferSize);

  auto f = SPIFFS.open(filename, "r");
  if (!f) {
    Serial.printf("Failed to read %s.\r\n", filename);
    return std::make_tuple(false, json);
  }

  DeserializationError err = deserializeJson(json, f);
  f.close();

  if (err != DeserializationError::Ok) {
    Serial.printf("Failed to parse JSON of %s: %s.\r\n", filename, err.c_str());
    return std::make_tuple(false, json);
  }

  return std::make_tuple(true, json);
}

void Config::writeToFile() {
  DynamicJsonDocument json(jsonBufferSize);

  for (auto& entry : _entries) {
    auto jsonEntry = json[entry._name];

    if (auto* value = std::get_if<std::string>(&entry._value)) {
      jsonEntry.set(*value);
    } else if (auto* value = std::get_if<int>(&entry._value)) {
      jsonEntry.set(*value);
    } else if (auto* value = std::get_if<bool>(&entry._value)) {
      jsonEntry.set(*value);
    } else {
      assert(false);
    }

  }

  SPIFFS.remove(backupConfigFileName);
  SPIFFS.rename(configFileName, backupConfigFileName);

  auto f = SPIFFS.open(configFileName, "w");
  if (f) {
    serializeJson(json, f);
    f.close();
  }
}

std::optional<Config::ValueType> Config::getValue(const char* name) const
{
  const auto& entry = getEntry(name);
  if (entry) {
    return entry.value().get()._value;
  } else {
    return std::nullopt;
  }
}

std::optional<std::string> Config::getValueAsString(const char* name) const
{
  auto valueVariant = getValue(name);

  if (valueVariant) {
    if (auto* value = std::get_if<std::string>(&valueVariant.value())) {
      return *value;
    }
  }

  return std::nullopt;
}


std::optional<int> Config::getValueAsInt(const char* name) const
{
  auto valueVariant = getValue(name);

  if (valueVariant) {
    if (auto* value = std::get_if<int>(&valueVariant.value())) {
      return *value;
    }
  }

  return std::nullopt;
}

std::optional<bool> Config::getValueAsBool(const char* name) const
{
  auto valueVariant = getValue(name);

  if (valueVariant) {
    if (auto* value = std::get_if<bool>(&valueVariant.value())) {
      return *value;
    }
  }

  return std::nullopt;
}

void Config::setValue(const char* name, const ValueType& value)
{
  const auto& entry = getEntry(name);
  if (entry) {
    entry.value().get()._value = value;
  }
}

std::optional<std::reference_wrapper<ConfigEntry>> Config::getEntry(const char* name)
{
  auto entry = std::find_if(std::begin(_entries), std::end(_entries), [name](const auto& v) {
    return strcmp(v._name, name) == 0;
  });

  if (entry != std::end(_entries)) {
    return *entry;
  } else {
    return std::nullopt;
  }
}

std::optional<std::reference_wrapper<const ConfigEntry>> Config::getEntry(const char* name) const {
  auto entry = std::find_if(std::begin(_entries), std::end(_entries), [name](const auto& v) {
    return strcmp(v._name, name) == 0;
  });

  if (entry != std::end(_entries)) {
    return *entry;
  } else {
    return std::nullopt;
  }
}
