# Architecture
SchedScope has one scheduling model and two execution targets. Both targets implement the same five policies and discrete-time execution model, with small target-specific differences in tie-breaking.

## AVR target

```
                 ┌──────────────────────────────────┐
 NEXT  (D4) ────▶│                                  │── D6 ──[330 Ω]──▶ WS2812 8×8 ×5 → 40×8 matrix
 START (D5) ────▶│  ATmega328P                      │
 RESET (D2) ────▶│  INT0 ISR → volatile flag        │── D7..D12 ──────▶ LM016L 16×2 LCD (4-bit)
                 │  run loop · policies · UI state  │
                 └──────────────────────────────────┘
```

### Software layers

| Layer | Responsibility | Key functions |
|---|---|---|
| Workload data | Six 3-task workloads as a `Task[6][3]` constant table | `taskSets`, `copyTasks()` |
| Policy engine | One function per policy sharing the same step structure | `runFCFS()`, `runSJF()`, `runPriority()`, `runEDF()`, `runRR()` |
| Run sequencer | Runs the five policies in order and aborts the chain on reset | `runAllAlgorithms()` |
| Timing | Paces each time unit (250 ms) in 25 ms slices and polls the reset flag | `waitOneUnit()` |
| Visualization | Maps (time, policy) to an LED index and writes task colors | `xyToIndex()`, `drawStep()`, `drawIdleStep()` |
| Telemetry | LCD status, per-step state, per-policy result and trace string | `showReady()`, `showRunning()`, `showFinalResult()` |
| Event handling | Minimal ISR plus deferred handler | `interruptHandler()`, `handleInterrupt()` |

### Input strategy

| Input | Mechanism | Rationale |
|---|---|---|
| NEXT, START | Polled in `loop()`, `INPUT_PULLUP`, active-low, 300 ms lockout | Only read while idle, where latency does not matter |
| RESET | `attachInterrupt(digitalPinToInterrupt(2), …, FALLING)` | Must be captured while a run is blocking in timed steps |

### Resource budget

- The FastLED frame buffer takes 320 LEDs × 3 bytes = 960 bytes of the 2 KB SRAM, which dominates RAM usage.
- Each policy call keeps a 41-byte trace string (`MATRIX_WIDTH + 1`) on the stack for the LCD Gantt readout.
- A run is capped at 40 time units, the width of the matrix.

## ARMv7 target

```
_start ─▶ FCFS ─▶ SJF ─▶ Priority ─▶ EDF ─▶ Round Robin ─▶ stop: b stop
            │       │        │         │         │
            ▼       ▼        ▼         ▼         ▼
         ┌────────────────────────────────────────────┐
         │ .data: tasks + one isolated region per      │
         │ policy (trace, length, misses, flags, ...)  │
         └────────────────────────────────────────────┘
```

- GNU assembler syntax (`.syntax unified`), ARMv7 generic system in CPUlator.
- No `bl`, `push` or `pop`. Control flow is plain branches, and shared logic is expanded by macros.
- Register conventions: `r0` always holds the current time. In FCFS, SJF, Priority and EDF, `r2` is the output base and `r3` the selected task index. In Round Robin, `r1` is the output base and `r2` the running task. `r5`–`r12` are scratch registers used by the macros.
- Literal pools are placed with `.ltorg` after each policy block to keep `ldr rX, =label` in range.
