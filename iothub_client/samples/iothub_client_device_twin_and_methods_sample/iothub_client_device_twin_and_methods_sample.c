// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This sample shows how to translate the Device Twin document received from Azure IoT Hub into
// meaningful data for your application. It also shows how to work with Direct Methods and their
// encoded payloads.
//
// There are two encoding options: CBOR or JSON. This sample demonstrates the use of JSON only and
// employs the parson library. However, you may choose your own preferred library to encode/decode
// the Device Twin document and Direct Methods payloads.

// There are analogous samples using the serializer component, which is an SDK library provided to
// help parse JSON. The serializer is dependent on the parson library. These samples are
// devicetwin_simplesample and devicemethod_simplesample. Most applications will not need use of the
// serializer.

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

#include "parson.h"

//
// Transport Layer Protocal -- Uncomment the protocol you wish to use.
//
//#define SAMPLE_MQTT
#define SAMPLE_MQTT_OVER_WEBSOCKETS
//#define SAMPLE_AMQP
//#define SAMPLE_AMQP_OVER_WEBSOCKETS
//#define SAMPLE_HTTP

#ifdef SAMPLE_MQTT
    #include "iothubtransportmqtt.h"
#elif defined SAMPLE_MQTT_OVER_WEBSOCKETS
    #include "iothubtransportmqtt_websockets.h"
#elif defined SAMPLE_AMQP
    #include "iothubtransportamqp.h"
#elif SAMPLE_AMQP_OVER_WEBSOCKETS
    #include "iothubtransportamqp_websockets.h"
#elif defined SAMPLE_HTTP
    #include "iothubtransporthttp.h"
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

// Serialize Car object to JSON blob. To be sent as a twin document with reported properties.
static void serializeToJSON(Car* car, unsigned char** result)
{
    JSON_Value* root_value = json_value_init_object(); // internal malloc
    JSON_Object* root_object = json_value_get_object(root_value);

    // Only reported properties:
    (void)json_object_set_string(root_object, "last_oil_change_date", car->last_oil_change_date);
    (void)json_object_dotset_string(root_object, "maker.name", car->maker.name);
    (void)json_object_dotset_string(root_object, "maker.style", car->maker.style);
    (void)json_object_dotset_number(root_object, "maker.year", car->maker.year);
    (void)json_object_dotset_number(root_object, "state.max_speed", car->state.max_speed);
    (void)json_object_dotset_number(root_object, "state.software_version", car->state.software_version);
    (void)json_object_dotset_string(root_object, "state.vanity_plate", car->state.vanity_plate);

    *result = (unsigned char*)json_serialize_to_string(root_value); // internal malloc

    json_value_free(root_value);
}

// Convert the desired properties of the Device Twin JSON blob from IoT Hub into a Car Object.
static void parseFromJSON(Car *car, const unsigned char* json_payload)
{
    JSON_Value* root_value = json_parse_string(json_payload);
    JSON_Object* root_object = json_value_get_object(root_value);

    // Only desired properties:
    JSON_Value* change_oil_reminder = json_object_get_value(root_object, "change_oil_reminder");
    JSON_Value* max_speed = json_object_dotget_value(root_object, "state.max_speed");
    JSON_Value* software_version = json_object_dotget_value(root_object, "state.software_version");

    if (change_oil_reminder != NULL)
    {
        car->change_oil_reminder = json_value_get_boolean(change_oil_reminder);
    }

    if (max_speed != NULL)
    {
        car->state.max_speed = (uint8_t)json_value_get_number(max_speed);
    }

    if (software_version != NULL)
    {
        car->state.software_version = json_value_get_number(software_version);
    }
}

//
// Callbacks
//

// Callback for async GET request to IoT Hub for entire Device Twin document.
static void getTwinAsyncCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback)
{
    (void)update_state;
    (void)userContextCallback;

    printf("getTwinAsyncCallback payload:\n%.*s\n", (int)size, payLoad);
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
    Car desiredCar;
    memset(&desiredCar, 0, sizeof(Car));
    parseFromJSON(&desiredCar, payload);
    // IMPORTANT: You must validate your own data prior to sending.

    if (desiredCar.change_oil_reminder != car->change_oil_reminder)
    {
        printf("Received a desired change_oil_reminder = %d\n", desiredCar.change_oil_reminder);
        car->change_oil_reminder = desiredCar.change_oil_reminder;
    }

    if (desiredCar.state.max_speed != 0 && desiredCar.state.max_speed != car->state.max_speed)
    {
        printf("Received a desired max_speed = %" PRIu8 "\n", desiredCar.state.max_speed);
        car->state.max_speed = desiredCar.state.max_speed;
    }

    if (desiredCar.state.software_version != 0 && desiredCar.state.software_version != car->state.software_version)
    {
        printf("Received a desired software_version = %ld" "\n", desiredCar.state.software_version);
        car->state.software_version = desiredCar.state.software_version;
    }

    unsigned char* reported_properties;
    serializeToJSON(car, &reported_properties); // internal malloc

    (void)IoTHubDeviceClient_SendReportedState(iothub_client_handle, reported_properties, strlen(reported_properties), deviceReportedPropertiesTwinCallback, NULL);
            ThreadAPI_Sleep(1000);

    free(reported_properties);
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
#elif defined SAMPLE_AMQP
    protocol = AMQP_Protocol;
#elif defined SAMPLE_AMQP_OVER_WEBSOCKETS
    protocol = AMQP_Protocol_over_WebSocketsTls;
#elif defined SAMPLE_HTTP
    protocol = HTTP_Protocol;
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

#if defined SAMPLE_MQTT || defined SAMPLE_MQTT_OVER_WEBSOCKETS
            // Set the auto URL Encoder (recommended for MQTT). Please use this option unless you
            // are URL Encoding inputs yourself. ONLY valid for use with MQTT.
            bool url_encode_on = true;
            (void)IoTHubDeviceClient_SetOption(iothub_client_handle, OPTION_AUTO_URL_ENCODE_DECODE, &url_encode_on);

            // This option not required to use JSON format due to backwards compatibility.
            // If option is used, it is ONLY valid for use with MQTT. Must occur priot to CONNECT.
            //OPTION_METHOD_TWIN_CONTENT_TYPE_VALUE ct = OPTION_METHOD_TWIN_CONTENT_TYPE_VALUE_JSON;
            //(void)IoTHubDeviceClient_SetOption(iothub_client_handle, OPTION_METHOD_TWIN_CONTENT_TYPE, &ct);
#endif // SAMPLE_MQTT || SAMPLE_MQTT_OVER_WEBSOCKETS

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

            unsigned char* reported_properties;
            serializeToJSON(&car, &reported_properties); // internal malloc
            printf("Size of encoded JSON: %zu\n", strlen(reported_properties));

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
            free(reported_properties);
        }

        IoTHub_Deinit();
    }
}

int main(void)
{
    iothub_client_device_twin_and_methods_sample_run();
    return 0;
}
