#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for samples. There's nothing here, except redirections to
#individual protocol samples

usePermissiveRulesForSdkSamplesAndTests()

# Tinycbor Library
set(TINYCBOR_DIR ${CMAKE_CURRENT_LIST_DIR}/deps/tinycbor/src)

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
set_target_properties(tinycbor PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/deps/tinycbor/
)

function(add_sample_directory whatIsBuilding)
    add_subdirectory(${whatIsBuilding})

    set_target_properties(${whatIsBuilding}
               PROPERTIES
               FOLDER "IoTHub_Samples")
endfunction()

add_sample_directory(iothub_convenience_sample)
add_sample_directory(iothub_ll_c2d_sample)
add_sample_directory(iothub_ll_client_x509_sample)
add_sample_directory(iothub_ll_telemetry_sample)
add_sample_directory(iotedge_downstream_device_sample)
add_sample_directory(iothub_client_device_twin_and_methods_sample)

if(${use_http})
    if(NOT ${dont_use_uploadtoblob})
        add_sample_directory(iothub_client_sample_upload_to_blob)
        add_sample_directory(iothub_client_sample_upload_to_blob_mb)
    endif()
    if (${use_edge_modules})
        add_sample_directory(iothub_client_sample_module_method_invoke)
    endif()
endif()

if(${use_mqtt})
    if(LINUX)
        add_sample_directory(iothub_client_sample_mqtt_dm)
    endif()
    if(${use_edge_modules})
        add_sample_directory(iothub_client_sample_module_sender)
        add_sample_directory(iothub_client_sample_module_filter)
    endif()
endif()

if(${use_amqp})
    add_sample_directory(iothub_client_sample_amqp_shared_methods)
endif()

if(${use_amqp} AND ${use_http})
    add_sample_directory(iothub_ll_client_shared_sample)
endif()

if(${use_mqtt})
   # PnP only supports MQTT so only build the sample if MQTT is enabled.
   add_subdirectory(pnp)
endif()
