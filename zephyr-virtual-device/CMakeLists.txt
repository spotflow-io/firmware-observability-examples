cmake_minimum_required(VERSION 3.20.0)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/credentials.conf")
    list(APPEND EXTRA_CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/credentials.conf")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zephyr_virtual_device)

add_subdirectory(src/net ${CMAKE_CURRENT_BINARY_DIR}/src/net)

target_sources(app PRIVATE
        src/main.c
        src/sensor_node.c
        src/virtual_device.c
)

# Passing the ingest key at run time needs the spotflow_override_ingest_key()
# hook in the device SDK. Without it the firmware still works, but only with
# CONFIG_SPOTFLOW_INGEST_KEY from credentials.conf.
if (EXISTS "${ZEPHYR_SPOTFLOW_MODULE_DIR}/zephyr/include/spotflow/ingest_key.h")
    target_compile_definitions(app PRIVATE VIRTUAL_DEVICE_SDK_HAS_INGEST_KEY_HOOK=1)
else()
    message(WARNING "This Spotflow device SDK revision has no spotflow_override_ingest_key() hook. "
                    "The ingest key can only be set at build time in credentials.conf.")
endif()
