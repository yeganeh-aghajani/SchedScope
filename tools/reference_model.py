#!/usr/bin/env python3
"""
SchedScope reference model.

A host-side re-implementation of the scheduling logic used by both targets.
It regenerates the expected execution trace for every policy and workload so the
AVR (LED matrix / LCD) and ARMv7 (memory) outputs can be checked against it.

    python3 tools/reference_model.py            # AVR workloads 1-6
    python3 tools/reference_model.py --armv7    # ARMv7 workload (IDs 1-based)

Semantics mirrored from the firmware:
  * one CPU unit is charged per time step; '-' marks an idle step
  * FCFS / SJF are non-preemptive; Priority / EDF re-select every step
  * ties: AVR breaks ties by arrival, then task index; the ARMv7 code breaks
    ties by task index only (identical traces for the bundled workloads)
  * Round Robin (Q=2): tasks arriving during a step are enqueued before the
    preempted task is re-queued
  * a task misses its deadline if completion time > deadline
"""
import argparse

# (arrival, duration, deadline, priority)
AVR_WORKLOADS = {
    "1": [(0, 8, 10, 3), (1, 1, 3, 1), (2, 1, 4, 1)],
    "2": [(0, 4, 8, 1), (0, 3, 10, 3), (0, 2, 10, 3)],
    "3": [(0, 4, 5, 1), (0, 4, 9, 1), (0, 4, 15, 1)],
    "4": [(0, 6, 15, 1), (1, 2, 8, 10), (2, 5, 13, 5)],
    "5": [(0, 2, 12, 1), (0, 2, 12, 1), (1, 6, 8, 2)],
    "6": [(0, 2, 3, 1), (0, 2, 3, 2), (0, 2, 3, 3)],
}
ARM_WORKLOAD = [(0, 4, 12, 1), (1, 3, 7, 3), (2, 2, 5, 2)]

ARR, DUR, DL, PRI = range(4)


def _non_preemptive(ts, key):
    n = len(ts)
    done, t, trace, finish = [False] * n, 0, [], {}
    while len(finish) < n:
        ready = [i for i in range(n) if not done[i] and ts[i][ARR] <= t]
        if not ready:
            trace.append(None); t += 1; continue
        s = min(ready, key=lambda i: key(ts, i))
        for _ in range(ts[s][DUR]):
            trace.append(s); t += 1
        done[s] = True; finish[s] = t
    return trace, finish


def _preemptive(ts, key):
    n = len(ts)
    rem, t, trace, finish = [x[DUR] for x in ts], 0, [], {}
    while len(finish) < n:
        ready = [i for i in range(n) if rem[i] > 0 and ts[i][ARR] <= t]
        if not ready:
            trace.append(None); t += 1; continue
        s = min(ready, key=lambda i: key(ts, i))
        trace.append(s); rem[s] -= 1; t += 1
        if rem[s] == 0:
            finish[s] = t
    return trace, finish


def _round_robin(ts, quantum=2):
    n = len(ts)
    rem, arrived = [x[DUR] for x in ts], [False] * n
    queue, cur, used, t, trace, finish = [], None, 0, 0, [], {}

    def admit():
        for i in range(n):
            if not arrived[i] and rem[i] > 0 and ts[i][ARR] <= t:
                queue.append(i); arrived[i] = True

    while len(finish) < n:
        admit()
        if cur is None and queue:
            cur, used = queue.pop(0), 0
        if cur is None:
            trace.append(None); t += 1; continue
        trace.append(cur); rem[cur] -= 1; used += 1; t += 1
        admit()
        if rem[cur] == 0:
            finish[cur] = t; cur = None
        elif used >= quantum:
            queue.append(cur); cur = None
    return trace, finish


POLICIES = {
    "FCFS":     lambda ts: _non_preemptive(ts, lambda ts, i: (ts[i][ARR], i)),
    "SJF":      lambda ts: _non_preemptive(ts, lambda ts, i: (ts[i][DUR], ts[i][ARR], i)),
    "Priority": lambda ts: _preemptive(ts, lambda ts, i: (ts[i][PRI], ts[i][ARR], i)),
    "EDF":      lambda ts: _preemptive(ts, lambda ts, i: (ts[i][DL], ts[i][ARR], i)),
    "RR":       _round_robin,
}


def run(ts):
    """Return {policy: (trace, misses_by_completion, misses_by_detection)}."""
    out = {}
    for name, fn in POLICIES.items():
        trace, finish = fn(ts)
        missed = [i for i in finish if finish[i] > ts[i][DL]]
        by_completion = sorted(missed, key=lambda i: finish[i])          # AVR order
        by_detection = sorted(missed, key=lambda i: (ts[i][DL], i))      # ARMv7 order
        out[name] = (trace, by_completion, by_detection)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--armv7", action="store_true", help="run the ARMv7 workload")
    args = ap.parse_args()

    if args.armv7:
        print(f"{'Policy':9} {'Trace (task ID per unit)':26} Misses")
        for name, (trace, _, det) in run(ARM_WORKLOAD).items():
            tr = " ".join("0" if s is None else str(s + 1) for s in trace)
            print(f"{name:9} {tr:26} {', '.join(f'T{i+1}' for i in det) or 'none'}")
        return

    print(f"{'Set':4} {'Policy':9} {'Trace':14} {'All misses':12} LCD report")
    for wl, ts in AVR_WORKLOADS.items():
        for name, (trace, comp, _) in run(ts).items():
            tr = "".join("-" if s is None else str(s) for s in trace)
            allm = ", ".join(f"T{i}" for i in comp) or "none"
            lcd = f"Miss T{comp[0]}" if comp else "OK"
            print(f"{wl:4} {name:9} {tr:14} {allm:12} {lcd}")


if __name__ == "__main__":
    main()
