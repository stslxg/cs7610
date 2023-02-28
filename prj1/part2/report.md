# Report for Lab 1 Part 2 - Xuangui Huang

In this part I implement the Chandy-Lamport algorithm in an asynchronous way.


# Files
Additionally,
 - `utils_tcp.cpp/h` implements **TCP** communications;
 - `cl-main.cpp/h` implements the algorithm.
And there are also some other `.h` files.

# Designs
The other processes will send the snapshot to the initiator after recording all the channels. Internal states and channels are sent in the original message form, with a new `SnapshotMessge` to denote the channel id and the end of sending.

For communications, the process will connect to processes with higher ids as clients, and listen then accept connections from processes with lower ids as server. There are one thread for sending and one for receiving for each host.