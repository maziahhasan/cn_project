#pragma once

#include <unordered_map>
#include <string>
#include <cstdint>
#include "arp_message.hh"
#include "ethernet_header.hh"
#include "ipv4_header.hh"

// Network interface class
class NetworkInterface
{
public:
    explicit NetworkInterface(const std::string& name);

    // Called periodically
    void tick(size_t ms);

    // Send ARP request
    void send_arp_request(uint32_t target_ip);

private:
    std::string _name;

    // Map IP -> MACAddress
    std::unordered_map<uint32_t, MACAddress> _arp_table;

    // ARP timers
    std::unordered_map<uint32_t, size_t> _arp_ttl;
};
