/*
 * SchedScope - AVR target
 * Discrete-time CPU scheduling visualizer for ATmega328P (Arduino Uno).
 *
 * Policies : FCFS, SJF (non-preemptive), Priority, EDF (preemptive), Round Robin (Q=2)
 * Output   : 40x8 WS2812 matrix (5 chained 8x8 panels) as a live Gantt chart,
 *            16x2 LM016L LCD (4-bit mode) for status and deadline reporting
 * Input    : NEXT (D4) and START (D5) polled; RESET (D2 / INT0) via attachInterrupt()
 *
 * Pin map  : LCD RS=D7 E=D8 D4..D7=D9..D12 | LED data=D6 (330R series)
 * Libraries: FastLED, LiquidCrystal
 * Validated in Proteus 8 simulation.
 */

#include <LiquidCrystal.h>
#include <FastLED.h>

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
const int NEXT_BTN  = 4;
const int START_BTN = 5;
const int INT_BTN   = 2;

#define LED_PIN        6
#define MATRIX_WIDTH   40
#define MATRIX_HEIGHT  8
#define NUM_LEDS       320
#define BRIGHTNESS     255

CRGB leds[NUM_LEDS];

const bool MATRIX_SERPENTINE = false;
const int TASK_COUNT = 3;
const int TASKSET_COUNT = 6;
const int RR_QUANTUM = 2;
const int UNIT_DELAY_MS = 250;

struct Task {
  char id;
  int arrival;
  int duration;
  int deadline;
  int priority;
};

Task taskSets[TASKSET_COUNT][TASK_COUNT] = {
  { {'0', 0, 8, 10, 3}, {'1', 1, 1, 3, 1},  {'2', 2, 1, 4, 1}  },
  { {'0', 0, 4, 8, 1},  {'1', 0, 3, 10, 3}, {'2', 0, 2, 10, 3} },
  { {'0', 0, 4, 5, 1},  {'1', 0, 4, 9, 1},  {'2', 0, 4, 15, 1} },
  { {'0', 0, 6, 15, 1}, {'1', 1, 2, 8, 10}, {'2', 2, 5, 13, 5} },
  { {'0', 0, 2, 12, 1}, {'1', 0, 2, 12, 1}, {'2', 1, 6, 8, 2}  },
  { {'0', 0, 2, 3, 1},  {'1', 0, 2, 3, 2},  {'2', 0, 2, 3, 3}  }
};

int selectedTaskSet = 1;
volatile bool interruptFlag = false;

unsigned long lastNextTime = 0;
unsigned long lastStartTime = 0;

void interruptHandler() {
  interruptFlag = true;
}

void setup() {
  pinMode(NEXT_BTN, INPUT_PULLUP);
  pinMode(START_BTN, INPUT_PULLUP);
  pinMode(INT_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INT_BTN), interruptHandler, FALLING);
  lcd.begin(16, 2);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  clearAllLeds();
  showReady();
}

void loop() {
  if (interruptFlag) {
    handleInterrupt();
  }
  if (digitalRead(NEXT_BTN) == LOW && millis() - lastNextTime > 300) {
    lastNextTime = millis();
    selectedTaskSet++;
    if (selectedTaskSet > TASKSET_COUNT) {
      selectedTaskSet = 1;
    }
    showReady();
  }

  if (digitalRead(START_BTN) == LOW && millis() - lastStartTime > 300) {
    lastStartTime = millis();
    runAllAlgorithms(selectedTaskSet - 1);
    delay(1000);
    showReady();
  }
}

void showReady() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Task Set: ");
  lcd.print(selectedTaskSet);
  lcd.setCursor(0, 1);
  lcd.print("Ready");
}

void showRunning(const char* algName, int setIndex) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Run: ");
  lcd.print(algName);
  lcd.setCursor(0, 1);
  lcd.print("Task Set: ");
  lcd.print(setIndex + 1);
}

void showFinalResult(const char* algName, char gantt[], bool deadlineMiss, char missedTask) {
  lcd.clear();
  if (deadlineMiss) {
    lcd.setCursor(0, 0);
    lcd.print("Miss ");
    lcd.print(algName);
    lcd.setCursor(0, 1);
    lcd.print("Task T");
    lcd.print(missedTask);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Done ");
    lcd.print(algName);
    lcd.setCursor(0, 1);
    lcd.print("OK");
  }
  delay(1500);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gantt ");
  lcd.print(algName);
  lcd.setCursor(0, 1);
  lcd.print(gantt);
  delay(2000);
}

uint16_t xyToIndex(int x, int y) {
  int panel = x / 8;
  int localX = x % 8;
  int localY = y;
  uint16_t base = panel * 64;
  if (MATRIX_SERPENTINE) {
    if (localY % 2 == 0) {
      return base + localY * 8 + localX;
    } else {
      return base + localY * 8 + (7 - localX);
    }
  }
  return base + localY * 8 + localX;
}

CRGB getTaskColor(char taskId) {
  if (taskId == '0') return CRGB(255, 0, 0);      // T0 = red
  if (taskId == '1') return CRGB(0, 255, 0);      // T1 = green
  if (taskId == '2') return CRGB(0, 0, 255);      // T2 = blue
  return CRGB(0, 0, 0);                           
}

void clearAllLeds() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

void drawStep(int row, int timeColumn, char taskId) {
  if (row < 0 || row >= MATRIX_HEIGHT) return;
  if (timeColumn < 0 || timeColumn >= MATRIX_WIDTH) return;
  leds[xyToIndex(timeColumn, row)] = getTaskColor(taskId);
  FastLED.show();
}

void drawIdleStep(int row, int timeColumn) {
  if (row < 0 || row >= MATRIX_HEIGHT) return;
  if (timeColumn < 0 || timeColumn >= MATRIX_WIDTH) return;
  leds[xyToIndex(timeColumn, row)] = CRGB::Black;
  FastLED.show();
}

void blinkInterruptPattern() {
  for (int k = 0; k < 3; k++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::White;
    }
    FastLED.show();
    delay(150);
    clearAllLeds();
    delay(150);
  }
}

void copyTasks(Task dest[], int setIndex) {
  for (int i = 0; i < TASK_COUNT; i++) {
    dest[i] = taskSets[setIndex][i];
  }
}

void initGantt(char gantt[]) {
  for (int i = 0; i <= MATRIX_WIDTH; i++) {
    gantt[i] = '\0';
  }
}

void addToGantt(char gantt[], int &ganttIndex, char value) {
  if (ganttIndex < MATRIX_WIDTH) {
    gantt[ganttIndex] = value;
    ganttIndex++;
    gantt[ganttIndex] = '\0';
  }
}

bool waitOneUnit() {
  int loops = UNIT_DELAY_MS / 25;
  for (int i = 0; i < loops; i++) {
    if (interruptFlag) {
      handleInterrupt();
      return false;
    }
    delay(25);
  }
  return true;
}

void handleInterrupt() {
  interruptFlag = false;
  clearAllLeds();
  blinkInterruptPattern();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("INTERRUPT!");
  lcd.setCursor(0, 1);
  lcd.print("System Reset");
  delay(1200);
  clearAllLeds();
}

void runAllAlgorithms(int setIndex) {
  clearAllLeds();
  if (!runFCFS(setIndex, 0)) return;
  if (!runSJF(setIndex, 1)) return;
  if (!runPriority(setIndex, 2)) return;
  if (!runEDF(setIndex, 3)) return;
  if (!runRR(setIndex, 4)) return;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("All Algorithms");
  lcd.setCursor(0, 1);
  lcd.print("Done");
  delay(2500);
}

//FCFS 
bool runFCFS(int setIndex, int ledRow) {
  Task tasks[TASK_COUNT];
  copyTasks(tasks, setIndex);
  bool done[TASK_COUNT] = {false, false, false};
  int completed = 0;
  int currentTime = 0;
  char gantt[MATRIX_WIDTH + 1];
  int ganttIndex = 0;
  initGantt(gantt);
  bool deadlineMiss = false;
  char missedTask = '-';
  showRunning("FCFS", setIndex);
  delay(700);

  while (completed < TASK_COUNT && currentTime < MATRIX_WIDTH) {
    int selected = -1;
    int bestArrival = 999;
    for (int i = 0; i < TASK_COUNT; i++) {
      if (!done[i] && tasks[i].arrival <= currentTime) {
        if (tasks[i].arrival < bestArrival) {
          bestArrival = tasks[i].arrival;
          selected = i;
        }
      }
    }

    if (selected == -1) {
      addToGantt(gantt, ganttIndex, '-');
      drawIdleStep(ledRow, currentTime);
      currentTime++;
      if (!waitOneUnit()) return false;
      continue;
    }

    for (int u = 0; u < tasks[selected].duration; u++) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("FCFS t=");
      lcd.print(currentTime);
      lcd.setCursor(0, 1);
      lcd.print("Running T");
      lcd.print(tasks[selected].id);
      addToGantt(gantt, ganttIndex, tasks[selected].id);
      drawStep(ledRow, currentTime, tasks[selected].id);
      currentTime++;
      if (!waitOneUnit()) return false;
    }

    done[selected] = true;
    completed++;
    if (currentTime > tasks[selected].deadline && !deadlineMiss) {
      deadlineMiss = true;
      missedTask = tasks[selected].id;
    }
  }

  showFinalResult("FCFS", gantt, deadlineMiss, missedTask);
  return true;
}

//SJF
bool runSJF(int setIndex, int ledRow) {
  Task tasks[TASK_COUNT];
  copyTasks(tasks, setIndex);
  bool done[TASK_COUNT] = {false, false, false};
  int completed = 0;
  int currentTime = 0;
  char gantt[MATRIX_WIDTH + 1];
  int ganttIndex = 0;
  initGantt(gantt);
  bool deadlineMiss = false;
  char missedTask = '-';
  showRunning("SJF", setIndex);
  delay(700);

  while (completed < TASK_COUNT && currentTime < MATRIX_WIDTH) {
    int selected = -1;
    for (int i = 0; i < TASK_COUNT; i++) {
      if (!done[i] && tasks[i].arrival <= currentTime) {
        if (selected == -1) {
          selected = i;
        } else if (tasks[i].duration < tasks[selected].duration) {
          selected = i;
        } else if (tasks[i].duration == tasks[selected].duration &&
                   tasks[i].arrival < tasks[selected].arrival) {
          selected = i;
        }
      }
    }

    if (selected == -1) {
      addToGantt(gantt, ganttIndex, '-');
      drawIdleStep(ledRow, currentTime);
      currentTime++;
      if (!waitOneUnit()) return false;
      continue;
    }

    for (int u = 0; u < tasks[selected].duration; u++) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SJF t=");
      lcd.print(currentTime);
      lcd.setCursor(0, 1);
      lcd.print("Running T");
      lcd.print(tasks[selected].id);
      addToGantt(gantt, ganttIndex, tasks[selected].id);
      drawStep(ledRow, currentTime, tasks[selected].id);
      currentTime++;
      if (!waitOneUnit()) return false;
    }

    done[selected] = true;
    completed++;
    if (currentTime > tasks[selected].deadline && !deadlineMiss) {
      deadlineMiss = true;
      missedTask = tasks[selected].id;
    }
  }

  showFinalResult("SJF", gantt, deadlineMiss, missedTask);
  return true;
}

//PRIORITY PREEMPTIVE
bool runPriority(int setIndex, int ledRow) {
  Task tasks[TASK_COUNT];
  copyTasks(tasks, setIndex);
  int remaining[TASK_COUNT];
  bool finished[TASK_COUNT] = {false, false, false};
  for (int i = 0; i < TASK_COUNT; i++) {
    remaining[i] = tasks[i].duration;
  }

  int completed = 0;
  int currentTime = 0;
  char gantt[MATRIX_WIDTH + 1];
  int ganttIndex = 0;
  initGantt(gantt);
  bool deadlineMiss = false;
  char missedTask = '-';
  showRunning("PRIORITY", setIndex);
  delay(700);

  while (completed < TASK_COUNT && currentTime < MATRIX_WIDTH) {
    int selected = -1;
    for (int i = 0; i < TASK_COUNT; i++) {
      if (remaining[i] > 0 && tasks[i].arrival <= currentTime) {
        if (selected == -1) {
          selected = i;
        } 
        else if (tasks[i].priority < tasks[selected].priority) {
          selected = i;
        } 
        else if (tasks[i].priority == tasks[selected].priority &&
                 tasks[i].arrival < tasks[selected].arrival) {
          selected = i;
        }
      }
    }

    if (selected == -1) {
      addToGantt(gantt, ganttIndex, '-');
      drawIdleStep(ledRow, currentTime);
      currentTime++;
      if (!waitOneUnit()) return false;
      continue;
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PRI t=");
    lcd.print(currentTime);
    lcd.setCursor(0, 1);
    lcd.print("Running T");
    lcd.print(tasks[selected].id);
    addToGantt(gantt, ganttIndex, tasks[selected].id);
    drawStep(ledRow, currentTime, tasks[selected].id);
    remaining[selected]--;
    currentTime++;

    if (!waitOneUnit()) return false;
    if (remaining[selected] == 0 && !finished[selected]) {
      finished[selected] = true;
      completed++;
      if (currentTime > tasks[selected].deadline && !deadlineMiss) {
        deadlineMiss = true;
        missedTask = tasks[selected].id;
      }
    }
  }
  showFinalResult("PRIORITY", gantt, deadlineMiss, missedTask);
  return true;
}

//EDF PREEMPTIVE
bool runEDF(int setIndex, int ledRow) {
  Task tasks[TASK_COUNT];
  copyTasks(tasks, setIndex);
  int remaining[TASK_COUNT];
  bool finished[TASK_COUNT] = {false, false, false};
  for (int i = 0; i < TASK_COUNT; i++) {
    remaining[i] = tasks[i].duration;
  }

  int completed = 0;
  int currentTime = 0;
  char gantt[MATRIX_WIDTH + 1];
  int ganttIndex = 0;
  initGantt(gantt);
  bool deadlineMiss = false;
  char missedTask = '-';
  showRunning("EDF", setIndex);
  delay(700);

  while (completed < TASK_COUNT && currentTime < MATRIX_WIDTH) {
    int selected = -1;
    for (int i = 0; i < TASK_COUNT; i++) {
      if (remaining[i] > 0 && tasks[i].arrival <= currentTime) {
        if (selected == -1) {
          selected = i;
        } else if (tasks[i].deadline < tasks[selected].deadline) {
          selected = i;
        } else if (tasks[i].deadline == tasks[selected].deadline &&
                   tasks[i].arrival < tasks[selected].arrival) {
          selected = i;
        }
      }
    }

    if (selected == -1) {
      addToGantt(gantt, ganttIndex, '-');
      drawIdleStep(ledRow, currentTime);
      currentTime++;
      if (!waitOneUnit()) return false;
      continue;
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("EDF t=");
    lcd.print(currentTime);
    lcd.setCursor(0, 1);
    lcd.print("Running T");
    lcd.print(tasks[selected].id);
    addToGantt(gantt, ganttIndex, tasks[selected].id);
    drawStep(ledRow, currentTime, tasks[selected].id);
    remaining[selected]--;
    currentTime++;

    if (!waitOneUnit()) return false;
    if (remaining[selected] == 0 && !finished[selected]) {
      finished[selected] = true;
      completed++;
      if (currentTime > tasks[selected].deadline && !deadlineMiss) {
        deadlineMiss = true;
        missedTask = tasks[selected].id;
      }
    }
  }

  showFinalResult("EDF", gantt, deadlineMiss, missedTask);
  return true;
}

//ROUND ROBIN
void enqueue(int queue[], int &tail, int &count, int value) {
  queue[tail] = value;
  tail = (tail + 1) % 8;
  count++;
}

int dequeue(int queue[], int &head, int &count) {
  int value = queue[head];
  head = (head + 1) % 8;
  count--;
  return value;
}

void addArrivals(Task tasks[], int remaining[], bool arrived[], int currentTime,
                 int queue[], int &tail, int &count) {
  for (int i = 0; i < TASK_COUNT; i++) {
    if (!arrived[i] && remaining[i] > 0 && tasks[i].arrival <= currentTime) {
      enqueue(queue, tail, count, i);
      arrived[i] = true;
    }
  }
}

bool runRR(int setIndex, int ledRow) {
  Task tasks[TASK_COUNT];
  copyTasks(tasks, setIndex);
  int remaining[TASK_COUNT];
  bool finished[TASK_COUNT] = {false, false, false};
  bool arrived[TASK_COUNT] = {false, false, false};
  for (int i = 0; i < TASK_COUNT; i++) {
    remaining[i] = tasks[i].duration;
  }

  int queue[8];
  int head = 0;
  int tail = 0;
  int count = 0;
  int currentTask = -1;
  int quantumUsed = 0;
  int completed = 0;
  int currentTime = 0;
  char gantt[MATRIX_WIDTH + 1];
  int ganttIndex = 0;
  initGantt(gantt);
  bool deadlineMiss = false;
  char missedTask = '-';
  showRunning("RR", setIndex);
  delay(700);

  while (completed < TASK_COUNT && currentTime < MATRIX_WIDTH) {
    addArrivals(tasks, remaining, arrived, currentTime, queue, tail, count);
    if (currentTask == -1 && count > 0) {
      currentTask = dequeue(queue, head, count);
      quantumUsed = 0;
    }
    if (currentTask == -1) {
      addToGantt(gantt, ganttIndex, '-');
      drawIdleStep(ledRow, currentTime);
      currentTime++;
      if (!waitOneUnit()) return false;
      continue;
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RR t=");
    lcd.print(currentTime);
    lcd.setCursor(0, 1);
    lcd.print("Running T");
    lcd.print(tasks[currentTask].id);
    addToGantt(gantt, ganttIndex, tasks[currentTask].id);
    drawStep(ledRow, currentTime, tasks[currentTask].id);
    remaining[currentTask]--;
    quantumUsed++;
    currentTime++;

    if (!waitOneUnit()) return false;
    addArrivals(tasks, remaining, arrived, currentTime, queue, tail, count);
    if (remaining[currentTask] == 0 && !finished[currentTask]) {
      finished[currentTask] = true;
      completed++;
      if (currentTime > tasks[currentTask].deadline && !deadlineMiss) {
        deadlineMiss = true;
        missedTask = tasks[currentTask].id;
      }
      currentTask = -1;
      quantumUsed = 0;
    } else if (quantumUsed >= RR_QUANTUM) {
      enqueue(queue, tail, count, currentTask);
      currentTask = -1;
      quantumUsed = 0;
    }
  }

  showFinalResult("RR", gantt, deadlineMiss, missedTask);
  return true;
}
