# Testing Results
I run my program in verbose mode so it will be more clear what happened. The messages printed out during the up process are not displayed here. Note also my program won't print out duplicate VC_Proof messages.

## Test Case 1
All processes will install and stay at View 1 after progress timer expires in View 0. Server 1 is the new leader.

---
p0: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VP installed = 1 from p1
p0: Server p1 is the new leader of view 1
Shift to reg non leader

---
p1: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VC attempted = 1 from p3
Received VP installed = 0 from p3
Received VC attempted = 1 from p0
p1: Server p1 is the new leader of view 1
Shift to prepare phase

---
p2: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VC attempted = 1 from p1
Received VP installed = 1 from p1
p2: Server p1 is the new leader of view 1
Shift to reg non leader

---
p3: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VC attempted = 1 from p1
Received VC attempted = 1 from p0
p3: Server p1 is the new leader of view 1
Shift to reg non leader

---
p4: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VP installed = 1 from p3
p4: Server p1 is the new leader of view 1
Shift to reg non leader

---

## Test Case 2
The leadership will rotate forever. Because we double the progress timer at each view change, the wait time before starting view change will increases exponentially.

---
p0: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VC attempted = 1 from p3
Received VC attempted = 1 from p1
p0: Server p1 is the new leader of view 1
Shift to reg non leader
Progress timer expires
Shift to leader election for view 2
Received VC attempted = 2 from p3
Received VC attempted = 2 from p2
p0: Server p2 is the new leader of view 2
Shift to reg non leader
Progress timer expires
Shift to leader election for view 3
Received VC attempted = 3 from p3
Received VC attempted = 3 from p2
p0: Server p3 is the new leader of view 3
Shift to reg non leader
Progress timer expires
Shift to leader election for view 4
Received VC attempted = 4 from p3
Received VP installed = 4 from p4
p0: Server p4 is the new leader of view 4
Shift to reg non leader
Progress timer expires
Shift to leader election for view 5
Received VC attempted = 5 from p4
Received VC attempted = 5 from p3
p0: Server p0 is the new leader of view 5
Shift to prepare phase

---
p1: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VC attempted = 1 from p0
Received VC attempted = 1 from p2
p1: Server p1 is the new leader of view 1
Shift to prepare phase
Progress timer expires
Shift to leader election for view 2
Received VP installed = 2 from p2
p1: Server p2 is the new leader of view 2
Shift to reg non leader
Progress timer expires
Shift to leader election for view 3
Received VP installed = 3 from p4
p1: Server p3 is the new leader of view 3
Shift to reg non leader
Progress timer expires
Shift to leader election for view 4
Received VP installed = 4 from p4
p1: Server p4 is the new leader of view 4
Shift to reg non leader
Progress timer expires
Shift to leader election for view 5
Received VP installed = 5 from p4
p1: Server p0 is the new leader of view 5
Shift to reg non leader

---
p2: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VP installed = 1 from p0
p2: Server p1 is the new leader of view 1
Shift to reg non leader
Progress timer expires
Shift to leader election for view 2
Received VC attempted = 2 from p4
Received VC attempted = 2 from p3
p2: Server p2 is the new leader of view 2
Shift to prepare phase
Progress timer expires
Shift to leader election for view 3
Received VP installed = 2 from p1
Received VP installed = 3 from p0
p2: Server p3 is the new leader of view 3
Shift to reg non leader
Progress timer expires
Shift to leader election for view 4
Received VC attempted = 4 from p4
Received VC attempted = 4 from p3
p2: Server p4 is the new leader of view 4
Shift to reg non leader
Progress timer expires
Shift to leader election for view 5
Received VP installed = 4 from p4
Received VP installed = 4 from p1
Received VP installed = 5 from p0
p2: Server p0 is the new leader of view 5
Shift to reg non leader

---
p3: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VC attempted = 1 from p4
Received VP installed = 0 from p4
Received VC attempted = 1 from p0
p3: Server p1 is the new leader of view 1
Shift to reg non leader
Progress timer expires
Shift to leader election for view 2
Received VP installed = 1 from p4
Received VC attempted = 2 from p4
Received VC attempted = 2 from p2
p3: Server p2 is the new leader of view 2
Shift to reg non leader
Progress timer expires
Shift to leader election for view 3
Received VC attempted = 3 from p2
Received VP installed = 2 from p1
Received VP installed = 3 from p0
p3: Server p3 is the new leader of view 3
Shift to prepare phase
Progress timer expires
Shift to leader election for view 4
Received VP installed = 4 from p4
p3: Server p4 is the new leader of view 4
Shift to reg non leader
Progress timer expires
Shift to leader election for view 5
Received VC attempted = 5 from p4
Received VC attempted = 5 from p0
p3: Server p0 is the new leader of view 5
Shift to reg non leader

---
p4: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VP installed = 0 from p1
Received VC attempted = 1 from p0
Received VC attempted = 1 from p3
p4: Server p1 is the new leader of view 1
Shift to reg non leader
Progress timer expires
Shift to leader election for view 2
Received VC attempted = 2 from p3
Received VC attempted = 2 from p0
p4: Server p2 is the new leader of view 2
Shift to reg non leader
Progress timer expires
Shift to leader election for view 3
Received VC attempted = 3 from p2
Received VC attempted = 3 from p0
p4: Server p3 is the new leader of view 3
Shift to reg non leader
Progress timer expires
Shift to leader election for view 4
Received VC attempted = 4 from p2
Received VC attempted = 4 from p0
p4: Server p4 is the new leader of view 4
Shift to prepare phase
Progress timer expires
Shift to leader election for view 5
Received VC attempted = 5 from p2
Received VP installed = 4 from p1
Received VP installed = 5 from p3
p4: Server p0 is the new leader of view 5
Shift to reg non leader

---

## Test Case 3
After Server 1 crashes, other servers will install View 1 successfully and continue to View 2, 3, etc.

---
p0: Server p1 is the new leader of view 1
Shift to reg non leader
Progress timer expires
Shift to leader election for view 2
Received VC attempted = 2 from p3
Received VC attempted = 2 from p2
p0: Server p2 is the new leader of view 2
Shift to reg non leader
Progress timer expires
Shift to leader election for view 3
Received VP installed = 2 from p2
Received VC attempted = 3 from p3
Received VC attempted = 3 from p4
p0: Server p3 is the new leader of view 3
Shift to reg non leader

---
p1: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VC attempted = 1 from p3
Received VP installed = 0 from p3
Received VC attempted = 1 from p0
p1 crashes

---
p2: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VC attempted = 1 from p0
Received VP installed = 0 from p4
Received VC attempted = 1 from p4
p2: Server p1 is the new leader of view 1
Shift to reg non leader
Progress timer expires
Shift to leader election for view 2
Received VC attempted = 2 from p4
Received VP installed = 2 from p3
p2: Server p2 is the new leader of view 2
Shift to prepare phase
Progress timer expires
Shift to leader election for view 3
Received VP installed = 2 from p4
Received VP installed = 3 from p3
p2: Server p3 is the new leader of view 3
Shift to reg non leader

---
p3: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VP installed = 0 from p0
Received VC attempted = 1 from p0
Received VP installed = 0 from p2
Received VC attempted = 1 from p1
p3: Server p1 is the new leader of view 1
Shift to reg non leader
Progress timer expires
Shift to leader election for view 2
Received VC attempted = 2 from p2
Received VC attempted = 2 from p4
p3: Server p2 is the new leader of view 2
Shift to reg non leader
Progress timer expires
Shift to leader election for view 3
Received VC attempted = 3 from p4
Received VP installed = 2 from p0
Received VC attempted = 3 from p2
p3: Server p3 is the new leader of view 3
Shift to prepare phase

---
p4: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VP installed = 1 from p0
p4: Server p1 is the new leader of view 1
Shift to reg non leader
Progress timer expires
Shift to leader election for view 2
Received VP installed = 2 from p0
p4: Server p2 is the new leader of view 2
Shift to reg non leader
Progress timer expires
Shift to leader election for view 3
Received VC attempted = 3 from p3
Received VC attempted = 3 from p2
p4: Server p3 is the new leader of view 3
Shift to reg non leader

---

## Test Case 4
In addition, after Server 2 crashes, the remaining servers will still install View 2 and then continue to View 3, 4, etc.

---
p0: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VC attempted = 1 from p3
Received VC attempted = 1 from p2
p0: Server p1 is the new leader of view 1
Shift to reg non leader
Progress timer expires
Shift to leader election for view 2
Received VC attempted = 2 from p4
Received VP installed = 1 from p2
Received VP installed = 1 from p3
Received VC attempted = 2 from p3
p0: Server p2 is the new leader of view 2
Shift to reg non leader
Progress timer expires
Shift to leader election for view 3
Received VP installed = 2 from p3
Received VC attempted = 3 from p4
Received VC attempted = 3 from p3
p0: Server p3 is the new leader of view 3
Shift to reg non leader

---
p1: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VC attempted = 1 from p2
Received VP installed = 0 from p2
Received VP installed = 1 from p3
p1 crashes

---
p2: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VP installed = 0 from p1
Received VC attempted = 1 from p1
Received VP installed = 1 from p3
p2: Server p1 is the new leader of view 1
Shift to reg non leader
Progress timer expires
Shift to leader election for view 2
Received VP installed = 2 from p4
p2 crashes

---
p3: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VC attempted = 1 from p0
Received VP installed = 0 from p1
Received VC attempted = 1 from p2
p3: Server p1 is the new leader of view 1
Shift to reg non leader
Progress timer expires
Shift to leader election for view 2
Received VC attempted = 2 from p2
Received VP installed = 2 from p4
p3: Server p2 is the new leader of view 2
Shift to reg non leader
Progress timer expires
Shift to leader election for view 3
Received VP installed = 3 from p4
p3: Server p3 is the new leader of view 3
Shift to prepare phase

---
p4: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VC attempted = 1 from p3
Received VC attempted = 1 from p0
p4: Server p1 is the new leader of view 1
Shift to reg non leader
Progress timer expires
Shift to leader election for view 2
Received VP installed = 1 from p0
Received VP installed = 1 from p2
Received VC attempted = 2 from p0
Received VP installed = 1 from p3
Received VC attempted = 2 from p3
p4: Server p2 is the new leader of view 2
Shift to reg non leader
Progress timer expires
Shift to leader election for view 3
Received VC attempted = 3 from p0
Received VC attempted = 3 from p3
p4: Server p3 is the new leader of view 3
Shift to reg non leader

---

## Test Case 5
In addition, after Server 3 crashes, Server 4 and 0 will still install View 3 but then will be stuck in the view change for view 4.

---
p0: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VP installed = 0 from p1
Received VC attempted = 1 from p2
Received VP installed = 0 from p3
Received VC attempted = 1 from p3
p0: Server p1 is the new leader of view 1
Shift to reg non leader
Progress timer expires
Shift to leader election for view 2
Received VP installed = 1 from p3
Received VC attempted = 2 from p3
Received VP installed = 1 from p4
Received VC attempted = 2 from p4
p0: Server p2 is the new leader of view 2
Shift to reg non leader
Progress timer expires
Shift to leader election for view 3
Received VC attempted = 3 from p3
Received VP installed = 3 from p4
p0: Server p3 is the new leader of view 3
Shift to reg non leader
Progress timer expires
Shift to leader election for view 4

---
p1: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VP installed = 0 from p3
Received VC attempted = 1 from p3
Received VP installed = 0 from p2
Received VC attempted = 1 from p4
p1 crashes

---
p2: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VP installed = 0 from p1
Received VP installed = 0 from p0
Received VC attempted = 1 from p1
Received VP installed = 0 from p3
Received VC attempted = 1 from p3
p2: Server p1 is the new leader of view 1
Shift to reg non leader
Progress timer expires
Shift to leader election for view 2
Received VP installed = 2 from p3
p2 crashes

---
p3: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VC attempted = 1 from p1
Received VC attempted = 1 from p4
p3: Server p1 is the new leader of view 1
Shift to reg non leader
Progress timer expires
Shift to leader election for view 2
Received VP installed = 1 from p0
Received VC attempted = 2 from p0
Received VP installed = 1 from p2
Received VC attempted = 2 from p2
p3: Server p2 is the new leader of view 2
Shift to reg non leader
Progress timer expires
Shift to leader election for view 3
Received VP installed = 3 from p4
p3 crashes

---
p4: Server p0 is the leader of view 0
Progress timer expires
Shift to leader election for view 1
Received VC attempted = 1 from p1
Received VP installed = 0 from p2
Received VP installed = 0 from p3
Received VC attempted = 1 from p3
p4: Server p1 is the new leader of view 1
Shift to reg non leader
Progress timer expires
Shift to leader election for view 2
Received VP installed = 1 from p2
Received VC attempted = 2 from p0
Received VC attempted = 2 from p2
p4: Server p2 is the new leader of view 2
Shift to reg non leader
Progress timer expires
Shift to leader election for view 3
Received VC attempted = 3 from p0
Received VC attempted = 3 from p3
p4: Server p3 is the new leader of view 3
Shift to reg non leader
Progress timer expires
Shift to leader election for view 4
Received VC attempted = 4 from p0
Received VP installed = 3 from p0

---