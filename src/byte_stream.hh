#pragma once
#include <cstdint>
#include <string>
#include <string_view>

class Reader;
class Writer;

class ByteStream
{
public:
  explicit ByteStream( uint64_t capacity );
  Reader& reader();
  const Reader& reader() const;
  Writer& writer();
  const Writer& writer() const;

  void set_error() { error_ = true; }
  bool has_error() const { return error_; }

protected:
  uint64_t capacity_;
  bool error_ { false };
  std::string buffer_ {};
  uint64_t total_pushed_ { 0 };
  uint64_t total_popped_ { 0 };
  bool closed_ { false };
};

class Writer : public ByteStream
{
public:
  using ByteStream::ByteStream;
  void push( std::string data );
  void close();
  bool is_closed() const;
  uint64_t available_capacity() const;
  uint64_t bytes_pushed() const;
};

class Reader : public ByteStream
{
public:
  using ByteStream::ByteStream;
  std::string_view peek() const;
  void pop( uint64_t len );
  bool is_finished() const;
  uint64_t bytes_buffered() const;
  uint64_t bytes_popped() const;
};

void read( Reader& reader, uint64_t max_len, std::string& out );
