# Execution Flow

## AVR state machine

```
          ┌──── NEXT: set = set % 6 + 1 ────┐
          ▼                                  │
     ┌─────────┐   START   ┌────────────────────────────────────┐   5 policies   ┌──────────┐
     │  READY  │──────────▶│ RUN  FCFS → SJF → PRI → EDF → RR   │───────────────▶│ RESULTS  │
     └─────────┘           └────────────────────────────────────┘                │ persist  │
          ▲                        │ RESET (INT0)                                 └──────────┘
          │                        ▼                                                  │ RESET
          └─── delay, show READY ◀─ clear, blink ×3, "INTERRUPT! / System Reset" ◀────┘
```

Aborted runs return to READY automatically. A reset after a completed run clears the matrix and leaves the reset message on the LCD until NEXT is pressed.

### LCD messages

| Phase | Line 1 | Line 2 |
|---|---|---|
| Ready | `Task Set: n` | `Ready` |
| Policy start | `Run: FCFS` | `Task Set: n` |
| Each step | `FCFS t=k` | `Running Tj` |
| Policy end | `Done FCFS` / `Miss FCFS` | `OK` / `Task Tj` |
| Trace readout | `Gantt FCFS` | trace string, e.g. `000011122` |
| All done | `All Algorithms` | `Done` |
| Reset | `INTERRUPT!` | `System Reset` |

Policy labels shown on the LCD: `FCFS`, `SJF`, `PRIORITY` (`PRI` in the per-step view), `EDF`, `RR`.

## Per-step sequence (AVR)

1. Select a task (or idle).
2. Update the LCD step view.
3. Append the task to the trace string and draw the LED at (t, policy row).
4. Charge one unit and advance t.
5. `waitOneUnit()`: 10 × 25 ms slices, checking the reset flag in each.
6. On completion, check the deadline.

## ARMv7 sequence

```
policy_begin:
    clear done/miss flags, zero miss counter, init remaining[]
policy_select:
    scan tasks → best index, or idle
policy_run:
    write task ID to output[t]; t += 1
    CHECK_MISSES
    on completion: set done flag, completed += 1
policy_done:
    output_length = t
    b next_policy_begin
...
stop: b stop
```
