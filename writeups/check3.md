My name:

Maziah hasan 23L-0795

I collaborated with:

Anoosha khan 23L-0835


This checkpoint took me about:

6-7 hours to do.

Program Structure and Design of the TCPSender

My TCPSender implementation manages outgoing TCP segments using a structured approach that tracks sequence numbers, retransmission timers, and outstanding messages.

The key data structures used include:

std::deque<Outstanding> — to store unacknowledged (outstanding) TCP segments with their starting sequence number and length.

ByteStream — to manage the outgoing data from the application layer.

Variables like next_seqno_, bytes_in_flight_, receiver_ackno_, and receiver_window_ — to track TCP state.

Wrap32 — for converting between absolute and wrapped sequence numbers.

The design focuses on correctness and simplicity:

Each outgoing segment is constructed and recorded using send_segment().

fill_window() handles both data transmission and flag management (SYN, FIN, RST).

A retransmission timer (RTO_) doubles after each timeout, ensuring TCP’s exponential backoff behavior.

On receiving ACKs, the sender removes acknowledged segments from the outstanding queue and resets its timer.

Alternative Designs Considered:
One alternative was to store only unacknowledged byte ranges instead of entire messages. While this could reduce memory use, it would complicate retransmissions and make the implementation harder to debug.

Benefits:

Clear state management.

Easy retransmission logic.

Readable and modular code.

Weaknesses:

Slightly higher memory use due to storing whole messages.

More data copying than an optimized TCP stack.

Overall, the chosen design balances clarity and correctness — ideal for this educational implementation.

Report from the Hands-on Component

I verified the implementation by running all provided tests (check3).
Special attention was given to RST-handling tests, window filling, and retransmission logic.
Debug statements (via debug()) were used to trace SYN/FIN/RST flag behavior and validate retransmission timing.

Implementation Challenges

Understanding when to set and clear the retransmission timer.

Properly handling RST in both directions:

When the local stream errors → send RST.

When a peer’s RST is received → set local error flags.

Getting sequence number wrapping (Wrap32) correct for all edge cases.

Ensuring fill_window() respects receiver window limits and does not oversend.

Debugging failing tests that were sensitive to flag ordering or unacknowledged bytes.

Remaining Bugs

All tests for Checkpoint 3 now pass.
No known bugs remain at this stage.

Optional Reflections

Unexpected Difficulty: Handling the interaction between retransmissions and the RST flag was trickier than expected.

Could improve the lab by: Providing a small visual flow diagram of sender/receiver interactions would make debugging much easier.

Surprised by: How small changes in flag order or acknowledgment logic could affect multiple tests.