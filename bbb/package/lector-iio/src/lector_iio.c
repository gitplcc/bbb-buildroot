#include <iio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int main(void) {
    printf("===============================================\n");
    printf(" Iniciando aplicación Lector IIO en BeagleBone \n");
    printf("===============================================\n");

    // Create a local IIO context
    struct iio_context *ctx;
    ctx = iio_create_local_context();
    if (!ctx) {
        char err_str[256];
        iio_strerror(errno, err_str, sizeof(err_str));
        fprintf(stderr, "Error al crear el contexto local: %s\n", err_str);
        return EXIT_FAILURE;
    }

    // Find the BME280 device
    struct iio_device *dev;
    dev = iio_context_find_device(ctx, "bme280");
    if (!dev) {
        fprintf(stderr, "Dispositivo 'bme280' no encontrado.\n");
        iio_context_destroy(ctx);
        return EXIT_FAILURE;
    }

    printf("Dispositivo encontrado:\n");
    printf("  - ID: %s\n", iio_device_get_id(dev));
    printf("  - Nombre: %s\n", iio_device_get_name(dev));
    printf("  - Número de canales: %u\n", iio_device_get_channels_count(dev));

    struct iio_channel *ch_temp, *ch_press, *ch_hum;
    ch_temp  = iio_device_find_channel(dev, "temp", false);
    ch_press = iio_device_find_channel(dev, "pressure", false);
    ch_hum   = iio_device_find_channel(dev, "humidityrelative", false);
    if (!ch_temp || !ch_press || !ch_hum) {
        fprintf(stderr, "Error al encontrar los canales del dispositivo.\n");
        iio_context_destroy(ctx);
        return EXIT_FAILURE;
    }

    while (1) {
        double temp_raw, press_raw, hum_raw;
        if (iio_channel_attr_read_double(ch_temp, "input", &temp_raw) == 0) {
            printf("Temperatura: %.2f °C\n", temp_raw / 1000.0);
        } else {
            fprintf(stderr, "Error al leer temperatura.\n");
        }

        if (iio_channel_attr_read_double(ch_press, "input", &press_raw) == 0) {
            printf("Presión:     %.2f hPa\n", press_raw * 10.0);
        } else {
            fprintf(stderr, "Error al leer presión.\n");
        }

        if (iio_channel_attr_read_double(ch_hum, "input", &hum_raw) == 0) {
            printf("Humedad:     %.2f %%\n", hum_raw / 1000.0);
        } else {
            fprintf(stderr, "Error al leer humedad.\n");
        }

        sleep(2);
    }

    iio_context_destroy(ctx);
    return EXIT_SUCCESS;
}
