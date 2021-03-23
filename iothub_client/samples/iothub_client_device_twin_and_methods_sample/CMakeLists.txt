#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for iothub_client_device_twin_and_methods_sample

if(NOT ${use_mqtt} AND NOT ${use_amqp})
    message(FATAL_ERROR "iothub_client_device_twin_and_methods_sample being generated without amqp or mqtt support")
endif()

compileAsC99()

set(iothub_client_device_twin_and_methods_sample_c_files
    iothub_client_device_twin_and_methods_sample.c
)

set(iothub_client_device_twin_and_methods_sample_h_files
)

IF(WIN32)
    #windows needs this define
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC)
ENDIF(WIN32)

#Conditionally use the SDK trusted certs in the samples
if(${use_sample_trusted_cert})
    add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)
    include_directories(${PROJECT_SOURCE_DIR}/certs)
    set(iothub_client_device_twin_and_methods_sample_c_files ${iothub_client_device_twin_and_methods_sample_c_files} ${PROJECT_SOURCE_DIR}/certs/certs.c)
endif()

include_directories(.)
include_directories(${PROJECT_SOURCE_DIR}/deps/parson)
include_directories(${PROJECT_SOURCE_DIR}/deps/tinycbor/src)

add_executable(iothub_client_device_twin_and_methods_sample ${iothub_client_device_twin_and_methods_sample_c_files} ${iothub_client_device_twin_and_methods_sample_h_files})

if(${use_amqp})
    target_link_libraries(iothub_client_device_twin_and_methods_sample
        iothub_client_amqp_transport
        uamqp
    )
    add_definitions(-DUSE_AMQP)
endif()

if(${use_mqtt})
    target_link_libraries(iothub_client_device_twin_and_methods_sample
        iothub_client_mqtt_transport
    )
    add_definitions(-DUSE_MQTT)
endif()

target_link_libraries(iothub_client_device_twin_and_methods_sample
    aziotsharedutil
    iothub_client
)

#Add tinycbor support only as needed by this sample.
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

#install(FILES ${tinycbor_h_files}
#    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/azureiot)
install(TARGETS ${tinycbor}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

target_link_libraries(iothub_client_device_twin_and_methods_sample
    tinycbor
)
