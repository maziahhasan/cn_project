#include "wrapping_integers.hh"
#include "debug.hh"
#include <cmath>
#include <cstdint>

using namespace std;

// Convert an absolute sequence number (64-bit) to a 32-bit wrapped seqno
Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  uint64_t sum = n + zero_point.raw_value_;
  return Wrap32 { static_cast<uint32_t>( sum % ( 1ull << 32 ) ) };
}

// Convert a wrapped seqno back to the absolute seqno nearest to the checkpoint
uint64_t Wrap32::unwrap(Wrap32 zero_point, uint64_t checkpoint) const {
    const uint64_t mod = 1ull << 32;

    // Offset between this value and zero_point (mod 2^32)
    uint64_t offset = (static_cast<uint64_t>(raw_value_) + mod - static_cast<uint64_t>(zero_point.raw_value_)) % mod;

    // Base candidate near checkpoint
    uint64_t candidate = (checkpoint & ~(mod - 1)) + offset;

    // Adjust by ±2^32 if a wrapped version is closer to the checkpoint
    if (candidate + (mod >> 1) < checkpoint)
        candidate += mod;
    else if (checkpoint + (mod >> 1) < candidate && candidate >= mod)
        candidate -= mod;

    return candidate;
}

