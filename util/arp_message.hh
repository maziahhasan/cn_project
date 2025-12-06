#pragma once

#include "ethernet_header.hh"
#include "ipv4_header.hh"
#include "parser.hh"

// MACAddress type alias
using MACAddress = EthernetAddress;

// [ARP](\ref rfc::rfc826) message
struct ARPMessage
{
    static constexpr size_t LENGTH = 28;
    static constexpr uint16_t TYPE_ETHERNET = 1;
    static constexpr uint16_t OPCODE_REQUEST = 1;
    static constexpr uint16_t OPCODE_REPLY = 2;

    uint16_t hardware_type = TYPE_ETHERNET;
    uint16_t protocol_type = EthernetHeader::TYPE_IPv4;
    uint8_t hardware_address_size = sizeof(EthernetHeader::src);
    uint8_t protocol_address_size = sizeof(IPv4Header::src);
    uint16_t opcode {};

    EthernetAddress sender_ethernet_address {};
    uint32_t sender_ip_address {};

    EthernetAddress target_ethernet_address {};
    uint32_t target_ip_address {};

    std::string to_string() const;
    bool supported() const;
    void parse(Parser& parser);
    void serialize(Serializer& serializer) const;
};
