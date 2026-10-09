cmake_minimum_required(VERSION 3.20.0)

# The SDK takes the ingest key either from Kconfig or from the application.
# With credentials.conf the key is set at build time, as in the other examples.
# Without it the firmware asks for the key at run time (see src/virtual_device.c).
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/credentials.conf")
    list(APPEND EXTRA_CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/credentials.conf")
else()
    list(APPEND EXTRA_CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/runtime-credentials.conf")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zephyr_virtual_device)

add_subdirectory(src/net ${CMAKE_CURRENT_BINARY_DIR}/src/net)

target_sources(app PRIVATE
        src/main.c
        src/sensor_node.c
        src/virtual_device.c
)
