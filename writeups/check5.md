Checkpoint 5 Writeup
====================

My name: Maziah Hasan 23L-0795

My SUNet ID: [your sunetid here]

I collaborated with: Anoosha Khan 23L-0835

I would like to thank/reward these classmates for their help: [list sunetids here]

This checkpoint took me about 10 hours to do. I [did/did not] attend the lab session.

Program Structure and Design of the NetworkInterface

For this checkpoint, I designed my NetworkInterface around three core ideas:
(1) keep the ARP logic deterministic,
(2) keep queues simple, and
(3) make the code easy to reason about under tests.

### Data structures I used

unordered_map<uint32_t, ARPEntry> arp_cache_
Stores the learned IP → MAC mappings, along with a TTL countdown.
This gives O(1) lookup and simple expiration.

unordered_map<uint32_t, size_t> arp_timer_
Tracks how long it has been since the last ARP request to each destination.
This prevents spamming ARP packets.

unordered_map<uint32_t, vector<InternetDatagram>> pending_
Holds datagrams waiting for ARP resolution.
I chose a vector because:

order doesn’t matter,

bounded queue size,

low overhead.

queue<EthernetFrame> outgoing_frames_
Stores frames produced when no OutputPort exists.

queue<InternetDatagram> datagrams_in_
Stores delivered IPv4 datagrams for the router to collect.

Main approach taken

My implementation follows a simple event-loop style:

send_datagram()

If ARP mapping exists → immediately build and send Ethernet frame.

If not → buffer the datagram and trigger an ARP request (rate-limited).

recv_frame()

Drop frames not addressed to us.

If IPv4 → parse and enqueue.

If ARP request:

Learn sender mapping.

If the request is for our IP, send an ARP reply.

Flush queued packets for that sender immediately.

tick()

Decrease ARP TTL.

Remove expired ARP entries.

Increase ARP retry timers and resend requests when allowed.

Alternative designs considered

Using a queue instead of a vector for pending datagrams

A queue strictly preserves ordering, but makes clearing + resending trickier.

Vector gave me much simpler logic with negligible drawbacks here.

Storing ARP timers inside the ARP entry struct

I avoided this because ARP retry timing and ARP TTL are conceptually separate.

Using a global “last ARP time” for all IPs

This failed test cases where simultaneous ARP lookups are needed.

Benefits of my design

Predictable behavior under timing tests

Minimal code branching → easier debugging

O(1) ARP lookups

Clean separation of ARP logic and frame serialization

Weaknesses

vector for pending packets may reallocate on growth

Doesn’t handle packet storms (but assignment doesn’t require it)

No backpressure or queue trimming for huge waits

Implementation Challenges

Matching the exact expected ARP behavior from the autograder
Small timing details (retry interval, TTL handling) were easy to get wrong.

Ensuring serialization and deserialization happened in the correct order
A few bugs came from forgetting to call serialize() vs parse() properly.

Avoiding duplicate ARP requests
Needed a clean separation between "cache expired" and "retry timer expired".

Test inputs where ARP replies come out of order
Required extra care to flush pending packets correctly.

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
