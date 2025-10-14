Checkpoint 2 Writeup
====================

My name: Maziah Hasan

My Roll No: 23L-0795

I collaborated with: Anooshay Khan (23L-0835)

I would like to thank/reward these classmates for their help: [list roll numbers here if any]

This lab took me about 7 hours to do. I [did/did not] attend the lab session.

Describe Wrap32 and TCPReceiver structure and design:

Wrap32:
The Wrap32 class handles conversion between 64-bit absolute sequence numbers and 32-bit wrapping sequence numbers used in TCP. It maintains modular arithmetic to deal with sequence number overflow.

Data Structures Used:
A 32-bit unsigned integer to store the wrapped sequence number.

Approach:
The wrap() function converts an absolute 64-bit sequence number into its 32-bit counterpart by applying modulo 2^32.
The unwrap() function converts a wrapped 32-bit sequence number back into a 64-bit absolute sequence number, choosing the value closest to a given checkpoint to maintain continuity.

Alternative Designs:
An alternative could be using a signed difference-based comparison to detect wrap-around. However, that increases complexity and potential for off-by-one bugs.

Benefits and Weaknesses:
Benefits: Simple modular logic, easy to reason about, and low overhead.
Weaknesses: Edge cases near wrap-around boundaries require careful handling.

TCPReceiver:
The TCPReceiver class is responsible for reassembling received TCP segments, tracking sequence numbers, and providing data to the stream writer in order. It manages the SYN/FIN flags, buffer capacity, and calculates the appropriate acknowledgment number.

Data Structures Used:

A StreamReassembler object to handle out-of-order segments.

Boolean flags for SYN and FIN reception.

Variables for sequence number tracking (ISN, checkpoint, etc.).

Approach:
The receiver uses the initial sequence number (ISN) from the SYN segment to translate incoming segment sequence numbers into absolute indices. It then feeds payloads into the StreamReassembler, which pushes contiguous data to the output stream. Once a FIN is received and all bytes are assembled, the stream closes.

Alternative Designs:
Could integrate reassembly logic directly into TCPReceiver, but that would increase complexity and reduce modularity. Keeping it separate in StreamReassembler makes the system cleaner.

Benefits and Weaknesses:
Benefits: Clear separation of concerns between reassembly and sequence handling; simpler debugging and testing.
Weaknesses: Requires careful synchronization between TCPReceiver and StreamReassembler to avoid off-by-one sequence errors.

Performance:
The design ensures O(1) operations for segment arrival and acknowledgment updates (ignoring reassembly complexity). Empirical performance is efficient for lab-scale testing.

Implementation Challenges:

Handling the off-by-one issues in sequence number translation between wrapped and absolute form.

Correctly determining the acknowledgment number when SYN and FIN flags appear in edge cases.

Ensuring the writer closes (is_closed = true) immediately after receiving a segment with both SYN and FIN set.

Remaining Bugs:



Optional Notes:
I had unexpected difficulty with: Off-by-one behavior during unwrapping sequence numbers near wrap boundaries.
I think you could make this lab better by: Adding more detailed debugging hints for tests like connect and reorder.
I was surprised by: How subtle the acknowledgment logic becomes once wrapping and checkpoint alignment are involved.
I’m not sure about: Whether the ackno should exist before SYN is received or not.
I made an extra test I think will be helpful in catching bugs: A test that sends a single segment containing both SYN and FIN to confirm the writer closes correctly.