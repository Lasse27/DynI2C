# DynI2C - Dynamic I2C

DynI2C is a small framework based on the I2C communication structure. It aims to simplify the retrieval of fields with dynamic sizes by adding a kind of protocol layer on top of conventional I2C communication.

The project is part of a larger project aimed at developing an autonomous robot. DynI2C is used there for communication between various ESP32 and Raspberry Pi devices. This is precisely why the framework was tested and developed exclusively for these two devices (specifically the ESP32-S3 and Raspberry Pi 4B).

# Usage

Various examples of how to use the protocol can be found in the `lib/DynI2C/examples` subfolder.

# Packet Structure

The protocol essentially consists of 3 different packets sent by the master:

1. GETMETA
2. GETDATA
3. SETDATA

and 2 different response packets sent by the slave.

4. META
5. DATA

The following diagram illustrates the structure of the various packets.

# Protocol Flow

The basic flow of I2C communication remains essentially the same.
However, before requesting a register or field, the framework queries the client for metadata about that field and caches it on the master.
The following diagram illustrates the protocol flow in three different scenarios.

# Get Involved

Feel free to open a pull request with new suggestions as they arise.