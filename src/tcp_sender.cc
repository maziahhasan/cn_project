#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"
#include <algorithm>
#include <string>

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return bytes_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retx_;
}

// Start timer if there is outstanding data and timer not already running.
void TCPSender::start_timer_if_needed()
{
  if ( !timer_running_ && !outstanding_.empty() ) {
    timer_running_ = true;
    timer_elapsed_ms_ = 0;
  }
}

// Stop the timer entirely.
void TCPSender::stop_timer()
{
  timer_running_ = false;
  timer_elapsed_ms_ = 0;
}

// send helper: transmit and record outstanding if it consumes sequence space
void TCPSender::send_segment( TCPSenderMessage& msg, const TransmitFunction& transmit )
{

   // ✅ Add this check:
  if (input_.reader().has_error() || input_.writer().has_error())
      msg.RST = true;

  transmit( msg );

  uint64_t len = msg.sequence_length();
  if ( len > 0 ) {
    Outstanding o;
    o.msg = msg;
    o.seq_start = next_seqno_;
    o.len = len;
    outstanding_.push_back( std::move( o ) );
    bytes_in_flight_ += len;
    next_seqno_ += len;
  } else {
    // zero-length (no seq space) messages are not tracked and do not advance next_seqno_
  }

  // Start timer if we sent data that can be retransmitted (and timer was not running)
  start_timer_if_needed();

  debug( "send_segment: len={}, SYN={}, FIN={}, bytes_in_flight={}",
         msg.sequence_length(),
         msg.SYN,
         msg.FIN,
         bytes_in_flight_ );
}

// Try to fill the window by creating and sending segments (SYN/FIN/payload)
void TCPSender::fill_window( const TransmitFunction& transmit )
{
  if ( fin_sent_ )
    return; // nothing more to send

  // Window special-case: if receiver advertises 0, treat as 1 for this call only (zero-window probe)
  uint64_t win_advertised = receiver_window_;
  if ( win_advertised == 0 )
    win_advertised = 1;

  // Keep sending until either window is full or there's no data to send
  while ( bytes_in_flight_ < win_advertised ) {
    TCPSenderMessage msg;
    msg.seqno = Wrap32::wrap( next_seqno_, isn_ );
    // If stream has errored, advertise RST on the outgoing segment immediately
if ( input_.reader().has_error() || input_.writer().has_error() ) {
    msg.RST = true;
}

    // SYN at connection start
    if ( !syn_sent_ ) {
      msg.SYN = true;
      syn_sent_ = true;
    }

    // remaining space for this segment (in sequence space)
    uint64_t used_for_flags = msg.SYN ? 1 : 0;
    uint64_t remaining_space = 0;
    if ( win_advertised > bytes_in_flight_ ) {
      // guard underflow
      uint64_t avail = win_advertised - bytes_in_flight_;
      if ( avail > used_for_flags )
        remaining_space = avail - used_for_flags;
      else
        remaining_space = 0;
    }

    // limit payload by MAX_PAYLOAD_SIZE
    uint64_t to_read = std::min<uint64_t>( remaining_space, TCPConfig::MAX_PAYLOAD_SIZE );

    // read bytes_to_send into payload using provided read(Reader&, max_len, std::string&)
    std::string payload;
    if ( to_read > 0 ) {
      ::read( input_.reader(), to_read, payload );
    }
    msg.payload = std::move( payload );

    // FIN if stream finished and there is space for FIN (consumes 1 sequence space)
    if ( !fin_sent_ && input_.reader().is_finished() ) {
      // can we fit a FIN? need to ensure bytes_in_flight_ + msg.sequence_length() + 1 <= win_advertised
      uint64_t would_be_len = msg.sequence_length() + 1; // include FIN
      if ( bytes_in_flight_ + would_be_len <= win_advertised ) {
        msg.FIN = true;
        fin_sent_ = true;
      }
    }

    // If this segment occupies no sequence numbers (no SYN, no payload, no FIN), break
    if ( msg.sequence_length() == 0 ) {
      break;
    }

    // send and record
    send_segment( msg, transmit );
    // loop continues: next_seqno_ was advanced in send_segment for non-empty segments
  }
}

void TCPSender::push( const TransmitFunction& transmit )
{
  fill_window( transmit );
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage msg;
  msg.seqno = Wrap32::wrap( next_seqno_, isn_ );

  // ✅ If the stream has errored, set RST flag
  if (input_.reader().has_error() || input_.writer().has_error())
      msg.RST = true;

  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // ✅ If the peer’s receiver sent RST, mark our stream as errored
  if ( msg.RST ) {
    input_.reader().set_error();
    input_.writer().set_error();
    return; // nothing more to do
  }

  if ( msg.ackno.has_value() ) {
    uint64_t abs_ack = msg.ackno->unwrap( isn_, receiver_ackno_ );

    if ( abs_ack <= next_seqno_ && abs_ack > receiver_ackno_ ) {
      receiver_ackno_ = abs_ack;

      while ( !outstanding_.empty() ) {
        Outstanding& front = outstanding_.front();
        uint64_t seg_end = front.seq_start + front.len;
        if ( seg_end <= receiver_ackno_ ) {
          bytes_in_flight_ -= front.len;
          outstanding_.pop_front();
        } else break;
      }

      RTO_ = initial_RTO_ms_;
      consecutive_retx_ = 0;
      if ( outstanding_.empty() )
        stop_timer();
      else {
        timer_running_ = true;
        timer_elapsed_ms_ = 0;
      }
    }
  }

  receiver_window_ = msg.window_size;
}


void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if ( !timer_running_ )
    return;
  if ( outstanding_.empty() ) {
    timer_running_ = false;
    return;
  }

  timer_elapsed_ms_ += ms_since_last_tick;

  if ( timer_elapsed_ms_ >= RTO_ ) {
    // retransmit earliest outstanding segment
    Outstanding& earliest = outstanding_.front();
    // Ensure retransmitted message carries RST if stream errored
if ( input_.reader().has_error() || input_.writer().has_error() ) {
    earliest.msg.RST = true;
}
    transmit( earliest.msg );

    // Only count/scale RTO if receiver window not zero
    if ( receiver_window_ > 0 ) {
      consecutive_retx_++;
      RTO_ *= 2;
    }

    // restart timer
    timer_elapsed_ms_ = 0;
  }
}
