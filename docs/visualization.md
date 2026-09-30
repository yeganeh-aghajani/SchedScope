# Visualization

## Matrix as a Gantt chart

Five 8×8 WS2812 panels share a single data line (D6, 330 Ω series resistor) and form a 40 × 8 canvas.

| Row | Policy |
|---|---|
| 0 | FCFS |
| 1 | SJF |
| 2 | Priority |
| 3 | EDF |
| 4 | Round Robin |
| 5–7 | unused |

- **Column** = time unit 0–39
- **Color** = task: T0 red `(255,0,0)`, T1 green `(0,255,0)`, T2 blue `(0,0,255)`; idle = off
- Pixels are written **during** scheduling, one per step, followed by `FastLED.show()`. The chart grows in real time rather than being drawn from a precomputed result.
- Rows persist until a reset, so the finished matrix is a side-by-side comparison of all five policies.

## Coordinate mapping

```cpp
uint16_t xyToIndex(int x, int y) {        // x = time, y = policy row
  int panel = x / 8, localX = x % 8;
  uint16_t base = panel * 64;             // 64 LEDs per chained panel
  if (MATRIX_SERPENTINE && (y % 2))       // odd rows reversed on zig-zag panels
    return base + y * 8 + (7 - localX);
  return base + y * 8 + localX;           // row-major panels
}
```

`MATRIX_SERPENTINE` is a compile-time switch, so the same firmware can drive panels wired row-major or serpentine.

## LCD as a secondary channel

The 16 × 2 LCD shows the current policy, time and running task at each step. After each policy it shows the result (`OK` or the first missed task) and then the textual trace (`Gantt EDF / 0112222200000`), which gives a readable record next to the color view.
