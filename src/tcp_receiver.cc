#include "tcp_receiver.hh"
#include "debug.hh"
#include "wrapping_integers.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST ) {
    reassembler_.reader().set_error();
    return;
  }

  // establish ISN (Initial Sequence Number)
  if ( message.SYN && !isn_.has_value() ) {
    isn_ = message.seqno;
  }

  if ( !isn_.has_value() )
    return; // ignore anything before SYN

  // unwrap sequence number to absolute
  uint64_t abs_seqno = message.seqno.unwrap( *isn_, reassembler_.writer().bytes_pushed() + 1 );

  // compute stream index (subtract 1 for SYN)
  uint64_t stream_index = abs_seqno - 1;

  // feed data into the Reassembler
  reassembler_.insert( stream_index, message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage msg {};

  if ( reassembler_.reader().has_error() ) {
    msg.RST = true;
    return msg;
  }

  // no SYN seen yet → no ackno
  if ( !isn_.has_value() )
    return msg;

  const auto& writer = reassembler_.writer();

  // bytes pushed into the output stream
  uint64_t bytes_pushed = writer.bytes_pushed();

  // compute acknowledgment number
  uint64_t ack_abs = bytes_pushed + 1; // +1 for SYN
  if ( writer.is_closed() )
    ack_abs += 1; // +1 for FIN

  msg.ackno = Wrap32::wrap( ack_abs, *isn_ );

  // window size = remaining capacity, capped to uint16_t
  msg.window_size = static_cast<uint16_t>( min<uint64_t>( UINT16_MAX, writer.available_capacity() ) );

  return msg;
}
