// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This sample shows how to translate the Device Twin document received from Azure IoT Hub into
// meaningful data for your application. It also shows how to work with Direct Methods and their
// encoded payloads.
//
// There are two encoding options: CBOR or JSON.  This sample demonstrates the use of CBOR only and
// employs the tinycbor library which must be installed independently of the C SDK. However, you may
// choose your own preferred library to encode/decode the Device Twin document and Direct Methods payloads.

// WARNING: Check the return of all API calls when developing your solution. Return checks are
//          ommited from this sample for simplification.

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "azure_macro_utils/macro_utils.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"

#include "iothub.h"
#include "iothub_client_options.h"
#include "iothub_device_client.h"
#include "iothub_message.h"

#include "tinycbor/cbor.h"
#define CBOR_BUFFER_SIZE 512

//
// Transport Layer Protocal -- Uncomment the protocol you wish to use.
//
#define SAMPLE_MQTT
//#define SAMPLE_MQTT_OVER_WEBSOCKETS

#ifdef SAMPLE_MQTT
    #include "iothubtransportmqtt.h"
#elif defined SAMPLE_MQTT_OVER_WEBSOCKETS
    #include "iothubtransportmqtt_websockets.h"
#endif // SAMPLE PROTOCOL

//
// Trusted Cert -- Turn on via build flag
//
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    #include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

//
// Connection String - Paste in the your iothub device connection string.
//
static const char* connection_string = "[device connection string]";

static IOTHUB_DEVICE_CLIENT_HANDLE iothub_client_handle;

//
// Car Object
//
typedef struct MAKER_TAG
{
    unsigned char* name;
    unsigned char* style;
    uint64_t year;
} Maker;

typedef struct STATE_TAG
{
    uint64_t software_version;         // desired/reported property
    uint8_t max_speed;                 // desired/reported property
    unsigned char* vanity_plate;       // reported property
} State;

typedef struct CAR_TAG
{
    unsigned char* last_oil_change_date;    // reported property
    bool change_oil_reminder;               // desired/reported property
    Maker maker;                            // reported property
    State state;                            // desired/reported property
} Car;


//
// Encoding/Decoding with chosen library
//

//  Serialize Car object to CBOR blob. To be sent as a twin document with reported properties.
static void serializeToCBOR(Car* car, uint8_t* cbor_buf, size_t buffer_size)
{
    CborEncoder cbor_encoder_root;
    CborEncoder cbor_encoder_root_container;
    CborEncoder cbor_encoder_maker;
    CborEncoder cbor_encoder_state;

    // WARNING: Check the return of all API calls when developing your solution. Return checks are
    //          ommited from this sample for simplification.
    cbor_encoder_init(&cbor_encoder_root, cbor_buf, buffer_size, 0);

    (void)cbor_encoder_create_map(&cbor_encoder_root, &cbor_encoder_root_container, 3);

        (void)cbor_encode_text_string(&cbor_encoder_root_container, "last_oil_change_date", strlen("last_oil_change_date"));
        (void)cbor_encode_text_string(&cbor_encoder_root_container, car->last_oil_change_date, strlen(car->last_oil_change_date));

        (void)cbor_encode_text_string(&cbor_encoder_root_container, "maker", strlen("maker"));
        (void)cbor_encoder_create_map(&cbor_encoder_root_container, &cbor_encoder_maker, 3);
            (void)cbor_encode_text_string(&cbor_encoder_maker, "name", strlen("name"));
            (void)cbor_encode_text_string(&cbor_encoder_maker, car->maker.name, strlen(car->maker.name));
            (void)cbor_encode_text_string(&cbor_encoder_maker, "style", strlen("style"));
            (void)cbor_encode_text_string(&cbor_encoder_maker, car->maker.style, strlen(car->maker.style));
            (void)cbor_encode_text_string(&cbor_encoder_maker, "year", strlen("year"));
            (void)cbor_encode_uint(&cbor_encoder_maker, car->maker.year);
        (void)cbor_encoder_close_container(&cbor_encoder_root_container, &cbor_encoder_maker);

        (void)cbor_encode_text_string(&cbor_encoder_root_container, "state", strlen("state"));
        (void)cbor_encoder_create_map(&cbor_encoder_root_container, &cbor_encoder_state, 3);
            (void)cbor_encode_text_string(&cbor_encoder_state, "max_speed", strlen("max_speed"));
            (void)cbor_encode_simple_value(&cbor_encoder_state, car->state.max_speed);
            (void)cbor_encode_text_string(&cbor_encoder_state, "software_version", strlen("software_version"));
            (void)cbor_encode_uint(&cbor_encoder_state, car->state.software_version);
            (void)cbor_encode_text_string(&cbor_encoder_state, "vanity_plate", strlen("vanity_plate"));
            (void)cbor_encode_text_string(&cbor_encoder_state, car->state.vanity_plate, strlen(car->state.vanity_plate));
        (void)cbor_encoder_close_container(&cbor_encoder_root_container, &cbor_encoder_state);

    (void)cbor_encoder_close_container(&cbor_encoder_root, &cbor_encoder_root_container);
}

// Convert the desired properties of the Device Twin CBOR blob from IoT Hub into a Car Object.
static void parseFromCBOR(Car* car, const unsigned char* cbor_payload)
{
    CborParser cbor_parser;
    CborValue root;
    CborValue state_root;

    // Only desired properties:
    CborValue change_oil_reminder;
    CborValue max_speed;
    CborValue software_version;

    // WARNING: Check the return of all API calls when developing your solution. Return checks are
    //          ommited from this sample for simplification.
    (void)cbor_parser_init(cbor_payload, strlen(cbor_payload), 0, &cbor_parser, &root);

    (void)cbor_value_map_find_value(&root, "change_oil_reminder", &change_oil_reminder);
    if (cbor_value_is_valid(&change_oil_reminder))
    {
        cbor_value_get_boolean(&change_oil_reminder, &car->change_oil_reminder);
    }

    (void)cbor_value_map_find_value(&root, "state", &state_root);
    (void)cbor_value_map_find_value(&state_root, "max_speed", &max_speed);
    if (cbor_value_is_valid(&max_speed))
    {
        cbor_value_get_simple_type(&max_speed, &car->state.max_speed);
    }

    (void)cbor_value_map_find_value(&state_root, "software_version", &software_version);
    if (cbor_value_is_valid(&software_version))
    {
        cbor_value_get_uint64(&software_version, &car->state.software_version);
    }
}


//
// Callbacks
//
static void deviceDesiredPropertiesTwinCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payload, size_t size, void* userContextCallback);
// Callback for async GET request to IoT Hub for entire Device Twin document.
static void getTwinAsyncCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
{
    (void)update_state;
    (void)userContextCallback;

    //printf("getTwinAsyncCallback payload:\n%.*s\n", (int)size, payLoad);

    //TEST//
    // JSON: {"changeOilReminder":true,"state":{"maxSpeed":120,"softwareVersion":2},"$version":13}
    // CBOR: A3 71 63 68 61 6E 67 65 4F 69 6C 52 65 6D 69 6E 64 65 72 F5 65 73 74 61 74 65 A2 68 6D 61 78 53 70 65 65 64 18 78 6F 73 6F 66 74 77 61 72 65 56 65 72 73 69 6F 6E 02 68 24 76 65 72 73 69 6F 6E 0D
    //uint8_t cbor_array[] = {0xA3, 0x71, 0x63, 0x68, 0x61, 0x6E, 0x67, 0x65, 0x4F, 0x69, 0x6C, 0x52, 0x65, 0x6D, 0x69, 0x6E, 0x64, 0x65, 0x72, 0xF5, 0x65, 0x73, 0x74, 0x61, 0x74, 0x65, 0xA2, 0x68, 0x6D, 0x61, 0x78, 0x53, 0x70, 0x65, 0x65, 0x64, 0x18, 0x78, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x02, 0x68, 0x24, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x0D};
    //Car desired_car;
    //memset(&desired_car, 0, sizeof(Car));
    //parseFromCBOR(&desired_car, cbor_array);
    //printf("change_oil_reminder: %d\n", desired_car.change_oil_reminder);
}

// Callback for when device sends reported properties to IoT Hub, and IoT Hub updates the Device
// Twin document.
static void deviceReportedPropertiesTwinCallback(int status_code, void* userContextCallback)
{
    (void)userContextCallback;
    printf("deviceReportedPropertiesTwinCallback: Result status code: %d\n", status_code);
}

// Callback for when IoT Hub updates the desired properties of the Device Twin document.
static void deviceDesiredPropertiesTwinCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payload, size_t size, void* userContextCallback)
{
    (void)update_state;
    (void)size;

    printf("deviceDesiredPropertiesTwinCallback payload:\n%.*s\n", (int)size, payload);

    Car* car = (Car*)userContextCallback;
    Car desired_car;
    memset(&desired_car, 0, sizeof(Car));
    parseFromCBOR(&desired_car, payload);

    if (desired_car.change_oil_reminder != car->change_oil_reminder)
    {
        printf("Received a desired change_oil_reminder = %d\n", desired_car.change_oil_reminder);
        car->change_oil_reminder = desired_car.change_oil_reminder;
    }

    if (desired_car.state.max_speed != 0 && desired_car.state.max_speed != car->state.max_speed)
    {
        printf("Received a desired max_speed = %" PRIu8 "\n", desired_car.state.max_speed);
        car->state.max_speed = desired_car.state.max_speed;
    }

    if (desired_car.state.software_version != 0 && desired_car.state.software_version != car->state.software_version)
    {
        printf("Received a desired software_version = %ld" "\n", desired_car.state.software_version);
        car->state.software_version = desired_car.state.software_version;
    }

    uint8_t reported_properties[CBOR_BUFFER_SIZE];
    serializeToCBOR(car, reported_properties, CBOR_BUFFER_SIZE);

    (void)IoTHubDeviceClient_SendReportedState(iothub_client_handle, reported_properties, strlen(reported_properties), deviceReportedPropertiesTwinCallback, NULL);
            ThreadAPI_Sleep(1000);
}

// Callback for when IoT Hub sends a Direct Method to the device.
static int deviceMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback)
{
    (void)userContextCallback;
    (void)payload;
    (void)size;

    int result;

    printf("deviceMethodCallback: method name: %s, payload: %.*s\n", method_name, (int)size, payload);

    if (strcmp("getCarVIN", method_name) == 0)
    {
        const char device_method_response[] = "{ \"Response\": \"1HGCM82633A004352\" }";
        *response_size = sizeof(device_method_response)-1;
        *response = malloc(*response_size);
        (void)memcpy(*response, device_method_response, *response_size);
        result = 200;
    }
    else
    {
        // All other entries are ignored.
        const char device_method_response[] = "{ }";
        *response_size = sizeof(device_method_response)-1;
        *response = malloc(*response_size);
        (void)memcpy(*response, device_method_response, *response_size);
        result = -1;
    }

    return result;
}

static void iothub_client_device_twin_and_methods_sample_run(void)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;

    //
    // Select the Transport Layer Protocal
    //
#ifdef SAMPLE_MQTT
    protocol = MQTT_Protocol;
#elif defined SAMPLE_MQTT_OVER_WEBSOCKETS
    protocol = MQTT_WebSocket_Protocol;
#endif // SAMPLE PROTOCOL

    if (IoTHub_Init() != 0)
    {
        (void)printf("Failed to initialize the platform.\r\n");
    }
    else
    {
        if ((iothub_client_handle = IoTHubDeviceClient_CreateFromConnectionString(connection_string, protocol)) == NULL)
        {
            (void)printf("ERROR: iothub_client_handle is NULL!\r\n");
        }
        else
        {
            //
            // Set Options
            //
            bool trace_on = true; // Debugging
            (void)IoTHubDeviceClient_SetOption(iothub_client_handle, OPTION_LOG_TRACE, &trace_on);

            // Set the auto URL Encoder (recommended for MQTT). Please use this option unless you
            // are URL Encoding inputs yourself. ONLY valid for use with MQTT.
            bool url_encode_on = true;
            (void)IoTHubDeviceClient_SetOption(iothub_client_handle, OPTION_AUTO_URL_ENCODE_DECODE, &url_encode_on);

            // Format Device Twin document and Direct Method payload using CBOR.
            // ONLY valid for use with MQTT. Must occur prior to CONNECT.
          //  OPTION_METHOD_TWIN_CONTENT_TYPE_VALUE ct = OPTION_METHOD_TWIN_CONTENT_TYPE_VALUE_CBOR;
          //  (void)IoTHubDeviceClient_SetOption(iothub_client_handle, OPTION_METHOD_TWIN_CONTENT_TYPE, &ct);

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
            (void)IoTHubDeviceClient_SetOption(iothub_client_handle, "TrustedCerts", certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

            //
            // Create Car Object
            //
            Car car;
            memset(&car, 0, sizeof(Car));
            car.last_oil_change_date = "2016";
            car.maker.name = "Fabrikam";
            car.maker.style = "sedan";
            car.maker.year = 2014;
            car.state.max_speed = 100;
            car.state.software_version = 1;
            car.state.vanity_plate = "1T1";

            uint8_t reported_properties[CBOR_BUFFER_SIZE];
            serializeToCBOR(&car, reported_properties, CBOR_BUFFER_SIZE);
            printf("Size of encoded CBOR: %zu\n", strlen(reported_properties));
            // IMPORTANT: You must validate your own data prior to sending.

            //
            // Send and receive messages from IoT Hub
            //
            (void)IoTHubDeviceClient_GetTwinAsync(iothub_client_handle, getTwinAsyncCallback, NULL);
            ThreadAPI_Sleep(1000);

            (void)IoTHubDeviceClient_SendReportedState(iothub_client_handle, reported_properties, strlen(reported_properties), deviceReportedPropertiesTwinCallback, NULL);
            ThreadAPI_Sleep(1000);

            (void)IoTHubDeviceClient_SetDeviceTwinCallback(iothub_client_handle, deviceDesiredPropertiesTwinCallback, &car);
            ThreadAPI_Sleep(1000);

            (void)IoTHubDeviceClient_SetDeviceMethodCallback(iothub_client_handle, deviceMethodCallback, NULL);
            ThreadAPI_Sleep(1000);

            //
            // Exit
            //
            (void)printf("Wait for desired properties update or direct method from service. Press any key to exit sample.\r\n");
            (void)getchar();

            IoTHubDeviceClient_Destroy(iothub_client_handle);
        }

        IoTHub_Deinit();
    }
}

int main(void)
{
    iothub_client_device_twin_and_methods_sample_run();
    return 0;
}
