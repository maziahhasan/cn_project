#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <functional>

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) )
    , isn_( isn )
    , initial_RTO_ms_( initial_RTO_ms )
    , next_seqno_( 0 )
    , bytes_in_flight_( 0 )
    , outstanding_()
    , // ? ADDED (fixes effc++)
    receiver_ackno_( 0 )
    , receiver_window_( 1 )
    , syn_sent_( false )
    , fin_sent_( false )
    , RTO_( initial_RTO_ms )
    , timer_elapsed_ms_( 0 )
    , timer_running_( false )
    , consecutive_retx_( 0 )
  {}


  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors (for tests)
  uint64_t sequence_numbers_in_flight() const;
  uint64_t consecutive_retransmissions() const;

  const Writer& writer() const { return input_.writer(); }
  const Reader& reader() const { return input_.reader(); }
  Writer& writer() { return input_.writer(); }

private:
  struct Outstanding
  {
    TCPSenderMessage msg {};
    uint64_t seq_start { 0 };
    uint64_t len { 0 };
  };

  // state (? order matches initialization order)
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  // sending bookkeeping
  uint64_t next_seqno_;
  uint64_t bytes_in_flight_;
  std::deque<Outstanding> outstanding_;

  // receiver view
  uint64_t receiver_ackno_;
  uint64_t receiver_window_;

  // SYN/FIN flags
  bool syn_sent_;
  bool fin_sent_;

  // retransmission timer
  uint64_t RTO_;
  uint64_t timer_elapsed_ms_;
  bool timer_running_;
  uint64_t consecutive_retx_;

  // helpers
  void send_segment( TCPSenderMessage& msg, const TransmitFunction& transmit );
  void fill_window( const TransmitFunction& transmit );
  void start_timer_if_needed();
  void stop_timer();
};
