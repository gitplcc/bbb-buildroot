#include <CLI/CLI.hpp>
#include <mqtt/async_client.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/syslog_sink.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// Try to use libfmt when using uClibc-ng as C library
#if defined(USE_LIBFMT) || (defined(__UCLIBC__) && __has_include(<fmt/core.h>))
#    include <fmt/chrono.h>
#    include <fmt/core.h>
#    include <fmt/format.h>
namespace fmt_ns = fmt;
#elif __has_include(<format>)
#    include <format>
#    if defined(__cpp_lib_format) && (__cpp_lib_format >= 201907L)
namespace fmt_ns = std;
#    else
#        error "Standard <format> header found, but __cpp_lib_format is unsupported."
#    endif
#else
#    error "Neither libfmt nor C++20 <format> is available. Enable BR2_PACKAGE_FMT=y on uClibc-ng."
#endif

import iio_sensor2domoticz_if.config;
import iio_sensor2domoticz_if;

using namespace IIOSensor2DomoticzIf;

constexpr auto StatusUpdateInterval = std::chrono::seconds(1);
constexpr auto SensorSamplingInterval = std::chrono::seconds(15);

// Utility struct for RAII based spdlog shutdown
struct SpdlogGuard {
    ~SpdlogGuard() {
        spdlog::shutdown();
    }
};

// Main loop control
static std::atomic<bool> Running{true};

// Handle termination signals
void signalHandler(int) {
    Running = false;
}

int calculateHumidityStatus(const IIOSensor2DomoticzIf::SensorValues& sample) {
    // Steam saturation pressure [hPa]
    auto es = 6.112 * std::exp((17.67 * sample.temperature) / (sample.temperature + 243.5));

    // Atmospheric presssure weighting factor
    auto fw = 1 + 0.0007 + (3.46e-6 * sample.pressure);

    // Real partial vapor pressure [hPa]
    auto e = (sample.humidity / 100.0) * es * fw;

    // 4. Apparent temperature of Steadman [ºC]
    auto ta = sample.temperature + (0.33 * e) - 4.00;

    // Encode humidity status for Domoticz
    //  0: Normal, 1: Comfortable, 2: Dry, 3: Wet
    int humStatus{0};
    if (sample.humidity <= 30)
        humStatus = 2;
    else if (sample.humidity >= 70 || ta > 27.0)
        humStatus = 3;
    else if (sample.humidity >= 40 && sample.humidity <= 60 && ta >= 20.0 && ta <= 26.0)
        humStatus = 1;

    return humStatus;
}

[[nodiscard]] auto getMqttIf(const std::string& srv_address, const std::string& client_id) noexcept
    -> std::expected<mqtt::async_client::ptr_t, int> try {
    using namespace std::chrono_literals;

    // Create the MQTT client with custom create options
    auto createOpts = mqtt::create_options_builder()
                          .send_while_disconnected(true, true)
                          .max_buffered_messages(60)
                          .delete_oldest_messages(true)
                          .finalize();
    auto client{std::make_shared<mqtt::async_client>(srv_address, client_id, createOpts)};

    // Connect to the MQTT server with custom connect options
    auto connOpts = mqtt::connect_options_builder()
                        .clean_session(true)
                        .keep_alive_interval(20s)
                        .connect_timeout(10s)
                        .automatic_reconnect(1s, 30s)
                        // Uncomment if your local Mosquitto requires credentials:
                        // .user_name("domoticz_user")
                        // .password("your_password")
                        .finalize();
    client->connect(connOpts)->wait();

    return client;
} catch (const mqtt::exception& e) {
    // std::cerr << "MQTT error: " << e.what() << std::endl;
    return std::unexpected(e.get_reason_code());
}

/* FIXME: should be in iio_sensor2domoticz_if.app_config but moved here as CLI11
 *   causes GCC 14.3 ICE
 */
[[nodiscard]] auto parseConfig(int argc, char* argv[]) noexcept
    -> std::expected<AppConfig, ConfigError> {
    CLI::App App{"Local I2C-connected bme280 to Domoticz daemon", std::string{Config::Name}};
    try {
        AppConfig Cfg;
        /* TODO: Add options to set:
         * - [ ] MQTT publisher id (Config::PublisherClientId)
         * - [ ] MQTT broker URL (Config::ServerAddress)
         * - [ ] IIO device id (Config::IioDevId)
         * - [ ] MQTT topic (Config::ServerTopic)
         * - [ ] Validation for --idx (positive number)
         */
        App.add_option("-i,--idx", Cfg.Idx, "Domoticz device id")
            ->check(CLI::PositiveNumber)
            ->envname("LECTOR_IIO_DEV_ID")
            ->required();
        App.add_option("-a,--altitude", Cfg.Altitude, "Site altitude in meters in seconds")
            ->check(CLI::PositiveNumber)
            ->envname("LECTOR_IIO_ALTITUDE");
        App.add_flag("-v,--verbose", Cfg.Verbose, "Enable debug logging")
            ->envname("LECTOR_IIO_VERBOSE");
        // app.add_flag("-s,--syslog", Config.use_syslog, "Send logs to syslog instead of
        // stderr");
        App.set_config(
            "-c,--config", std::string{Config::ConfigPath}, "Configuration file path", false);
        App.parse(argc, argv);
        return Cfg;
    } catch (const CLI::ParseError& e) {
        int exit_code = App.exit(e);  // Handles --help, --version & syntax errors cleanly
        return std::unexpected(ConfigError::ParseError);
    }
}

int main(int argc, char* argv[]) {
    using namespace IIOSensor2DomoticzIf;
    using namespace std::literals;
    using namespace std::chrono;
    namespace fs = std::filesystem;

    SpdlogGuard LogGuard;  // Guarantees spdlog gracefully shutdown on scope exit

    auto SyslogSink{std::make_shared<spdlog::sinks::syslog_sink_mt>(
        std::string{Config::Name}, LOG_PID, LOG_DAEMON, true)};
    SyslogSink->set_pattern(R"({"desc":"%v","module":"%n","file":"%s","line":%#})");
    SyslogSink->set_level(spdlog::level::info);
    std::vector<spdlog::sink_ptr> Sinks{SyslogSink};

    auto Config{parseConfig(argc, argv)};
    if (!Config)
        return EXIT_FAILURE;

    if ((*Config).Verbose) {
        auto ConsoleSink{std::make_shared<spdlog::sinks::stdout_color_sink_mt>()};
        ConsoleSink->set_level(spdlog::level::debug);
        Sinks.push_back(ConsoleSink);
    }
    auto Logger = std::make_shared<spdlog::logger>("main", Sinks.begin(), Sinks.end());
    // logger->set_level(spdlog::level::debug);

    spdlog::set_default_logger(Logger);

    SPDLOG_INFO("Starting service.");

    auto Publisher = getMqttIf((*Config).BrokerUrl, (*Config).ClientId);
    if (!Publisher)
        return Publisher.error();

    auto Sensor{SensorInterface::create((*Config).IioDevId)};
    if (!Sensor)
        return EXIT_FAILURE;

    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);

    try {
        BarometricForecaster WeatherForecaster{(*Config).Altitude};
        auto NxtStatusUpdateTime{steady_clock::now()};
        auto NxtSensorSampligTime{NxtStatusUpdateTime};
        while (Running) {
            if (NxtSensorSampligTime <= NxtStatusUpdateTime) {
                auto Sample = Sensor->readValues();
                if (Sample) {
                    auto [Temp, Hum, Press] = *Sample;
                    WeatherForecaster.addReading(NxtSensorSampligTime, Press, Temp);
                    // std::string result = std::apply([](const auto&... args) {
                    //     return std::format("ID: {}, Temp: {:.1f}, Name: {}", args...);
                    // }, t);
                    auto JsonStr = fmt_ns::format(
                        R"({{"idx": {}, "nvalue": 0, "svalue": "{:.2f};{:.2f};{};{:.2f};{}"}})",
                        (*Config).Idx,
                        Temp,
                        Hum,
                        calculateHumidityStatus(*Sample),
                        Press,
                        WeatherForecaster.getForecast());
                    (*Publisher)->publish((*Config).BrokerTopic, JsonStr, 1, false);
                    if ((*Config).Verbose)
                        std::cout << fmt_ns::format(
                                         R"([{}]: {})", std::chrono::system_clock::now(), JsonStr)
                                  << std::endl;
                }
                NxtSensorSampligTime += SensorSamplingInterval;
            }
            NxtStatusUpdateTime += StatusUpdateInterval;
            std::this_thread::sleep_until(NxtStatusUpdateTime);
        }
        SPDLOG_INFO("Shutting down service.");
        return EXIT_SUCCESS;
    } catch (const mqtt::exception& e) {
        SPDLOG_CRITICAL("MQTT error: {}", e.what());
        return e.get_reason_code();
    } catch (const SensorInterfaceError& e) {
        SPDLOG_CRITICAL("Sensor error: {}", e.what());
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        SPDLOG_CRITICAL("libstd++ unexpected exception: {}", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        SPDLOG_CRITICAL("Unknown non-standard exception occurred");
        return EXIT_FAILURE;
    }
    std::unreachable();
}
