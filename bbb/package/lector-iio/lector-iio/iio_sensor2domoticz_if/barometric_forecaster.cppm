module;

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>

export module iio_sensor2domoticz_if:barometric_forecaster;

namespace IIOSensor2DomoticzIf {

export class BarometricForecaster {
public:
    using TimeStamp = std::chrono::steady_clock::time_point;

    static constexpr std::chrono::seconds ForecastSamplingInterval = std::chrono::minutes{10};

    explicit BarometricForecaster(double station_altitude_meters)
        : altitude_m(station_altitude_meters) {}

    void addReading(TimeStamp timestamp, double raw_pressure_hpa, double temp_c) {
        if (timestamp >= NxtUpdateTime_) {
            double p0 = toSeaLevel(raw_pressure_hpa, temp_c);
            history[head] = p0;
            head = (head + 1) % BUFFER_SIZE;
            if (count < BUFFER_SIZE)
                count++;
            NxtUpdateTime_ = timestamp + ForecastSamplingInterval;
        }
    }

    // 0: Stable (Estable)
    // 1: Sunny (Soleado)
    // 2: Cloudy (Nublado)
    // 3: Unstable (Inestable)
    // 4: Thunderstorm (Tormenta)
    // 5: Unknown (Desconocido)
    // 6: Cloudy/Rain (Nublado / Lluvia)
    int getForecast() const {
        int forecast_code{5};  // Default to Unknown
        if (count < BUFFER_SIZE) {
            return forecast_code;
        }

        size_t newest_idx = (head + BUFFER_SIZE - 1) % BUFFER_SIZE;
        size_t oldest_idx = head;
        auto current_p0 = history[newest_idx];
        auto delta = current_p0 - history[oldest_idx];

        if (delta > 2.0f) {
            forecast_code = 1;  // 1: Rapidly clearing
        } else if (delta >= 0.5f) {
            forecast_code = (current_p0 > 1013.0f) ? 1 : 0;  // 1: Sunny, 0: Stable
        } else if (delta > -0.5f) {
            forecast_code = (current_p0 > 1013.0f) ? 0 : 3;  // 0: Stable, 3: Unstable
        } else if (delta >= -2.0f) {
            forecast_code = (current_p0 > 1013.0f) ? 2 : 6;  // 2: Cloudy, 6: Rain
        } else {
            forecast_code = 4;  // 4: Thunderstorm
        }
        return forecast_code;
    }

private:
    double toSeaLevel(double pressure_hpa, double temp_c) const {
        double lapse = 0.0065 * altitude_m;
        return pressure_hpa * std::pow(1.0f - (lapse / (temp_c + lapse + 273.15f)), -5.257f);
    }

    static constexpr size_t BUFFER_SIZE = 18;
    std::array<double, BUFFER_SIZE> history{};
    size_t head = 0;
    size_t count = 0;
    double altitude_m;
    TimeStamp NxtUpdateTime_{TimeStamp::min()};
};  // class BarometricForecaster

}  // namespace IIOSensor2DomoticzIf
