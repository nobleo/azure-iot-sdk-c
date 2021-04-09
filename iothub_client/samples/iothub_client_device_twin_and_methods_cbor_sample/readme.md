# Run a simple Device Twin and Direct Methods using CBOR sample

## Introduction

**About this document**

This document describes how to build and run this sample application on the Linux and Windows platforms. This multi-step process includes:

<a name="Step-1-Prerequisites"></a>

## Step 1: Prerequisites

You should have the following items ready before beginning the process:

-   [Prepare your development environment, IoT Hub, and Device](https://github.com/Azure/azure-iot-sdk-c/tree/master/iothub_client/samples#how-to-compile-and-run-the-samples)


## Step 2: Install Intel's tinycbor library.
Intel/tinycbor is [licenesed](https://github.com/intel/tinycbor/blob/master/LICENSE) under the MIT License.

Linux:

1. Run the folling commands:

    ```
    git clone https://github.com/intel/tinycbor.git
    cd tinycbor
    make
    sudo make install
    ```

2.  Please check your `/usr/local/lib` directory and update permissions on the installed libraries if needed.

Windows:

1.  If using an x86 system, open the Developer Command Prompt fro Visual Studio. If using an x64 system, open the x64 Native Tools Command Prompt for Visual Studio.

2.  Run the following commands:

    ```
    git clone https://github.com/intel/tinycbor.git
    cd tinycbor
    NMAKE /F Makefile.nmake
    ```

3.  Update your Path environment variable to include the tinycbor directory.

<a name="Step-2-Build"></a>

## Step 3: Build and Run the sample

Follow [these instructions](https://github.com/Azure/azure-iot-sdk-c/blob/master/doc/devbox_setup.md) to build and run the sample for Linux or Windows.

NOTE: If intel/tinycbor is installed on Windows as an x86 library, be sure to use the command `cmake .. -A Win32` when building the CMake files.
