#include "byte_stream.hh"
#include <algorithm>

using namespace std;

/* Constructor */
ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity ), error_( false ), buffer_(), total_pushed_( 0 ), total_popped_( 0 ), closed_( false )
{}

/* Writer methods */
void Writer::push( string data )
{
  uint64_t can_write = available_capacity();
  uint64_t n = min<uint64_t>( can_write, data.size() );
  buffer_.append( data.substr( 0, n ) );
  total_pushed_ += n;
}

void Writer::close()
{
  closed_ = true;
}
bool Writer::is_closed() const
{
  return closed_;
}
uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_.size();
}
uint64_t Writer::bytes_pushed() const
{
  return total_pushed_;
}

/* Reader methods */
string_view Reader::peek() const
{
  return string_view( buffer_.data(), buffer_.size() );
}

void Reader::pop( uint64_t len )
{
  uint64_t n = min<uint64_t>( len, buffer_.size() );
  buffer_.erase( 0, n );
  total_popped_ += n;
}

bool Reader::is_finished() const
{
  return closed_ && buffer_.empty();
}
uint64_t Reader::bytes_buffered() const
{
  return buffer_.size();
}
uint64_t Reader::bytes_popped() const
{
  return total_popped_;
}
