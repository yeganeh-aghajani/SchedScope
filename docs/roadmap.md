# Roadmap

None of the items below is implemented yet.

### Accuracy and reporting
- Report **all** deadline misses on the AVR LCD (cycle through them), matching the ARMv7 behavior.
- Compute per-policy metrics: average waiting, turnaround and response time, and CPU utilization.
- Return the LCD to READY after a post-run reset.

### Timing
- Drive the logical clock from a Timer1 CTC interrupt instead of `delay()`. That gives a deterministic time base and a non-blocking main loop, and lets reset be serviced during result screens as well.

### Verification
- Build the reference model into automated tests with golden traces.
- Assemble and run `schedscope.s` under QEMU with the GNU ARM toolchain in CI, dump the result regions and compare them against the model.
- Unify tie-breaking rules across targets and add workloads that exercise ties.

### Scalability
- Variable task count. Bounds-checked trace buffers and a circular RR queue on ARMv7.
- Move workload tables to `PROGMEM` on AVR to free SRAM.
- Additional policies: SRTF (preemptive SJF), Rate-Monotonic for periodic tasks, priority aging.

### Hardware
- Build and test on a physical Uno with WS2812 panels, including power budgeting (320 LEDs at full white) and data-line level shifting.
