# Task Model and Workloads

## Task model

A task is **τᵢ = {IDᵢ, Aᵢ, Cᵢ, Dᵢ, Pᵢ}**:

| Field | Meaning |
|---|---|
| ID | Unique identifier |
| Arrival Aᵢ | Time unit at which the task becomes ready |
| Duration Cᵢ | CPU units required; granted one unit per step |
| Deadline Dᵢ | Absolute time by which the task must complete |
| Priority Pᵢ | Lower value = higher precedence |

A task **misses** its deadline if its completion time is greater than Dᵢ. Completing exactly at Dᵢ is on time. Misses are recorded but never stop scheduling.

## AVR workloads (IDs T0–T2)

Selected on the device with NEXT and shown on the LCD as `Task Set: n`.

| Set | T0 (A, C, D, P) | T1 | T2 | ΣC | What it exercises |
|---|---|---|---|---|---|
| 1 | 0, 8, 10, 3 | 1, 1, 3, 1 | 2, 1, 4, 1 | 10 | Long low-priority job blocking short urgent arrivals |
| 2 | 0, 4, 8, 1 | 0, 3, 10, 3 | 0, 2, 10, 3 | 9 | SJF reordering at the cost of the urgent task |
| 3 | 0, 4, 5, 1 | 0, 4, 9, 1 | 0, 4, 15, 1 | 12 | Equal jobs; RR time-slicing versus deadlines |
| 4 | 0, 6, 15, 1 | 1, 2, 8, 10 | 2, 5, 13, 5 | 13 | Priority ordering conflicts with urgency (low-priority task, tight deadline) |
| 5 | 0, 2, 12, 1 | 0, 2, 12, 1 | 1, 6, 8, 2 | 10 | Only EDF meets all deadlines |
| 6 | 0, 2, 3, 1 | 0, 2, 3, 2 | 0, 2, 3, 3 | 6 | Infeasible load: no policy can succeed |

All workloads fit the 40-column display.

## ARMv7 workload (IDs 1–3; 0 = idle)

```asm
.equ TASK_COUNT, 3
.equ TASK_SIZE, 20            @ 5 words per task

tasks:
    .word 1, 0, 4, 12, 1      @ ID, arrival, duration, deadline, priority
    .word 2, 1, 3,  7, 3
    .word 3, 2, 2,  5, 2
```

| Field | Offset |
|---|---|
| ID | 0 |
| Arrival | 4 |
| Duration | 8 |
| Deadline | 12 |
| Priority | 16 |
