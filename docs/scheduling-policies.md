# Scheduling Policies

## Common step structure

```
t = 0
while not all tasks complete:
    ready = { i : arrival[i] ≤ t and remaining[i] > 0 }
    if ready is empty:  record idle; t += 1; continue
    i = select(ready)                     # policy-specific
    record(t, i); remaining[i] -= 1; t += 1
    update completion / deadline state
```

## Policies

| Policy | Re-selects | Key | Tie-break (AVR) | Tie-break (ARMv7) |
|---|---|---|---|---|
| FCFS | on completion | arrival | lower index | lower index |
| SJF | on completion | duration | arrival, then index | lower index |
| Priority | every unit | priority value | arrival, then index | lower index |
| EDF | every unit | absolute deadline | arrival, then index | lower index |
| Round Robin | quantum expiry / completion | queue head | queue order | queue order |

The two targets break ties slightly differently. None of the bundled workloads contains a tie that distinguishes the two rules, so they produce identical traces.

## Non-preemptive policies (FCFS, SJF)

Once selected, a task runs for its full duration. The AVR implementation expresses this as an inner loop over the task's duration. The ARMv7 implementation loads the duration into a countdown register and loops until it reaches zero.

## Preemptive policies (Priority, EDF)

A `remaining[]` array is initialized from the durations. Selection runs every unit, so a newly arrived task with a better key takes over at the next step boundary. Workload 1 shows this: T1 and T2 preempt T0 at t = 1 and t = 2 under both Priority and EDF.

## Round Robin (Q = 2)

- AVR: an 8-slot circular queue (`enqueue` / `dequeue`) plus an `arrived[]` flag array.
- ARMv7: a 16-word linear queue with `rr_head` / `rr_tail` indices and `rr_added` flags, held in ordinary memory rather than on the stack.

**Ordering rule:** after each executed unit, newly arrived tasks are admitted first, and only then is the preempted task re-queued. On the ARMv7 workload this is why T2 and T3 both run before T1 resumes. A different convention would produce a different schedule.
