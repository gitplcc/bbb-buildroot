module;

/* #include <chrono> */
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

export module iio_sensor2domoticz_if:app_config;

import iio_sensor2domoticz_if.config;

namespace IIOSensor2DomoticzIf {

export struct AppConfig {
    double Altitude{690.0};
    std::string BrokerUrl{std::string{Config::BrokerUrl}};
    std::string BrokerTopic{std::string{Config::BrokerTopic}};
    std::string ClientId{std::string{Config::PublisherClientId}};
    int Idx{1};
    std::string IioDevId{std::string{Config::IioDevId}};
    // std::chrono::seconds SamplingInterval{std::chrono::seconds{15}};
    bool Verbose{false};
};

export enum class ConfigError { FileNotFound, ParseError };

}  // namespace IIOSensor2DomoticzIf
