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
uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  const uint64_t base = zero_point.raw_value_;
  const uint64_t val = raw_value_;
  const uint64_t mod = 1ull << 32;

  // offset of this seqno relative to zero_point, in 0 … 2^32-1
  const uint64_t offset = ( val + mod - base ) % mod;

  // candidate absolute seqno close to checkpoint
  uint64_t candidate = ( checkpoint & ~( mod - 1 ) ) + offset;

  // adjust if wrapping the other way is closer
  if ( candidate + ( mod >> 1 ) < checkpoint )
    candidate += mod;
  else if ( checkpoint + ( mod >> 1 ) < candidate )
    candidate -= mod;

  return candidate;
}
