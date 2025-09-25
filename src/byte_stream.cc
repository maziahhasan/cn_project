#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_(capacity),
    buffer_(),
    total_pushed_(0),
    total_popped_(0),
    closed_(false) {}

void Writer::push( string data )
{
  // Only push as many as available_capacity allows
  uint64_t can_write = available_capacity();
  uint64_t to_write = std::min<uint64_t>(can_write, data.size());

  for (uint64_t i = 0; i < to_write; i++) {
    buffer_.push_back(data[i]);
  }

  total_pushed_ += to_write;
}

void Writer::close()
{
  closed_=true;
}

bool Writer::is_closed() const
{
  return closed_; // Your code here.
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_.size(); // Your code here.
}

uint64_t Writer::bytes_pushed() const
{
  return total_pushed_; // Your code here.
}

string_view Reader::peek() const
{
  if (buffer_.empty()) {
    return {};
  }


  // Return a view of continuous memory (deque isn't guaranteed contiguous, but
  // in CS144 starter they test small chunks — string_view over deque won't work directly)
  // To fix: store data in std::string instead of std::deque<char>
  // For now, return first contiguous segment.
  return std::string_view(buffer_.data(), buffer_.size());
}

void Reader::pop( uint64_t len )
{
  uint64_t to_pop = std::min<uint64_t>(len, buffer_.size());
  buffer_.erase(0, static_cast<size_t>(to_pop));
  total_popped_ += to_pop;
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

