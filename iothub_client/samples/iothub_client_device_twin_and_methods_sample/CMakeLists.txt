#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_client_device_twin_and_methods_sample

if(NOT ${use_mqtt} AND NOT ${use_amqp})
    message(FATAL_ERROR "iothub_client_device_twin_and_methods_sample being generated without amqp or mqtt support")
endif()

compileAsC99()

# Compiler Defintions
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
endif(WIN32)
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
endif()
if(${use_amqp})
    add_definitions(-DUSE_AMQP)
endif()
if(${use_mqtt})
    add_definitions(-DUSE_MQTT)
endif()

# Executable
set(iothub_client_device_twin_and_methods_sample_c_files
    iothub_client_device_twin_and_methods_sample.c
)
if(${use_sample_trusted_cert})
    set(iothub_client_device_twin_and_methods_sample_c_files ${iothub_client_device_twin_and_methods_sample_c_files} ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()
set(iothub_client_device_twin_and_methods_sample_h_files
)

add_executable(iothub_client_device_twin_and_methods_sample ${iothub_client_device_twin_and_methods_sample_c_files} ${iothub_client_device_twin_and_methods_sample_h_files})
include_directories(.)

# C-utility and iothub_client -- Compiled and Included at a higher level
target_link_libraries(iothub_client_device_twin_and_methods_sample
    aziotsharedutil
    iothub_client
)

# AMQP Library -- Compiled and Included at a higher level
if(${use_amqp})
    target_link_libraries(iothub_client_device_twin_and_methods_sample
        iothub_client_amqp_transport
        uamqp
    )
endif()

# MQTT Library -- Compiled and Included at a higher level
if(${use_mqtt})
    target_link_libraries(iothub_client_device_twin_and_methods_sample
        iothub_client_mqtt_transport
    )
endif()

# Parson Library -- Compiled and Linked at a higher level
include_directories(${PROJECT_SOURCE_DIR}/deps/parson)

# Tinycbor Library -- Compile, Link, and Include
set(TINYCBOR_DIR ${PROJECT_SOURCE_DIR}/deps/tinycbor/src)

set(tinycbor_c_files
    ${TINYCBOR_DIR}/cborencoder.c
    ${TINYCBOR_DIR}/cborerrorstrings.c
    ${TINYCBOR_DIR}/cborparser.c
    ${TINYCBOR_DIR}/cborvalidation.c
)
set(tinycbor_h_files
    ${TINYCBOR_DIR}/cbor.h
    ${TINYCBOR_DIR}/cborinternal_p.h
    ${TINYCBOR_DIR}/compilersupport_p.h
    ${TINYCBOR_DIR}/utf8_p.h
)

add_library(tinycbor
    ${tinycbor_c_files}
    ${tinycbor_h_files}
)

target_link_libraries(iothub_client_device_twin_and_methods_sample
    tinycbor
)

include_directories(${TINYCBOR_DIR})
