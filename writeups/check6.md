Checkpoint 6 Writeup
====================

My name: Maziah Hasan 23L-0795

My SUNet ID: [your sunetid here]

I collaborated with: Anoosha khan 23L-0835

I would like to thank/reward these classmates for their help: [list sunetids here]

This checkpoint took me about 5 hours to do. I [did/did not] attend the lab session.

Program Structure and Design of the Router
------------------------------------------
### Data Structure
I implemented the routing table using a `std::vector` of a private helper struct 
called `RouteEntry`. This struct holds the four key pieces of information for a 
rule: the `route_prefix`, `prefix_length`, `next_hop` (optional), and the 
`interface_num`.

### Algorithm: Linear Scan
For the `route()` method, I utilized a linear scan O(N) approach. For every 
incoming datagram, the router iterates through the entire `routing_table_` vector. 
For each entry, it checks if the destination address matches the prefix. 

To determine a match, I generate a bitmask based on the `prefix_length`. I verify 
if `(destination & mask) == (route_prefix & mask)`. Among all matching routes, 
I track the one with the largest `prefix_length` (Longest Prefix Match).

### Design Trade-offs
I considered implementing a Trie (Prefix Tree), which would offer $O(L)$ lookup 
time (where $L=32$). However, I chose the `std::vector` approach for two reasons:
1. **Simplicity:** The vector approach is significantly less error-prone to 
   implement and debug given the short timeframe.
2. **Performance:** For the small number of routes in this lab (and many edge 
   routers), the overhead of pointer chasing in a Trie often exceeds the cost of 
   a linear scan over contiguous memory (which is cache-friendly).
   
The weakness is scalability; if the routing table grew to thousands of entries 
(like a core Internet router), this O(N) lookup would become a bottleneck.

Implementation Challenges
-------------------------
### undefined Behavior with Bit Shifts
One specific challenge was generating the subnet mask. In C++, shifting a 
32-bit integer by 32 bits (e.g., `0xFFFFFFFF << 32`) is undefined behavior. 
I initially encountered issues when the `prefix_length` was 0 (default route). 
I solved this by adding a conditional check: if `prefix_length` is 0, the mask 
is explicitly set to 0; otherwise, standard shifting logic is used.

### Direct vs. Indirect Routing
Understanding the `next_hop` logic required care. I had to ensure that if the 
`next_hop` optional was empty, the router interprets this as a "direct" 
connection, meaning the next hop IP is simply the destination IP of the 
datagram itself.

Remaining Bugs
--------------
None. The implementation passes all functionality tests, including the logic for 
TTL decrementing and checksum re-computation.

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]