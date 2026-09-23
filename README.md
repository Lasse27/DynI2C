# DynI2C - Dynamic I2C

DynI2C is a small framework based on the I2C communication structure. It aims to simplify the retrieval of fields with dynamic sizes by adding a kind of protocol layer on top of conventional I2C communication.

The project is part of a larger project aimed at developing an autonomous robot. DynI2C is used there for communication between various ESP32 and Raspberry Pi devices. This is precisely why the framework was tested and developed exclusively for these two devices (specifically the ESP32-S3 and Raspberry Pi 4B).

# Usage

Various examples of how to use the protocol can be found in the `lib/DynI2C/examples` subfolder.

# Packet Structure

The protocol essentially consists of three different packets sent by the master:

1. GETMETA – For retrieving metadata about a register.
2. GETDATA – For retrieving data from a register.
3. SETDATA – For setting data in a register.

and two different response packets sent by the slave.

4. META – Metadata about a register
5. DATA – Data from a register

The following diagram illustrates the structure of the various packets.


# Protocol Flow

The basic flow of I2C communication remains essentially the same. There is still one master and multiple slave devices.

The master continues to dictate the flow of communication and the requested values. However, whenever a client requests a value, more than one I2C action may now be performed. The following actions can be or will be performed when a value is requested:

- If the client has never been addressed before, the client handle is dynamically created and cached in the master.

- If the client’s specific key/register has never been accessed before, the metadata for this register is first retrieved via a GETMETA packet and stored, unless the metadata explicitly specifies otherwise, e.g., the DYNAMIC_SIZE flag is set.

- By retrieving the metadata, the master no longer needs to explicitly know the size of the client’s fields but can cache this data dynamically.

- If the metadata query is performed again with every data request, fields of variable size can also be queried.

The following diagram illustrates the protocol flow in three different scenarios.


# Get Involved

Feel free to open an issue or pull request with new suggestions as they arise.