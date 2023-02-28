# Report for Lab 2 - Xuangui Huang

In this project I implement the view change (leader election) algorithm in the "paxos for system builders" paper.

# Designs
For user-friendliness the program I will always print a process' host name as in the host name file.

I have one additional message type, `UpMessage`  for the up process. I will explain its difference from last lab later.

Each process has three status, `STATUS_UP`, `LEADER_ELECTION`, and `STATUS_OTHER`:

```
                       +-------------+
                       |             |
                       |  STATUS_UP  |
                       |             |
                       +-------------+
                             |All processes are up
                             |And Progress timer expires
                             v
                     +-------+----------+
                     |                  |
                     | LEADER_ELECTION  +<----------------------+
                     |                  |                       |
                     +-+--+-------------+                       |
                       |  |                                     |
Received enough        |  |Received VC_Proof                    |
View Change            |  |for higher view                      |
for current attempted  |  |                                     |
view                   |  |                                     |
                       v  v                                     |
                       +--+-----------+                         |
                       |              |  Progress Timer expires |
                       | STATUS_OTHER +-------------------------+
                       |              |
                       +--------------+
```
In the paper there are two status `REG_LEADER` and `REG_NON_LEADER`, but as we only need to implement view change so I consolidate them into one status `STATUS_OTHER`.

Same as last time, I use one receiver thread, and one sender thread per server.

## The main view change algorithm
The main algorithm, as specified in the paper, is mainly implemented in the function `parse_message()` in `main.cpp`. The only difference from the paper is that upon receiving `VC_Proof` message for later views, I will also multiply the progress timer value by 2 and start it. This will make processes synchronize better.

To record `View_Change` messages, I use a `std::set<uint32_t>` named `vc_received` to record the server id from which I have already received VC messages in the current view change, and store it directly in `view_changes`. 

For sending `VC_Proof` messages, I use a separate thread invoking the function `VP_sender()` (in `main.cpp`), which in an infinite loop sleeps for the desired time then send out `VC_Proof` messages to others.

For progress timer, I use a separate thread calling the function `progress_timer_func()` (in `main.cpp`) , which sleeps for the time we want then invoke a new leader election.

## Implementing test cases
Except the first progress timer which triggers the view change for view 1, all the other progress timers won't be set for test case 1.

The function `check_and_crash()` (in `main.cpp`) will check if the process need to crash and do it if so. It will be invoked after receiving enough `View_Change` messages for the current attempted view.

As described in my piazza post ([@140](https://piazza.com/class/k045z0dc7u060f?cid=140)), a process can also enter the view after receiving a `VC_Proof` message, and moreover in some cases this is the only way the system doesn't get stuck. So `check_and_crash()` will also be invoked after receiving a `VC_Proof` with higher view.

## The up process
Last time I only check if the containers are up, but this time I will check if every **process** is up. The `UpMessage` now contains the id of the sender instead of the receiver.

To get the id of myself, I made two changes:

 - `get_addr_info(id)` (in `utils.cpp`) will record my own IP address when `id == ID_UNKNOWN` by calling `gethostname` first then use this name to get and record my real IP address (last time I used `NULL` in the first parameter of the `getaddrinfo()` library call, then I only got a `0.0.0.0` address, useless for address comparison). When giving `id`, this function will now check if the resolved IP address is the same as my own address to get my server id.
 - `init_socket()` (in `utils.cpp`) now calls `get_addr_info(ID_UNKNOWN)` to get my own IP address and then call it again immediately on every server id to check.

In `main.cpp`, the main function will repetitively call `broadcast_up()` to send out `UpMessage` showing that I'm up until the status changes.

In `parse_message()`, in the `STATUS_UP` status, upon receiving an `UpMessage` I will record its sender, update the count of processes that are already up, and then **send back my own `UpMessage`**. The last step helps synchronizing processes since the sender can now (almost) immediately know that I'm also up and do not need to wait for my next round of broadcasting.
If all the processes are up, it will enter view 0 and start the progress timer.

## Language Choice
Last time my code is very C-like even though I was using C++. I tried to change the coding style into more effective-modern-C++\-like but had some problems, so I stuck with my old way at last.

More specifically, I am using the threaded model instead of event-driven model. Some of the threads will actually have lots of busy waiting, reducing the efficiency of my implementation. I read the book *C++ Concurrency in Action* to learn new features and libraries I can use for better concurrent programming, such as futures, `std::async()`, and conditional variables. However, the things that I needed most are not yet implemented in the latest version of `gcc`: I would like to use `std::future.then()` to chain up futures, enabling continuation passing style concurrent programming; I would also like to use `when_all` to wait for futures, which might increase the efficiency of my implementation of the up process as there would be no need to busy looping and checking the current status.

 Beside, the Networking TS in the new standard is not finalized so there is basically no C++ way to write networking code, thus we still have to deal with raw pointers instead of using `shared_ptr`, `unique_ptr`, etc.

# Files

 - `utils.cpp/h` implements connectless UDP communications;
 - `pack.cpp/h` implements packing and unpacking of messages;
 - `main.cpp/h` implements the main functionalities;
 - `common.h` defines common configuration parameters, structs, and constants.
