#pragma once

#include "address.hh"
#include "ethernet_frame.hh"
#include "arp_message.hh"
#include "ipv4_datagram.hh"

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <queue>
#include <vector>
#include <string>
#include <memory>

class NetworkInterface {
public:
    class OutputPort {
    public:
        virtual void transmit(const NetworkInterface& nic, const EthernetFrame& frame) = 0;
        virtual ~OutputPort() = default;
    };

    NetworkInterface(std::string name,
                     std::shared_ptr<OutputPort> port,
                     const EthernetAddress& hw,
                     const Address& ip);

    // Constructor used when tests don't attach a port
    NetworkInterface(const EthernetAddress& hw,
                     const Address& ip);

    void send_datagram(const InternetDatagram& dgram, const Address& next_hop);

    std::optional<EthernetFrame> recv_frame(const EthernetFrame& frame);

    void tick(size_t ms_elapsed);

    std::optional<EthernetFrame> maybe_send();

    std::queue<InternetDatagram>& datagrams_received() { return datagrams_in_; }

    const std::string& name() const { return name_; }

private:
    struct ARPEntry {
        EthernetAddress mac{};
        size_t ttl_ms{};
    };

    std::string name_;
    std::shared_ptr<OutputPort> port_;
    EthernetAddress hw_address_;
    Address ip_address_;

    std::queue<EthernetFrame> outgoing_frames_;
    std::queue<InternetDatagram> datagrams_in_;

    std::unordered_map<uint32_t, ARPEntry> arp_cache_;               // IP → MAC + TTL
    std::unordered_map<uint32_t, size_t> arp_timer_;                 // IP → time since last ARP request
    std::unordered_map<uint32_t, std::vector<InternetDatagram>> pending_; // IP → waiting datagrams

    void transmit(const EthernetFrame& f);
    void send_arp(uint32_t ip);
};
