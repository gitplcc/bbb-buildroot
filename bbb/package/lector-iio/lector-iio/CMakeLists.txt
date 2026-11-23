find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBIIO REQUIRED IMPORTED_TARGET libiio)
find_package(CLI11 REQUIRED)
find_package(PahoMqttCpp REQUIRED)
find_package(fmt QUIET) # Formatting library as a fall-back for std::format
find_package(spdlog REQUIRED)

add_executable(${DAEMON_NAME})
target_sources(${DAEMON_NAME}
    PRIVATE
        lector_iio.cpp
    PRIVATE
        FILE_SET modules
            TYPE CXX_MODULES
            BASE_DIRS
                iio_sensor2domoticz_if
                "${PROJECT_BINARY_DIR}/generated"
            FILES
                iio_sensor2domoticz_if/iio_sensor2domoticz_if.cppm
                iio_sensor2domoticz_if/app_config.cppm
                iio_sensor2domoticz_if/barometric_forecaster.cppm
                iio_sensor2domoticz_if/sensor_interface.cppm
                iio_sensor2domoticz_if/thermal_comfort_calculator.cppm
                "${PROJECT_BINARY_DIR}/generated/config.cppm"
)
target_compile_features(${DAEMON_NAME} PUBLIC cxx_std_23)
set_target_properties(${DAEMON_NAME} PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(${DAEMON_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${DAEMON_NAME}
    PRIVATE
        CLI11::CLI11
        PkgConfig::LIBIIO
        PahoMqttCpp::paho-mqttpp3
        spdlog::spdlog
)
# target_compile_definitions(${DAEMON_NAME}
#     PRIVATE
#         $<$<CONFIG:Release>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>
# )
if(fmt_FOUND)
    target_link_libraries(${DAEMON_NAME} PRIVATE fmt::fmt)
    target_compile_definitions(${DAEMON_NAME} PRIVATE USE_LIBFMT=1)
endif()

install(TARGETS ${DAEMON_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
