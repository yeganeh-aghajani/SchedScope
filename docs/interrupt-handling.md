# Interrupt Handling (AVR)

## Configuration

```cpp
pinMode(INT_BTN, INPUT_PULLUP);                       // D2, active-low
attachInterrupt(digitalPinToInterrupt(INT_BTN),       // INT0
                interruptHandler, FALLING);
```

## ISR design

```cpp
volatile bool interruptFlag = false;
void interruptHandler() { interruptFlag = true; }
```

The ISR does one thing: it sets a `volatile` flag. All visible work happens in `handleInterrupt()`, called from normal context:

1. clear the flag
2. clear the matrix
3. blink the whole matrix white three times (150 ms on, 150 ms off)
4. show `INTERRUPT! / System Reset` on the LCD
5. clear the matrix

This keeps the ISR short, and it avoids calling `FastLED.show()` (which disables interrupts while it clocks out data) or `delay()` from interrupt context.

## Where the flag is serviced

| Context | Check point | Effect |
|---|---|---|
| During a run | `waitOneUnit()`, every 25 ms | Handler runs, the policy returns `false`, and the run chain stops immediately. The system returns to READY. |
| Idle / after a run | top of `loop()` | Handler runs and the persisted Gantt chart is cleared |

## Latency characteristics

- Edge capture is handled by hardware INT0, so reset input does not depend on main-loop polling.
- *Servicing* happens at the next check point. During scheduling steps that is at most about one 25 ms slice plus the current LCD/LED update. During the fixed result and banner delays (0.7–3.5 s), the request waits until the delay ends.
- There is no hardware debounce. Repeated edges from one press only re-set the same flag.
