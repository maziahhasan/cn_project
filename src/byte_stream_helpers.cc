#include "byte_stream.hh"
#include <stdexcept>

using namespace std;

void read( Reader& reader, uint64_t max_len, string& out )
{
  out.clear();

  while ( reader.bytes_buffered() and out.size() < max_len ) {
    auto view = reader.peek();

    if ( view.empty() ) {
      throw runtime_error( "Reader::peek() returned empty string_view" );
    }

    view = view.substr( 0, max_len - out.size() );
    out += view;
    reader.pop( view.size() );
  }
}

Reader& ByteStream::reader()
{
  static_assert( sizeof( Reader ) == sizeof( ByteStream ), "Add member vars to ByteStream base, not Reader." );
  return static_cast<Reader&>( *this );
}

const Reader& ByteStream::reader() const
{
  static_assert( sizeof( Reader ) == sizeof( ByteStream ), "Add member vars to ByteStream base, not Reader." );
  return static_cast<const Reader&>( *this );
}

Writer& ByteStream::writer()
{
  static_assert( sizeof( Writer ) == sizeof( ByteStream ), "Add member vars to ByteStream base, not Writer." );
  return static_cast<Writer&>( *this );
}

const Writer& ByteStream::writer() const
{
  static_assert( sizeof( Writer ) == sizeof( ByteStream ), "Add member vars to ByteStream base, not Writer." );
  return static_cast<const Writer&>( *this );
}
