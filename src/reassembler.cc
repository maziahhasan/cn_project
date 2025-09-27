#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
// Mark end of stream if this chunk is flagged
  if ( is_last_substring ) {
    last_seen_ = true;
    last_index_ = first_index + data.size();
  }

  
// Special case: if stream is empty and last substring is at 0
if (last_seen_ && next_index_ == last_index_) {
  output_.writer().close();
  return; // we’re done
}

   // --- Trim left: drop data before next_index_ ---
  if ( first_index + data.size() <= next_index_ ) {
    return; // whole chunk already written
  }
  if ( first_index < next_index_ ) {
    size_t cut = next_index_ - first_index;
    data.erase( 0, cut );
    first_index = next_index_;
  }

   // --- Trim right: respect ByteStream capacity ---
  uint64_t cap = output_.writer().available_capacity();
  if ( first_index >= next_index_ + cap ) {
    return; // can’t fit anything
  }
  if ( first_index + data.size() > next_index_ + cap ) {
    data.resize( ( next_index_ + cap ) - first_index );
  }

   // --- Store substring ---
  if ( !data.empty() ) {
    pending_[first_index] = data;
  }

  // --- Try to flush contiguous bytes into ByteStream ---
  while ( true ) {
    auto it = pending_.find( next_index_ );
    if ( it == pending_.end() ) break;

    // Write this chunk
    output_.writer().push( it->second );
    next_index_ += it->second.size();
    pending_.erase( it );
  }

  // --- Close if we’ve reached the declared end ---
  if ( last_seen_ && next_index_ == last_index_ ) {
    output_.writer().close();
  }







  debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t total = 0;
  for (const auto& [idx, chunk] : pending_) {
    total += chunk.size();
  }
    debug( "unimplemented count_bytes_pending() called" );

  return total;

}
