cmake_minimum_required(VERSION 3.0)
project(lector_iio C)

add_executable(lector_iio lector_iio.c)

# Buscar libiio en el entorno de compilación cruzada de Buildroot
find_library(IIO_LIB iio)
if(IIO_LIB)
    target_link_libraries(lector_iio ${IIO_LIB})
else()
    message(FATAL_ERROR "¡No se encontró libiio!")
endif()

include(GNUInstallDirs)
install(TARGETS lector_iio DESTINATION ${CMAKE_INSTALL_BINDIR})

