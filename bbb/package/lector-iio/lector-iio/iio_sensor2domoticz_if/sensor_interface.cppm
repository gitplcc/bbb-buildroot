module;

#include <expected>
#include <stdexcept>
#include <string>
#include <utility>

#include <iio.h>  // Must after system includes

export module iio_sensor2domoticz_if:sensor_interface;

namespace IIOSensor2DomoticzIf {

export class SensorInterfaceError : public std::runtime_error {
public:
    enum class ErrorCode { CantCreateCtx, DevNotFound, ChNotFound, ReadError, UnknownError };

    explicit SensorInterfaceError(ErrorCode ec)
        : std::runtime_error(message(ec))
        , error_code_(ec) {}

private:
    static const char* message(ErrorCode ec) {
        switch (ec) {
            case ErrorCode::CantCreateCtx:
                return "Error al crear el contexto local.";
            case ErrorCode::DevNotFound:
                return "Dispositivo no encontrado.";
            case ErrorCode::ChNotFound:
                return "Canal no encontrado.";
            case ErrorCode::ReadError:
                return "Error al leer los valores del sensor.";
            default:
                return "Error desconocido.";
        }
        /*
                switch (sensor.error()) {
            case SensorInterface::Error::CantCreateCtx:
                char err_str[256];
                iio_strerror(errno, err_str, sizeof(err_str));
                std::cerr << "Error al crear el contexto local: " << err_str << std::endl;
                break;
            case SensorInterface::Error::DevNotFound:
                std::cerr << "Dispositivo '" << "bme280" << "' no encontrado." << std::endl;
                break;
            case SensorInterface::Error::ChNotFound:
                throw SensorInterfaceError("Error al crear la interfaz del sensor.");
                break;
        */
    }

    ErrorCode error_code_;
};

export struct SensorValues {
    double temperature;
    double humidity;
    double pressure;
};

export class SensorInterface {
public:
    // Factory creation avoids exceptions and two-stage init
    [[nodiscard]] static auto create(const std::string& name)
        -> std::expected<SensorInterface, SensorInterfaceError::ErrorCode> {
        using enum SensorInterfaceError::ErrorCode;

        SensorInterfaceError::ErrorCode retcode;
        struct iio_context* ctx = nullptr;
        struct iio_device* dev = nullptr;
        struct iio_channel* ch_temp = nullptr;
        struct iio_channel* ch_press = nullptr;
        struct iio_channel* ch_hum = nullptr;

        ctx = iio_create_local_context();
        if (!ctx) {
            retcode = CantCreateCtx;
            goto failure;
        }

        dev = iio_context_find_device(ctx, name.c_str());
        if (!dev) {
            retcode = DevNotFound;
            goto failure;
        }

        ch_temp = iio_device_find_channel(dev, "temp", false);
        ch_press = iio_device_find_channel(dev, "pressure", false);
        ch_hum = iio_device_find_channel(dev, "humidityrelative", false);
        if (!ch_temp || !ch_press || !ch_hum) {
            retcode = ChNotFound;
            goto failure;
        }

        return SensorInterface(ctx, dev, ch_temp, ch_press, ch_hum);

    failure:
        if (ctx)
            iio_context_destroy(ctx);
        return std::unexpected(retcode);
    }

    // Delete copy constructor and copy assignment operator to prevent copying
    SensorInterface(const SensorInterface&) = delete;
    SensorInterface& operator=(const SensorInterface&) = delete;
    // Move constructor and move assignment operator
    SensorInterface(SensorInterface&& other) noexcept
        : ctx_(std::exchange(other.ctx_, nullptr))
        , dev_(std::exchange(other.dev_, nullptr))
        , ch_temp_(std::exchange(other.ch_temp_, nullptr))
        , ch_press_(std::exchange(other.ch_press_, nullptr))
        , ch_hum_(std::exchange(other.ch_hum_, nullptr)) {}

    SensorInterface& operator=(SensorInterface&& other) noexcept {
        if (this != &other) {
            release();
            ctx_ = std::exchange(other.ctx_, nullptr);
            dev_ = std::exchange(other.dev_, nullptr);
            ch_temp_ = std::exchange(other.ch_temp_, nullptr);
            ch_press_ = std::exchange(other.ch_press_, nullptr);
            ch_hum_ = std::exchange(other.ch_hum_, nullptr);
        }
        return *this;
    }

    // Deterministic RAII cleanup of the IIO context
    ~SensorInterface() {
        release();
    }

    std::expected<SensorValues, SensorInterfaceError::ErrorCode> readValues() {
        double temp_raw, press_raw, hum_raw;
        if (iio_channel_attr_read_double(ch_temp_, "input", &temp_raw) != 0 ||
            iio_channel_attr_read_double(ch_hum_, "input", &hum_raw) != 0 ||
            iio_channel_attr_read_double(ch_press_, "input", &press_raw) != 0)
            return std::unexpected(SensorInterfaceError::ErrorCode::ReadError);

        return SensorValues{
            temperature: temp_raw / 1000.0,
            humidity: hum_raw / 1000.0,
            pressure: press_raw * 10.0
        };
    }

private:
    explicit SensorInterface(struct iio_context* ctx,
                             struct iio_device* dev,
                             struct iio_channel* ch_temp,
                             struct iio_channel* ch_press,
                             struct iio_channel* ch_hum) noexcept
        : ctx_(ctx)
        , dev_(dev)
        , ch_temp_(ch_temp)
        , ch_press_(ch_press)
        , ch_hum_(ch_hum) {}

    void release() noexcept {
        if (ctx_) {
            iio_context_destroy(ctx_);
            ctx_ = nullptr;
        }
    }

    struct iio_context* ctx_;
    struct iio_device* dev_;
    struct iio_channel *ch_temp_, *ch_press_, *ch_hum_;
};  // class SensorInterface

}  // namespace IIOSensor2DomoticzIf
