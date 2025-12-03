#include "network_interface.hh"
#include "arp_message.hh"
#include "ipv4_datagram.hh"
#include "ethernet_frame.hh"
#include "parser.hh"

#include <unordered_map>
#include <iostream>

using namespace std;

static const size_t ARP_TTL = 30'000;      // 30 seconds
static const size_t ARP_COOLDOWN = 5'000;  // 5 seconds

static const EthernetAddress BROADCAST = {255,255,255,255,255,255};

void NetworkInterface::send_datagram(const InternetDatagram &dgram,
                                     const Address &next_hop)
{
    uint32_t ip = next_hop.ipv4_numeric();

    // If we know the MAC → send directly
    if (arp_cache_.count(ip) && arp_cache_[ip].expires_at > time_ms_) {

        EthernetFrame f;
        f.header().src = ethernet_address_;
        f.header().dst = arp_cache_[ip].mac;
        f.header().type = EthernetHeader::TYPE_IPv4;

        // Serialize datagram into BufferList
        f.payload() = dgram.serialize();

        transmit(f);
        return;
    }

    // Otherwise queue it
    waiting_[ip].push_back(dgram);

    // Cooldown: don't spam ARP
    if (!last_arp_request_.count(ip) ||
        last_arp_request_[ip] + ARP_COOLDOWN <= time_ms_) 
    {
        last_arp_request_[ip] = time_ms_;

        ARPMessage req;
        req.opcode = ARPMessage::OPCODE_REQUEST;
        req.hardware_type = ARPMessage::TYPE_ETHERNET;
        req.protocol_type = ARPMessage::TYPE_IPv4;
        req.hardware_address_size = 6;
        req.protocol_address_size = 4;

        req.sender_ethernet_address = ethernet_address_;
        req.sender_ip_address = ip_address_.ipv4_numeric();
        req.target_ethernet_address = {};
        req.target_ip_address = ip;

        EthernetFrame f;
        f.header().src = ethernet_address_;
        f.header().dst = BROADCAST;
        f.header().type = EthernetHeader::TYPE_ARP;
        f.payload() = req.serialize();

        transmit(f);
    }
}

void NetworkInterface::recv_frame(EthernetFrame frame)
{
    // Drop frames not for us
    if (!(frame.header().dst == ethernet_address_ ||
          frame.header().dst == BROADCAST))
        return;

    // IPv4
    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram d;
        if (d.parse(frame.payload()) == ParseResult::NoError)
            datagrams_received_.push(d);
        return;
    }

    // ARP
    if (frame.header().type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp;
        if (arp.parse(frame.payload()) != ParseResult::NoError)
            return;

        uint32_t sender_ip = arp.sender_ip_address;
        EthernetAddress sender_mac = arp.sender_ethernet_address;

        // Learn MAC
        arp_cache_[sender_ip] = { sender_mac, time_ms_ + ARP_TTL };

        // If this ARP solves waiting packets → send them
        if (waiting_.count(sender_ip)) {
            for (auto &d : waiting_[sender_ip]) {
                EthernetFrame f;
                f.header().src = ethernet_address_;
                f.header().dst = sender_mac;
                f.header().type = EthernetHeader::TYPE_IPv4;
                f.payload() = d.serialize();
                transmit(f);
            }
            waiting_.erase(sender_ip);
        }

        // If it's a request for US → reply
        if (arp.opcode == ARPMessage::OPCODE_REQUEST &&
            arp.target_ip_address == ip_address_.ipv4_numeric())
        {
            ARPMessage reply;
            reply.opcode = ARPMessage::OPCODE_REPLY;
            reply.hardware_type = ARPMessage::TYPE_ETHERNET;
            reply.protocol_type = ARPMessage::TYPE_IPv4;
            reply.hardware_address_size = 6;
            reply.protocol_address_size = 4;

            reply.sender_ethernet_address = ethernet_address_;
            reply.sender_ip_address = ip_address_.ipv4_numeric();
            reply.target_ethernet_address = sender_mac;
            reply.target_ip_address = sender_ip;

            EthernetFrame out;
            out.header().src = ethernet_address_;
            out.header().dst = sender_mac;
            out.header().type = EthernetHeader::TYPE_ARP;
            out.payload() = reply.serialize();

            transmit(out);
        }
    }
}

void NetworkInterface::tick(size_t ms_since_last_tick)
{
    time_ms_ += ms_since_last_tick;

    // Remove expired ARP entries
    vector<uint32_t> expired;
    for (auto &[ip, entry] : arp_cache_) {
        if (entry.expires_at <= time_ms_)
            expired.push_back(ip);
    }
    for (uint32_t ip : expired)
        arp_cache_.erase(ip);
}
