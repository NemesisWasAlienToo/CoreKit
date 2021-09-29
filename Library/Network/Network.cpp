#pragma once

namespace Network
{
    enum AddressFamily
        {
            AnyFamilyAddress = AF_UNSPEC,
            // LocalAddress = AF_LOCAL,
            // BluetoothAddress = AF_BLUETOOTH,
            // NFCAddressAddress = AF_NFC,
            // CANAddress = AF_CAN,
            UnixAddress = AF_UNIX,
            IPv4Address = AF_INET,
            IPv6Address = AF_INET6,
        };

    enum ProtocolFamily
    {
        AnyProtocol = PF_UNSPEC,
        // LocalProtocol = PF_LOCAL,
        // BluetoothProtocol = PF_BLUETOOTH,
        // NFCProtocol = PF_NFC,
        // CANProtocol = PF_CAN,
        UnixProtocol = PF_UNIX,
        IPv4Protocol = PF_INET,
        IPv6Protocol = PF_INET6,
    };

    enum SocketType
    {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM,
        // RawSocket = SOCK_RAW,
    };
}