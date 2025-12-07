#include "network_interface.hh"
#include "../util/helpers.hh"

using namespace std;

static constexpr size_t ARP_TTL_MS      = 30'000; // 30 seconds
static constexpr size_t ARP_RETRY_MS    = 5'000;  // retry ARP every 5 seconds

NetworkInterface::NetworkInterface(string name,
                                   shared_ptr<OutputPort> port,
                                   const EthernetAddress& hw,
                                   const Address& ip)
    : name_(move(name)),
      port_(move(port)),
      hw_address_(hw),
      ip_address_(ip),
      outgoing_frames_(),
      datagrams_in_(),
      arp_cache_(),
      arp_timer_(),
      pending_()
{}

NetworkInterface::NetworkInterface(const EthernetAddress& hw,
                                   const Address& ip)
    : name_(),
      port_(nullptr),
      hw_address_(hw),
      ip_address_(ip),
      outgoing_frames_(),
      datagrams_in_(),
      arp_cache_(),
      arp_timer_(),
      pending_()
{}

void NetworkInterface::transmit(const EthernetFrame& f) {
    if (port_) {
        port_->transmit(*this, f);
    } else {
        outgoing_frames_.push(f);
    }
}

void NetworkInterface::send_datagram(const InternetDatagram& dgram,
                                     const Address& next_hop)
{
    uint32_t nh = next_hop.ipv4_numeric();

    // If ARP cache contains entry → send immediately
    if (auto it = arp_cache_.find(nh); it != arp_cache_.end()) {
        EthernetFrame frame;
        frame.header.src  = hw_address_;
        frame.header.dst  = it->second.mac;
        frame.header.type = EthernetHeader::TYPE_IPv4;
        frame.payload     = serialize(dgram);
        return transmit(frame);
    }

    // Cache miss → buffer packet
    pending_[nh].push_back(dgram);

    // Check if we are allowed to send ARP request
    bool can_send = !arp_timer_.count(nh) || arp_timer_[nh] >= ARP_RETRY_MS;

    if (can_send) {
        send_arp(nh);
        arp_timer_[nh] = 0;
    }
}

std::optional<EthernetFrame> NetworkInterface::recv_frame(const EthernetFrame& frame)
{
    const auto& hdr = frame.header;

    if (hdr.dst != hw_address_ && hdr.dst != ETHERNET_BROADCAST)
        return nullopt;

    // IPv4 packet
    if (hdr.type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram d;
        if (parse(d, frame.payload)) {
            datagrams_in_.push(d);
        }
        return nullopt;
    }

    // ARP packet
    if (hdr.type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp;
        if (!parse(arp, frame.payload))
            return nullopt;

        uint32_t sender_ip = arp.sender_ip_address;
        EthernetAddress sender_mac = arp.sender_ethernet_address;

        // Update ARP cache
        arp_cache_[sender_ip] = { sender_mac, ARP_TTL_MS };
        arp_timer_.erase(sender_ip);

        // Flush queued datagrams
        if (auto it = pending_.find(sender_ip); it != pending_.end()) {
            for (const auto& p : it->second) {
                EthernetFrame out;
                out.header.src  = hw_address_;
                out.header.dst  = sender_mac;
                out.header.type = EthernetHeader::TYPE_IPv4;
                out.payload     = serialize(p);
                transmit(out);
            }
            pending_.erase(it);
        }

        // Reply to request if it's for us
        if (arp.opcode == ARPMessage::OPCODE_REQUEST &&
            arp.target_ip_address == ip_address_.ipv4_numeric()) {

            EthernetFrame reply;
            reply.header.src = hw_address_;
            reply.header.dst = sender_mac;
            reply.header.type = EthernetHeader::TYPE_ARP;

            ARPMessage msg;
            msg.opcode = ARPMessage::OPCODE_REPLY;
            msg.sender_ip_address = ip_address_.ipv4_numeric();
            msg.sender_ethernet_address = hw_address_;
            msg.target_ip_address = sender_ip;
            msg.target_ethernet_address = sender_mac;

            reply.payload = serialize(msg);
            transmit(reply);
        }

        return nullopt;
    }

    return nullopt;
}

void NetworkInterface::tick(size_t ms)
{
    // Age ARP cache
    for (auto it = arp_cache_.begin(); it != arp_cache_.end();) {
        if (it->second.ttl_ms <= ms) {
            it = arp_cache_.erase(it);
        } else {
            it->second.ttl_ms -= ms;
            ++it;
        }
    }

    // Age ARP retry timers and resend if needed
    for (auto& t : arp_timer_) {
        t.second += ms;

        if (t.second >= ARP_RETRY_MS && pending_.count(t.first)) {
            pending_[t.first].clear();   // flush old
            send_arp(t.first);
            t.second = 0;
        }
    }
}

std::optional<EthernetFrame> NetworkInterface::maybe_send()
{
    if (outgoing_frames_.empty())
        return nullopt;

    EthernetFrame f = outgoing_frames_.front();
    outgoing_frames_.pop();
    return f;
}

void NetworkInterface::send_arp(uint32_t ip)
{
    ARPMessage req;
    req.opcode = ARPMessage::OPCODE_REQUEST;
    req.sender_ip_address = ip_address_.ipv4_numeric();
    req.sender_ethernet_address = hw_address_;
    req.target_ip_address = ip;

    EthernetFrame frame;
    frame.header.src  = hw_address_;
    frame.header.dst  = ETHERNET_BROADCAST;
    frame.header.type = EthernetHeader::TYPE_ARP;
    frame.payload     = serialize(req);

    transmit(frame);
}
