# Report for Lab 1 Part 1 - Xuangui Huang

In this part I implement the ISIS total order multicast algorithm in an asynchronous way.

# Designs
A process's id is the index of its host in the host name file. For user-friendliness the program will always print its host name as in the host name file.

I have two additional message types: `UpMessage` and `AckSeqMessage`. Their usages will be explained in the following paragraphs.

Each process has two status:

 1. `STATUS_UP`: Initialize everything, setup sockets and resolve addresses. In this status it will also send to every process an `UpMessage` containing the receiving process's id. In this way it can get its own id without comparing IP addresses. It will also check if all the **machines** are up; if so, go into the next status. This is mainly implemented in `check_up()` in `main.cpp`. Note that a process might not be up even if its container is up; this case is handled since my program can handle package drops.
 2. `STATUS_MULTICAST`: In this status the process do the algorithm in the following ways:
	 - I have one thread (`receiver()` in `main.cpp`) for receiving all the messages, and one thread (`sender(id)` in `main.cpp`) per host for sending to that host.
	 - Message sending: when a message is to be sent to a process indexed as `id`, the program call `add_send_message(buf, send_len, id)`  to add the message into a queue `send_queues[id]` in `main.cpp`. Then the corresponding sender will check that queue and send it.
	 - Message managing: each data message is managed by a separated thread, so it is out-of-order. This is implemented in `message_manager(message_id)` in `main.cpp`. This thread will monitor if we receive all the `AckMessage` after <timeout>; if not, resend the `DataMessage` to the process that haven't acknowledge. It will try resending for <trylimit> times, and then just waiting for all the `AckMessage`.  Then it will send the `SeqMessage` and similarly waiting for `AckSeqMessage`, which contains the receiver id of the `SeqMessage`.
	 - Message receiving and parsing: parsing is the main part of this algorithm, implemented in `parse_message(buf)`. For different type of messages it will always **check for duplicate** first. If not, it do the following:
		 - `DataMessage`: increase sequence number `seq`, put the message into the `hold_back` queue, then send `AckMessage`
		 - `AckMessage`: Update current values of `final_seq` and `final_seq_proposer` for that `DataMessage`.
		 - `SeqMessage`: update `seq`, use the `increase_key` method of `hold_back` queue (since it is a min heap) to update the message, then send `AckSeqMessage`.

**Tie-breaking rules** and hold_back queue design: to support sending at the same time, I use the lexicographical order of `(seq, proposer)` to break ties. I designed so that the agreed pair will always be at least the proposed pair, so it is an `increase_key` operation we can perform in a min heap (priority queue). The min heap stores messages in the form `(sender, message_id, seq, proposer, deliverable?)`, and has a `map` to quickly find this tuple by `(sender, message_id)` in the heap.


# Files

 - `utils.cpp/h` implements **connectless UDP** communications;
 - `pack.cpp/h` implements packing and unpacking of messages;
 - `heap.cpp/h` implements the min heap efficiently;
 - `main.cpp/h` implements the main functionalities.
And there are also some other `.h` files.
