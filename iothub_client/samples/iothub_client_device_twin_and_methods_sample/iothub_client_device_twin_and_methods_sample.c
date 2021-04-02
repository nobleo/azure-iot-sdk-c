// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// This sample shows how to translate the Device Twin document received from Azure IoT Hub into
// meaningful data for your application. It also shows how to work with Direct Methods and their
// encoded payloads.
//
// There are two encoding options: CBOR or JSON. For CBOR, this sample uses the tinycbor library,
// and for JSON, it uses the parson library. Both tinycbor and parson are included in the C SDK as
// submodules, however you may choose your own preferred library to encode/decode the Device Twin
// document and Direct Methods payloads.

// There are analogous samples using the serializer component, which is an SDK library provided to
// help parse JSON. The serializer is dependent on the parson library, and there is no CBOR support.
// These samples are devicetwin_simplesample and devicemethod_simplesample. Most applications will
// not need use of the serializer.

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

//
// Format -- Uncomment the format you wish to use.
//
#define CONTENT_TYPE_CBOR
//#define CONTENT_TYPE_JSON

#ifdef CONTENT_TYPE_CBOR
    #include "cbor.h"
    #define CBOR_BUFFER_SIZE 512
#elif defined CONTENT_TYPE_JSON
    #include "parson.h"
#endif // CONTENT TYPE

//
// Transport Layer Protocal -- Uncomment the protocol you wish to use.
//
#define SAMPLE_MQTT
//#define SAMPLE_MQTT_OVER_WEBSOCKETS
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
static const char* connectionString = "[device connection string]";

static IOTHUB_DEVICE_CLIENT_HANDLE iotHubClientHandle;

//
// Car Object
//
typedef struct MAKER_TAG
{
    unsigned char name[32];
    unsigned char style[32];
    uint64_t year;
} Maker;

typedef struct STATE_TAG
{
    uint64_t softwareVersion;         // desired/reported property
    uint8_t maxSpeed;                 // desired/reported property
    unsigned char vanityPlate[32];    // reported property
} State;

typedef struct CAR_TAG
{
    unsigned char lastOilChangeDate[32];    // reported property
    bool changeOilReminder;                 // desired/reported property
    Maker maker;                            // reported property
    State state;                            // desired/reported property
} Car;


//
//  Serialize Car object to CBOR/JSON blob. To be sent as a twin document with reported properties.
//
#ifdef CONTENT_TYPE_CBOR
static void serializeToCBOR(Car* car, uint8_t* cbor_buf, size_t buffer_size)
{
    CborEncoder cbor_encoder_root;
    CborEncoder cbor_encoder_root_container;
    CborEncoder cbor_encoder_maker;
    CborEncoder cbor_encoder_state;

    cbor_encoder_init(&cbor_encoder_root, cbor_buf, buffer_size, 0);

    (void)cbor_encoder_create_map(&cbor_encoder_root, &cbor_encoder_root_container, 3);

        (void)cbor_encode_text_string(&cbor_encoder_root_container, "lastOilChangeDate", strlen("lastOilChangeDate"));
        (void)cbor_encode_boolean(&cbor_encoder_root_container, car->lastOilChangeDate);

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
            (void)cbor_encode_text_string(&cbor_encoder_state, "maxSpeed", strlen("maxSpeed"));
            (void)cbor_encode_simple_value(&cbor_encoder_state, car->state.maxSpeed);
            (void)cbor_encode_text_string(&cbor_encoder_state, "softwareVersion", strlen("softwareVersion"));
            (void)cbor_encode_uint(&cbor_encoder_state, car->state.softwareVersion);
            (void)cbor_encode_text_string(&cbor_encoder_state, "vanityPlate", strlen("vanityPlate"));
            (void)cbor_encode_text_string(&cbor_encoder_state, car->state.vanityPlate, strlen(car->state.vanityPlate));
        (void)cbor_encoder_close_container(&cbor_encoder_root_container, &cbor_encoder_state);

    (void)cbor_encoder_close_container(&cbor_encoder_root, &cbor_encoder_root_container);
}
#elif defined CONTENT_TYPE_JSON
static void serializeToJSON(Car* car, unsigned char** result)
{
    JSON_Value* root_value = json_value_init_object(); // internal malloc
    JSON_Object* root_object = json_value_get_object(root_value);

    // Only reported properties:
    (void)json_object_set_string(root_object, "lastOilChangeDate", car->lastOilChangeDate);
    (void)json_object_dotset_string(root_object, "maker.name", car->maker.name);
    (void)json_object_dotset_string(root_object, "maker.style", car->maker.style);
    (void)json_object_dotset_number(root_object, "maker.year", car->maker.year);
    (void)json_object_dotset_number(root_object, "state.maxSpeed", car->state.maxSpeed);
    (void)json_object_dotset_number(root_object, "state.softwareVersion", car->state.softwareVersion);
    (void)json_object_dotset_string(root_object, "state.vanityPlate", car->state.vanityPlate);

    *result = (unsigned char*)json_serialize_to_string(root_value); // internal malloc

    json_value_free(root_value);
}
#endif // CONTENT TYPE

//
// Convert the desired properties of the Device Twin CBOR/JSON blob from IoT Hub into a Car Object.
//
#ifdef CONTENT_TYPE_CBOR
static void parseFromCBOR(Car* car, const unsigned char* cbor_payload)
{
    CborParser cbor_parser;
    CborValue root;
    CborValue state_root;

    // Only desired properties:
    CborValue changeOilReminder;
    CborValue maxSpeed;
    CborValue softwareVersion;

    (void)cbor_parser_init(cbor_payload, strlen(cbor_payload), 0, &cbor_parser, &root);

    (void)cbor_value_map_find_value(&root, "changeOilReminder", &changeOilReminder);
    if (cbor_value_is_valid(&changeOilReminder))
    {
        cbor_value_get_boolean(&changeOilReminder, &car->changeOilReminder);
    }

    (void)cbor_value_map_find_value(&root, "state", &state_root);
        (void)cbor_value_map_find_value(&state_root, "maxSpeed", &maxSpeed);
        if (cbor_value_is_valid(&maxSpeed))
        {
            cbor_value_get_simple_type(&maxSpeed, &car->state.maxSpeed);
        }

        (void)cbor_value_map_find_value(&state_root, "softwareVersion", &softwareVersion);
        if (cbor_value_is_valid(&softwareVersion))
        {
            cbor_value_get_uint64(&softwareVersion, &car->state.softwareVersion);
        }
}
#elif defined CONTENT_TYPE_JSON
static void parseFromJSON(Car *car, const unsigned char* json_payload)
{
    JSON_Value* root_value = json_parse_string(json_payload);
    JSON_Object* root_object = json_value_get_object(root_value);

    // Only desired properties:
    JSON_Value* changeOilReminder = json_object_get_value(root_object, "changeOilReminder");
    JSON_Value* maxSpeed = json_object_dotget_value(root_object, "state.maxSpeed");
    JSON_Value* softwareVersion = json_object_dotget_value(root_object, "state.softwareVersion");

    if (changeOilReminder != NULL)
    {
        car->changeOilReminder = json_value_get_boolean(changeOilReminder);
    }

    if (maxSpeed != NULL)
    {
        car->state.maxSpeed = (uint8_t)json_value_get_number(maxSpeed);
    }

    if (softwareVersion != NULL)
    {
        car->state.softwareVersion = json_value_get_number(softwareVersion);
    }
}
#endif // CONTENT TYPE

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

#ifdef CONTENT_TYPE_CBOR
// TO test CBOR parser ONLY.
    Car car1;
    memset(&car1, 0, sizeof(Car));
    strcpy(car1.lastOilChangeDate, "2016");
    strcpy(car1.maker.name, "Fabrikam");
    strcpy(car1.maker.style, "sedan");
    car1.maker.year = 2014;
    car1.state.maxSpeed = 158;
    car1.state.softwareVersion = 55;
    strcpy(car1.state.vanityPlate, "1T1");

    uint8_t reportedProperties1[CBOR_BUFFER_SIZE];
    serializeToCBOR(&car1, reportedProperties1, CBOR_BUFFER_SIZE);
    payload = reportedProperties1;
#endif
    printf("deviceDesiredPropertiesTwinCallback payload:\n%.*s\n", (int)size, payload);

    Car* car = (Car*)userContextCallback;
    Car desiredCar;
    memset(&desiredCar, 0, sizeof(Car));

#ifdef CONTENT_TYPE_CBOR
    parseFromCBOR(&desiredCar, payload);
#elif defined CONTENT_TYPE_JSON
    parseFromJSON(&desiredCar, payload);
#endif // CONTENT TYPE

    if (desiredCar.changeOilReminder != car->changeOilReminder)
    {
        printf("Received a desired changeOilReminder = %d\n", desiredCar.changeOilReminder);
        car->changeOilReminder = desiredCar.changeOilReminder;
    }

    if (desiredCar.state.maxSpeed != 0 && desiredCar.state.maxSpeed != car->state.maxSpeed)
    {
        printf("Received a desired maxSpeed = %" PRIu8 "\n", desiredCar.state.maxSpeed);
        car->state.maxSpeed = desiredCar.state.maxSpeed;
    }

    if (desiredCar.state.softwareVersion != 0 && desiredCar.state.softwareVersion != car->state.softwareVersion)
    {
        printf("Received a desired softwareVersion = %ld" "\n", desiredCar.state.softwareVersion);
        car->state.softwareVersion = desiredCar.state.softwareVersion;
    }

#ifdef CONTENT_TYPE_CBOR
    uint8_t reportedProperties[CBOR_BUFFER_SIZE];
    serializeToCBOR(car, reportedProperties, CBOR_BUFFER_SIZE);
#elif defined CONTENT_TYPE_JSON
    unsigned char* reportedProperties;
    serializeToJSON(car, &reportedProperties); // internal malloc
#endif // CONTENT TYPE

    (void)IoTHubDeviceClient_SendReportedState(iotHubClientHandle, reportedProperties, strlen(reportedProperties), deviceReportedPropertiesTwinCallback, NULL);
            ThreadAPI_Sleep(1000);

#ifdef CONTENT_TYPE_JSON
    free(reportedProperties);
#endif // CONTENT_TYPE_JSON
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
        const char deviceMethodResponse[] = "{ \"Response\": \"1HGCM82633A004352\" }";
        *response_size = sizeof(deviceMethodResponse)-1;
        *response = malloc(*response_size);
        (void)memcpy(*response, deviceMethodResponse, *response_size);
        result = 200;
    }
    else
    {
        // All other entries are ignored.
        const char deviceMethodResponse[] = "{ }";
        *response_size = sizeof(deviceMethodResponse)-1;
        *response = malloc(*response_size);
        (void)memcpy(*response, deviceMethodResponse, *response_size);
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
        if ((iotHubClientHandle = IoTHubDeviceClient_CreateFromConnectionString(connectionString, protocol)) == NULL)
        {
            (void)printf("ERROR: iotHubClientHandle is NULL!\r\n");
        }
        else
        {
            //
            // Set Options
            //
            bool traceOn = true; // Debugging
            (void)IoTHubDeviceClient_SetOption(iotHubClientHandle, OPTION_LOG_TRACE, &traceOn);

#if defined SAMPLE_MQTT || defined SAMPLE_MQTT_OVER_WEBSOCKETS
            // Set the auto URL Encoder (recommended for MQTT). Please use this option unless you
            // are URL Encoding inputs yourself. ONLY valid for use with MQTT.
            bool urlEncodeOn = true;
            (void)IoTHubDeviceClient_SetOption(iotHubClientHandle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);
#ifdef CONTENT_TYPE_CBOR
            // Format Device Twin document and Direct Method payload using CBOR.
            // ONLY valid for use with MQTT. Must occur prior to CONNECT.
            //OPTION_METHOD_TWIN_CONTENT_TYPE_VALUE ct = OPTION_METHOD_TWIN_CONTENT_TYPE_VALUE_CBOR;
            //(void)IoTHubDeviceClient_SetOption(iotHubClientHandle, OPTION_METHOD_TWIN_CONTENT_TYPE, &ct);
#elif defined CONTENT_TYPE_JSON
            // This option not required to use JSON format due to backwards compatibility.
            // If option is used, it is ONLY valid for use with MQTT. Must occur priot to CONNECT.
            //OPTION_METHOD_TWIN_CONTENT_TYPE_VALUE ct = OPTION_METHOD_TWIN_CONTENT_TYPE_VALUE_JSON;
            //(void)IoTHubDeviceClient_SetOption(iotHubClientHandle, OPTION_METHOD_TWIN_CONTENT_TYPE, &ct);
#endif // CONTENT TYPE
#endif // SAMPLE_MQTT || SAMPLE_MQTT_OVER_WEBSOCKETS

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
            (void)IoTHubDeviceClient_SetOption(iotHubClientHandle, "TrustedCerts", certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

            //
            // Create Car Object
            //
            Car car;
            memset(&car, 0, sizeof(Car));
            strcpy(car.lastOilChangeDate, "2016");
            strcpy(car.maker.name, "Fabrikam");
            strcpy(car.maker.style, "sedan");
            car.maker.year = 2014;
            car.state.maxSpeed = 100;
            car.state.softwareVersion = 1;
            strcpy(car.state.vanityPlate, "1T1");

#ifdef CONTENT_TYPE_CBOR
            uint8_t reportedProperties[CBOR_BUFFER_SIZE];
            serializeToCBOR(&car, reportedProperties, CBOR_BUFFER_SIZE);
            printf("Size of encoded CBOR: %zu\n", strlen(reportedProperties));
#elif defined CONTENT_TYPE_JSON
            unsigned char* reportedProperties;
            serializeToJSON(&car, &reportedProperties); // internal malloc
            printf("Size of encoded JSON: %zu\n", strlen(reportedProperties));
#endif // CONTENT TYPE

            //
            // Send and receive messages from IoT Hub
            //
            (void)IoTHubDeviceClient_GetTwinAsync(iotHubClientHandle, getTwinAsyncCallback, NULL);
            ThreadAPI_Sleep(1000);

            (void)IoTHubDeviceClient_SendReportedState(iotHubClientHandle, reportedProperties, strlen(reportedProperties), deviceReportedPropertiesTwinCallback, NULL);
            ThreadAPI_Sleep(1000);

            (void)IoTHubDeviceClient_SetDeviceTwinCallback(iotHubClientHandle, deviceDesiredPropertiesTwinCallback, &car);
            ThreadAPI_Sleep(1000);

            (void)IoTHubDeviceClient_SetDeviceMethodCallback(iotHubClientHandle, deviceMethodCallback, NULL);
            ThreadAPI_Sleep(1000);

            //
            // Exit
            //
            (void)printf("Wait for desired properties update or direct method from service. Press any key to exit sample.\r\n");
            (void)getchar();

            IoTHubDeviceClient_Destroy(iotHubClientHandle);
#ifdef CONTENT_TYPE_JSON
            free(reportedProperties);
#endif // CONTENT_TYPE_JSON

        }

        IoTHub_Deinit();
    }
}

int main(void)
{
    iothub_client_device_twin_and_methods_sample_run();
    return 0;
}
