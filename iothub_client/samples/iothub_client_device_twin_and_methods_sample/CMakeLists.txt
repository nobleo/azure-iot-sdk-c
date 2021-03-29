#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT ${use_mqtt} AND NOT ${use_amqp} AND NOT ${use_http})
    message(FATAL_ERROR "iothub_client_device_twin_and_methods_sample being generated without protocol support")
endif()

compileAsC99()

# Sample Executable
set(iothub_client_device_twin_and_methods_sample_c_files
    iothub_client_device_twin_and_methods_sample.c
)

add_executable(iothub_client_device_twin_and_methods_sample
    ${iothub_client_device_twin_and_methods_sample_c_files}
)

target_link_libraries(iothub_client_device_twin_and_methods_sample
    iothub_client
)

# Certs Library
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    target_link_libraries(iothub_client_device_twin_and_methods_sample certs)
endif()

# Parson Library
include_directories(${PROJECT_SOURCE_DIR}/deps/parson)
target_link_libraries(iothub_client_device_twin_and_methods_sample parson)

# Tinycbor Library
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

include_directories(${TINYCBOR_DIR})
target_link_libraries(iothub_client_device_twin_and_methods_sample tinycbor)
