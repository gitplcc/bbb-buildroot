#include <stdio.h>
#include <unistd.h>

int main(void) {
    printf("==========================================\n");
    printf(" Iniciando aplicación Lector IIO en BeagleBone\n");
    printf("==========================================\n");
    
    while(1) {
        // Aquí meterías la lectura real del archivo /sys/bus/iio/devices/...
        printf("[INFO] Leyendo canal ADC: 2048 raw\n");
        sleep(2);
    }
    return 0;
}

