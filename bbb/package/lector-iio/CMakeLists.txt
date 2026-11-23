cmake_minimum_required(VERSION 3.30...4.4)

project(lector-iio VERSION 1.0.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(CPM)

include(GNUInstallDirs)

CPMAddPackage("gh:fuzzylite/fuzzylite#fe62b61")

set(DAEMON_NAME ${CMAKE_PROJECT_NAME})
add_subdirectory(lector-iio)

set(DAEMON_CONFIG_DIR "${CMAKE_INSTALL_SYSCONFDIR}/${DAEMON_NAME}")
set(DAEMON_CONFIG_PATH "${DAEMON_CONFIG_DIR}/${DAEMON_NAME}.conf")
install(FILES conf/${DAEMON_NAME}.conf DESTINATION ${DAEMON_CONFIG_DIR})

configure_file(
    lector-iio/iio_sensor2domoticz_if/config/config.cppm.in
    generated/config.cppm
    @ONLY
)
