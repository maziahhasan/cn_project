#include "router.hh"
#include "debug.hh"

#include <iostream>
#include <limits>

using namespace std;

void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // Save the route for later use [cite: 62]
  routing_table_.push_back( { route_prefix, prefix_length, next_hop, interface_num } );
}

void Router::route()
{
  // Go through all interfaces to route every incoming datagram [cite: 10, 76]
  for ( auto& current_interface : interfaces_ ) {
    
    // Note: Assuming `datagrams_received()` returns a reference to the queue of received datagrams
    auto& queue = current_interface->datagrams_received();

    while ( !queue.empty() ) {
      auto dgram = queue.front();
      queue.pop();

      // Check TTL: If 0 or 1, it will die after decrement. Drop it. 
      if ( dgram.header.ttl <= 1 ) {
        continue;
      }

      // Decrement TTL [cite: 86]
      dgram.header.ttl--;
      // Recompute checksum after modifying header (standard IPv4 requirement)
      dgram.header.compute_checksum();

      // Longest-Prefix Match (LPM) Algorithm [cite: 79]
      int best_match_index = -1;
      int max_prefix_len = -1;
      uint32_t dst_ip = dgram.header.dst;

      for ( size_t i = 0; i < routing_table_.size(); ++i ) {
        const auto& route = routing_table_[i];

        // Create mask. Watch out for shifting by 32 (Undefined Behavior) 
        // If length is 0, mask is 0. Otherwise, shift all 1s.
        uint32_t mask = ( route.prefix_length == 0 ) ? 0 : std::numeric_limits<uint32_t>::max() << ( 32 - route.prefix_length );

        // Check if most-significant bits match [cite: 82]
        if ( ( dst_ip & mask ) == ( route.route_prefix & mask ) ) {
          
          // Tie-breaker: choose longest prefix length [cite: 83]
          if ( route.prefix_length > max_prefix_len ) {
            max_prefix_len = route.prefix_length;
            best_match_index = i;
          }
        }
      }

      // If no route matched, drop the datagram [cite: 85]
      if ( best_match_index == -1 ) {
        continue;
      }

      // Determine next hop 
      // If route.next_hop has a value, use it. If empty, use datagram destination (direct).
      const auto& best_route = routing_table_[best_match_index];
      Address next_hop_addr = best_route.next_hop.has_value() 
                              ? best_route.next_hop.value() 
                              : Address::from_ipv4_numeric( dst_ip );

      // Send the modified datagram on the correct interface [cite: 53, 88]
      interface( best_route.interface_num )->send_datagram( dgram, next_hop_addr );
    }
  }
}