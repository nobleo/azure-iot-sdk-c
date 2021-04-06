#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_mqtt})
    message(FATAL_ERROR "iothub_client_device_twin_and_methods_cbor_sample being generated without MQTT support")
endif()

compileAsC99()

# Sample Executable
set(iothub_client_device_twin_and_methods_cbor_sample_c_files
    iothub_client_device_twin_and_methods_cbor_sample.c
)

if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    set(iothub_client_device_twin_and_methods_cbor_sample_c_files ${iothub_client_device_twin_and_methods_cbor_sample_c_files}
        ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

add_executable(iothub_client_device_twin_and_methods_cbor_sample
    ${iothub_client_device_twin_and_methods_cbor_sample_c_files}
)

target_link_libraries(iothub_client_device_twin_and_methods_cbor_sample
    iothub_client
)

# Tinycbor Library
find_library(tinycbor tinycbor)
target_link_libraries(iothub_client_device_twin_and_methods_cbor_sample tinycbor)
