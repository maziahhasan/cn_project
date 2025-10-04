#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
 // --- mark end of stream if flagged ---
  if (is_last_substring) {
    last_seen_ = true;
    last_index_ = first_index + data.size();
  }

  // --- special case: empty stream ends immediately ---
  if (last_seen_ && next_index_ == last_index_) {
    output_.writer().close();
    return;
  }

  // --- left trim: drop already written bytes ---
  if (first_index + data.size() <= next_index_) {
    return; // whole chunk is old
  }
  if (first_index < next_index_) {
    size_t cut = next_index_ - first_index;
    data.erase(0, cut);
    first_index = next_index_;
  }

  // --- right trim: respect capacity ---
  uint64_t cap = output_.writer().available_capacity();
  uint64_t stream_limit = next_index_ + cap;

  if (first_index >= stream_limit) {
    return; // completely beyond window
  }
  if (first_index + data.size() > stream_limit) {
    data.resize(stream_limit - first_index);
  }

  if (data.empty()) {
    return;
  }

  // --- merge with overlapping pending chunks ---
  auto it = pending_.lower_bound(first_index);

  // merge with previous if overlapping
  if (it != pending_.begin()) {
    auto prev = std::prev(it);
    uint64_t prev_end = prev->first + prev->second.size();
    if (prev_end >= first_index) {
      size_t overlap = prev_end - first_index;
      if (overlap < data.size()) {
        prev->second += data.substr(overlap);
      }
      first_index = prev->first;
      data = prev->second;
      pending_.erase(prev);
    }
  }

  // merge forward with any following overlapping chunks
  while (it != pending_.end() && it->first <= first_index + data.size()) {
    uint64_t overlap = (first_index + data.size()) - it->first;
    if (overlap < it->second.size()) {
      data += it->second.substr(overlap);
    }
    auto erase_it = it++;
    pending_.erase(erase_it);
  }

  // store merged chunk
  pending_[first_index] = data;

  // --- try flushing contiguous data into ByteStream ---
  while (true) {
    auto it2 = pending_.find(next_index_);
    if (it2 == pending_.end()) break;

    output_.writer().push(it2->second);
    next_index_ += it2->second.size();
    pending_.erase(it2);
  }

  // --- close if we've written the last byte ---
  if (last_seen_ && next_index_ == last_index_) {
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
