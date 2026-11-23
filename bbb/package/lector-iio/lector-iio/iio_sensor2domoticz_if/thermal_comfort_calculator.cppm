module;

#include "fl/Headers.h"

#include <cmath>
#include <ctime>
#include <iostream>
#include <numbers>
#include <string>


export module iio_sensor2domoticz_if:thermal_comfort_calculator;

import iio_sensor2domoticz_if.config;

namespace IIOSensor2DomoticzIf {

class ThermalComfortCalculator {};

using namespace fl;

// 1. Preprocesamiento: Calcula el índice estacional [-1, 1] desde fecha y latitud
double calcularIndiceEstacional(int anio, int mes, int dia, double latitud) {
    std::tm time_in = {0, 0, 0, dia, mes - 1, anio - 1900};
    std::time_t time_temp = std::mktime(&time_in);
    const std::tm* time_out = std::localtime(&time_temp);

    int dia_anio = time_out->tm_yday + 1;  // 1 a 365/366

    // Declinación solar en radianes
    double declinacion_deg =
        23.45 * std::sin((2.0 * std::numbers::pi / 365.0) * (284.0 + dia_anio));
    double declinacion_rad = declinacion_deg * (std::numbers::pi / 180.0);
    double latitud_rad = latitud * (std::numbers::pi / 180.0);

    return std::sin(latitud_rad) * std::sin(declinacion_rad);
}

// 2. Conversión a código oficial de confort de Domoticz (0, 1, 2, 3)
int obtenerCodigoDomoticz(double indiceConfort, double humedad, double temperatura) {
    if (indiceConfort >= 75.0) {
        return 1;  // 1 = Comfortable (Óptimo)
    } else if (indiceConfort >= 50.0) {
        return 0;  // 0 = Normal
    } else {
        // Si el confort es bajo (<50), Domoticz distingue si es por sequedad o humedad/bochorno
        if (humedad < 40.0) {
            return 2;  // 2 = Dry (Seco)
        } else {
            return 3;  // 3 = Wet (Húmedo / Inconfortable)
        }
    }
}

int main() {
    try {
        // Cargar el motor borroso desde el archivo .fll
        Engine* engine = FllImporter().fromFile("confort.fll");

        std::string status;
        if (!engine->isReady(&status)) {
            std::cerr << "Error en el motor FuzzyLite: " << status << std::endl;
            return 1;
        }

        // Obtener punteros a las variables
        InputVariable* tempInput = engine->getInputVariable("Temperatura");
        InputVariable* humInput = engine->getInputVariable("Humedad");
        InputVariable* presInput = engine->getInputVariable("Presion");
        InputVariable* estInput = engine->getInputVariable("Estacion");
        OutputVariable* confortOutput = engine->getOutputVariable("Confort");

        // Datos de ejemplo (Madrid, 23 de noviembre de 2026)
        double tempActual = 20.5;
        double humActual = 50.0;
        double presActual = 1015.0;
        double latitud = 40.4168;
        double estacion = calcularIndiceEstacional(2026, 11, 23, latitud);

        // Asignar entradas
        tempInput->setValue(tempActual);
        humInput->setValue(humActual);
        presInput->setValue(presActual);
        estInput->setValue(estacion);

        // Procesar inferencia borrosa
        engine->process();

        // Obtener resultados
        double porcentajeConfort = confortOutput->getValue();
        int codigoDomoticz = obtenerCodigoDomoticz(porcentajeConfort, humActual, tempActual);

        // Mostrar resultados y formato URL para Domoticz API
        std::cout << "Indice Estacional: " << estacion << std::endl;
        std::cout << "Confort Calculado: " << porcentajeConfort << "%" << std::endl;
        std::cout << "Codigo Domoticz (HUM_STAT): " << codigoDomoticz << std::endl;

        // Ejemplo de payload para actualizar un sensor Temp+Hum+Baro (IDX = 123) en Domoticz
        std::cout << "\nURL API Domoticz:\n"
                  << "/json.htm?type=command&param=udevice&idx=123&nvalue=0&svalue=" << tempActual
                  << ";" << humActual << ";" << codigoDomoticz << ";" << presActual << ";0"
                  << std::endl;

        delete engine;
    } catch (const std::exception& e) {
        std::cerr << "Excepcion: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
}  // namespace IIOSensor2DomoticzIf
