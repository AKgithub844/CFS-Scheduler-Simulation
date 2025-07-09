# CFS-Scheduler-Simulation
A C++ simulation of the Completely Fair Scheduler (CFS), inspired by the Linux kernel’s process scheduling algorithm. This project demonstrates how CFS manages CPU-bound and IO-bound processes using virtual runtime and fair queuing.

=> Features :
1. Simulates both CPU-bound and IO-bound processes

2. Priority queue-based scheduling using process virtual runtime (vruntime)

3. Adjustable process priorities and burst times

4. Detailed logging of process execution slices, including start/end times and durations

5. Neatly formatted console output for process info and execution logs

=> Demo Output :
=== CFS Scheduler Demo ===

  PID      Priority       Burst         VRun        Type
----------------------------------------------------
    1         0          15            0         CPU
    2         5          20            0          IO
    3         2          10            0         CPU
    4         1          25            0          IO
    5         3          12            0         CPU

PID  Start(ns)          End(ns)            Duration(ns)
------------------------------------------------------------
    1   1234567890   1234567900           10
    3   1234567900   1234567910           10
    ...


