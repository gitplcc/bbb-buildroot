cmake_minimum_required(VERSION 3.0)
project(lector_iio C)

# Find libiio using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBIIO REQUIRED IMPORTED_TARGET libiio)

add_executable(lector_iio lector_iio.c)
target_link_libraries(lector_iio PRIVATE PkgConfig::LIBIIO)

include(GNUInstallDirs)
install(TARGETS lector_iio DESTINATION ${CMAKE_INSTALL_BINDIR})
