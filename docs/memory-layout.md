# Memory Layout (ARMv7)

All state is statically allocated in `.data` (4-byte aligned). Each policy owns an isolated region, so later policies never overwrite earlier results and all five can be compared once execution reaches `stop`.

| Symbol | Size | Policies | Contents |
|---|---|---|---|
| `tasks` | 15 words | shared | Workload records, 20 B each |
| `<p>_output` | 64 B (16 words) | all | Task ID executed at each time unit; 0 = idle |
| `<p>_output_length` | 1 word | all | Number of time units executed |
| `<p>_deadline_miss` | 12 B (3 words) | all | IDs of tasks that missed, in detection order |
| `<p>_deadline_miss_count` | 1 word | all | Number of logged misses |
| `<p>_done_flags` | 3 words | all | 1 = task complete |
| `<p>_miss_flags` | 3 words | all | 1 = miss already logged (deduplication) |
| `<p>_remaining` | 3 words | priority, edf, rr | Remaining burst per task |
| `rr_queue` | 16 words | rr | FIFO of task indices |
| `rr_head`, `rr_tail` | 1 word each | rr | Queue indices |
| `rr_added` | 3 words | rr | 1 = task already admitted to the queue |

`<p>` ∈ {`fcfs`, `sjf`, `priority`, `edf`, `rr`}.

## Macros

Because the stack is not used, reusable logic is expanded inline by macros rather than called. Labels inside macros use `\@` to stay unique on every expansion.

**`CLEAR_THREE label`** zeroes three consecutive words (per-task flags).

**`CHECK_MISSES done, miss, out, count`** runs after every executed or idle unit:
```asm
    ldr   r11, [r7, r5, lsl #2]   @ done flag   → skip if complete
    ldr   r11, [r8, r5, lsl #2]   @ miss flag   → skip if already logged
       mov r11, #TASK_SIZE
    mla   r12, r5, r11, r6        @ &tasks[i]
    ldr   r11, [r12, #12]         @ deadline
    cmp   r0, r11                 @ r0 = current time
    ble   .Lcheck_next\@          @ t ≤ D → not missed yet
    @ log tasks[i].id at out[count], count += 1, miss flag = 1
```
A task is therefore logged at the first step where t > D while it is still incomplete. The miss is detected as soon as the deadline has passed instead of when the task finally completes. Every missing task is logged, each exactly once.

**`RR_ADD_ARRIVALS`** admits every task with `arrival ≤ t` that has not yet been admitted, appending it at `rr_tail`.

## Inspecting results in CPUlator

Memory tab, Size **Word**, Format **Unsigned decimal**, then go to a label:

| Label | Expected (bundled workload) |
|---|---|
| `fcfs_output` | 1 1 1 1 2 2 2 3 3 |
| `edf_output` | 1 2 3 3 2 2 1 1 1 |
| `*_output_length` | 9 |
| `sjf_deadline_miss` / `_count` | 3, 2 / 2 |
| `edf_deadline_miss_count` | 0 |
