# Results and Verification

## Method

`tools/reference_model.py` re-implements the scheduling semantics of both targets on the host (same selection rules, tie-breaks, Round Robin ordering and deadline definition). The traces observed in simulation were compared against it:

- **AVR:** Proteus LED matrix and LCD output
- **ARMv7:** memory contents at `stop` in CPUlator

All observed traces match the model.

## AVR: six workloads

Trace digits are task IDs per time unit.

| Set | FCFS | SJF | Priority | EDF | Round Robin |
|---|---|---|---|---|---|
| 1 | `0000000012` ✗ T1,T2 | `0000000012` ✗ T1,T2 | `0120000000` ✓ | `0120000000` ✓ | `0012000000` ✓ |
| 2 | `000011122` ✓ | `221110000` ✗ T0 | `000011122` ✓ | `000011122` ✓ | `001122001` ✓ |
| 3 | `000011112222` ✓ | `000011112222` ✓ | `000011112222` ✓ | `000011112222` ✓ | `001122001122` ✗ T0,T1 |
| 4 | `0000001122222` ✓ | `0000001122222` ✓ | `0000002222211` ✗ T1 | `0112222200000` ✓ | `0011220022002` ✓ |
| 5 | `0011222222` ✗ T2 | `0011222222` ✗ T2 | `0011222222` ✗ T2 | `0222222011` ✓ | `0011222222` ✗ T2 |
| 6 | `001122` ✗ T1,T2 | `001122` ✗ T1,T2 | `001122` ✗ T1,T2 | `001122` ✗ T1,T2 | `001122` ✗ T1,T2 |

✗ lists **every** missing task. The device LCD reports only the first one by completion time (e.g. set 3 RR shows `Miss RR / Task T0`).

**Observations**
- EDF meets every deadline in sets 1–5. Set 6 is infeasible: three 2-unit tasks all due at t = 3.
- Set 4: preemptive Priority delays T1 (priority 10) long enough for it to miss its deadline at t = 8, while every other policy succeeds.
- Set 2: SJF runs the short jobs first and pushes T0 past its deadline, which is the classic trade-off between average waiting time and deadline compliance.
- Set 3: Round Robin's fairness interleaves equal jobs and causes two misses that the run-to-completion policies avoid.

## ARMv7: bundled workload

| Policy | `<p>_output` | `_length` | `_deadline_miss` | `_count` |
|---|---|---|---|---|
| FCFS | 1 1 1 1 2 2 2 3 3 | 9 | 3 | 1 |
| SJF | 1 1 1 1 3 3 2 2 2 | 9 | 3, 2 | 2 |
| Priority | 1 1 1 1 3 3 2 2 2 | 9 | 3, 2 | 2 |
| EDF | 1 2 3 3 2 2 1 1 1 | 9 | — | 0 |
| Round Robin | 1 1 2 2 3 3 1 1 2 | 9 | 3, 2 | 2 |

In FCFS, T2 completes exactly at its deadline (t = 7) and is correctly *not* counted as a miss.

## Reproduce

```bash
python3 tools/reference_model.py
python3 tools/reference_model.py --armv7
```
