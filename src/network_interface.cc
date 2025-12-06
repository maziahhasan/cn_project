#include "network_interface.hh"
#include <iostream>

static constexpr size_t ARP_TTL = 30000;  // 30 seconds

NetworkInterface::NetworkInterface(const std::string& name)
    : _name(name), _arp_table(), _arp_ttl() // <-- initialize members here
{
}

// Tick function: remove expired ARP entries
void NetworkInterface::tick(size_t ms)
{
    for (auto it = _arp_ttl.begin(); it != _arp_ttl.end(); )
    {
        if (it->second <= ms)
        {
            _arp_table.erase(it->first);
            it = _arp_ttl.erase(it);
        }
        else
        {
            it->second -= ms;
            ++it;
        }
    }
}

// Example: send ARP request
void NetworkInterface::send_arp_request(uint32_t target_ip)
{
    ARPMessage arp{};
    arp.opcode = ARPMessage::OPCODE_REQUEST;
    arp.sender_ethernet_address = MACAddress{}; // Fill your MAC
    arp.sender_ip_address = 0;                  // Fill your IP
    arp.target_ip_address = target_ip;
    arp.target_ethernet_address = MACAddress{};

    // Serialize and send on your link
    std::cout << "Sending ARP request for IP: " << target_ip << "\n";
}
