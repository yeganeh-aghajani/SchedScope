@ SchedScope - ARMv7 target
@ Five scheduling policies in stack-free ARMv7 GNU assembly.
@ Each policy writes its execution trace and deadline misses to its own .data region.
@ Run in CPUlator (ARMv7 generic): Compile and Load, breakpoint on `stop`, Continue,
@ then inspect <policy>_output, <policy>_output_length, <policy>_deadline_miss[_count].

.syntax unified
.global _start

.equ TASK_COUNT, 3
.equ TASK_SIZE, 20

.macro CLEAR_THREE label
    ldr r5, =\label
    mov r6, #0
    str r6, [r5, #0]
    str r6, [r5, #4]
    str r6, [r5, #8]
.endm

.macro CHECK_MISSES done_label, miss_label, out_label, count_label
    mov r5, #0
    ldr r6, =tasks
    ldr r7, =\done_label
    ldr r8, =\miss_label
    ldr r9, =\out_label
    ldr r10, =\count_label

.Lcheck_loop\@:
    cmp r5, #TASK_COUNT
    bge .Lcheck_end\@

    ldr r11, [r7, r5, lsl #2]
    cmp r11, #0
    bne .Lcheck_next\@

    ldr r11, [r8, r5, lsl #2]
    cmp r11, #0
    bne .Lcheck_next\@

    mov r11, #TASK_SIZE
    mla r12, r5, r11, r6

    ldr r11, [r12, #12]
    cmp r0, r11
    ble .Lcheck_next\@

    ldr r11, [r12, #0]
    ldr r12, [r10]
    str r11, [r9, r12, lsl #2]

    add r12, r12, #1
    str r12, [r10]

    mov r11, #1
    str r11, [r8, r5, lsl #2]

.Lcheck_next\@:
    add r5, r5, #1
    b .Lcheck_loop\@

.Lcheck_end\@:
.endm

.macro RR_ADD_ARRIVALS
    mov r5, #0
    ldr r6, =tasks
    ldr r7, =rr_added
    ldr r8, =rr_queue
    ldr r9, =rr_tail
    ldr r10, [r9]

.Lrr_add_loop\@:
    cmp r5, #TASK_COUNT
    bge .Lrr_add_end\@

    ldr r11, [r7, r5, lsl #2]
    cmp r11, #0
    bne .Lrr_add_next\@

    mov r11, #TASK_SIZE
    mla r12, r5, r11, r6

    ldr r11, [r12, #4]
    cmp r11, r0
    bgt .Lrr_add_next\@

    str r5, [r8, r10, lsl #2]
    add r10, r10, #1

    mov r11, #1
    str r11, [r7, r5, lsl #2]

.Lrr_add_next\@:
    add r5, r5, #1
    b .Lrr_add_loop\@

.Lrr_add_end\@:
    str r10, [r9]
.endm

.section .text
_start:

@ FCFS - Non-Preemptive

    CLEAR_THREE fcfs_done_flags
    CLEAR_THREE fcfs_miss_flags

    ldr r5, =fcfs_deadline_miss_count
    mov r6, #0
    str r6, [r5]

    mov r0, #0
    mov r1, #0
    ldr r2, =fcfs_output

fcfs_select:
    cmp r1, #TASK_COUNT
    bge fcfs_done

    mov r3, #255
    mov r4, #255
    mov r5, #0
    ldr r6, =tasks
    ldr r7, =fcfs_done_flags

fcfs_scan:
    cmp r5, #TASK_COUNT
    bge fcfs_scan_end

    ldr r8, [r7, r5, lsl #2]
    cmp r8, #0
    bne fcfs_scan_next

    mov r8, #TASK_SIZE
    mla r9, r5, r8, r6

    ldr r10, [r9, #4]
    cmp r10, r4
    bge fcfs_scan_next

    mov r4, r10
    mov r3, r5

fcfs_scan_next:
    add r5, r5, #1
    b fcfs_scan

fcfs_scan_end:
    cmp r3, #255
    beq fcfs_done

    cmp r4, r0
    ble fcfs_run_prepare

    mov r5, #0
    str r5, [r2, r0, lsl #2]

    add r0, r0, #1

    CHECK_MISSES fcfs_done_flags, fcfs_miss_flags, fcfs_deadline_miss, fcfs_deadline_miss_count

    b fcfs_select

fcfs_run_prepare:
    ldr r5, =tasks
    mov r6, #TASK_SIZE
    mla r5, r3, r6, r5

    ldr r4, [r5, #8]

fcfs_run:
    ldr r5, =tasks
    mov r6, #TASK_SIZE
    mla r5, r3, r6, r5

    ldr r6, [r5, #0]
    str r6, [r2, r0, lsl #2]

    sub r4, r4, #1
    add r0, r0, #1

    CHECK_MISSES fcfs_done_flags, fcfs_miss_flags, fcfs_deadline_miss, fcfs_deadline_miss_count

    cmp r4, #0
    bgt fcfs_run

    ldr r5, =fcfs_done_flags
    mov r6, #1
    str r6, [r5, r3, lsl #2]

    add r1, r1, #1
    b fcfs_select

fcfs_done:
    ldr r5, =fcfs_output_length
    str r0, [r5]
    b sjf_begin

.ltorg

sjf_begin:

@ SJF - Non-Preemptive

    CLEAR_THREE sjf_done_flags
    CLEAR_THREE sjf_miss_flags

    ldr r5, =sjf_deadline_miss_count
    mov r6, #0
    str r6, [r5]

    mov r0, #0
    mov r1, #0
    ldr r2, =sjf_output

sjf_select:
    cmp r1, #TASK_COUNT
    bge sjf_done

    mov r3, #255
    mov r4, #255
    mov r5, #0
    ldr r6, =tasks
    ldr r7, =sjf_done_flags

sjf_scan:
    cmp r5, #TASK_COUNT
    bge sjf_scan_end

    ldr r8, [r7, r5, lsl #2]
    cmp r8, #0
    bne sjf_scan_next

    mov r8, #TASK_SIZE
    mla r9, r5, r8, r6

    ldr r10, [r9, #4]
    cmp r10, r0
    bgt sjf_scan_next

    ldr r10, [r9, #8]
    cmp r10, r4
    bge sjf_scan_next

    mov r4, r10
    mov r3, r5

sjf_scan_next:
    add r5, r5, #1
    b sjf_scan

sjf_scan_end:
    cmp r3, #255
    bne sjf_run

    mov r5, #0
    str r5, [r2, r0, lsl #2]

    add r0, r0, #1

    CHECK_MISSES sjf_done_flags, sjf_miss_flags, sjf_deadline_miss, sjf_deadline_miss_count

    b sjf_select

sjf_run:
    ldr r5, =tasks
    mov r6, #TASK_SIZE
    mla r5, r3, r6, r5

    ldr r6, [r5, #0]
    str r6, [r2, r0, lsl #2]

    sub r4, r4, #1
    add r0, r0, #1

    CHECK_MISSES sjf_done_flags, sjf_miss_flags, sjf_deadline_miss, sjf_deadline_miss_count

    cmp r4, #0
    bgt sjf_run

    ldr r5, =sjf_done_flags
    mov r6, #1
    str r6, [r5, r3, lsl #2]

    add r1, r1, #1
    b sjf_select

sjf_done:
    ldr r5, =sjf_output_length
    str r0, [r5]
    b priority_begin

.ltorg
priority_begin:

@ Priority - Preemptive

    CLEAR_THREE priority_done_flags
    CLEAR_THREE priority_miss_flags

    ldr r5, =priority_deadline_miss_count
    mov r6, #0
    str r6, [r5]

    ldr r5, =tasks
    ldr r6, =priority_remaining
    mov r7, #0

priority_init:
    cmp r7, #TASK_COUNT
    bge priority_start

    ldr r8, [r5, #8]
    str r8, [r6, r7, lsl #2]

    add r5, r5, #TASK_SIZE
    add r7, r7, #1
    b priority_init

priority_start:
    mov r0, #0
    mov r1, #0
    ldr r2, =priority_output

priority_select:
    cmp r1, #TASK_COUNT
    bge priority_done

    mov r3, #255
    mov r4, #255
    mov r5, #0
    ldr r6, =tasks
    ldr r7, =priority_remaining

priority_scan:
    cmp r5, #TASK_COUNT
    bge priority_scan_end

    ldr r8, [r7, r5, lsl #2]
    cmp r8, #0
    beq priority_scan_next

    mov r8, #TASK_SIZE
    mla r9, r5, r8, r6

    ldr r10, [r9, #4]
    cmp r10, r0
    bgt priority_scan_next

    ldr r10, [r9, #16]
    cmp r10, r4
    bge priority_scan_next

    mov r4, r10
    mov r3, r5

priority_scan_next:
    add r5, r5, #1
    b priority_scan

priority_scan_end:
    cmp r3, #255
    bne priority_run

    mov r5, #0
    str r5, [r2, r0, lsl #2]

    add r0, r0, #1

    CHECK_MISSES priority_done_flags, priority_miss_flags, priority_deadline_miss, priority_deadline_miss_count

    b priority_select

priority_run:
    ldr r5, =tasks
    mov r6, #TASK_SIZE
    mla r5, r3, r6, r5

    ldr r6, [r5, #0]
    str r6, [r2, r0, lsl #2]

    ldr r5, =priority_remaining
    ldr r6, [r5, r3, lsl #2]

    sub r6, r6, #1
    str r6, [r5, r3, lsl #2]

    add r0, r0, #1

    CHECK_MISSES priority_done_flags, priority_miss_flags, priority_deadline_miss, priority_deadline_miss_count

    ldr r5, =priority_remaining
    ldr r6, [r5, r3, lsl #2]

    cmp r6, #0
    bne priority_select

    ldr r5, =priority_done_flags
    mov r6, #1
    str r6, [r5, r3, lsl #2]

    add r1, r1, #1
    b priority_select

priority_done:
    ldr r5, =priority_output_length
    str r0, [r5]
    b edf_begin

.ltorg

edf_begin:

@ EDF - Preemptive

    CLEAR_THREE edf_done_flags
    CLEAR_THREE edf_miss_flags

    ldr r5, =edf_deadline_miss_count
    mov r6, #0
    str r6, [r5]

    ldr r5, =tasks
    ldr r6, =edf_remaining
    mov r7, #0

edf_init:
    cmp r7, #TASK_COUNT
    bge edf_start

    ldr r8, [r5, #8]
    str r8, [r6, r7, lsl #2]

    add r5, r5, #TASK_SIZE
    add r7, r7, #1
    b edf_init

edf_start:
    mov r0, #0
    mov r1, #0
    ldr r2, =edf_output

edf_select:
    cmp r1, #TASK_COUNT
    bge edf_done

    mov r3, #255
    mov r4, #255
    mov r5, #0
    ldr r6, =tasks
    ldr r7, =edf_remaining

edf_scan:
    cmp r5, #TASK_COUNT
    bge edf_scan_end

    ldr r8, [r7, r5, lsl #2]
    cmp r8, #0
    beq edf_scan_next

    mov r8, #TASK_SIZE
    mla r9, r5, r8, r6

    ldr r10, [r9, #4]
    cmp r10, r0
    bgt edf_scan_next

    ldr r10, [r9, #12]
    cmp r10, r4
    bge edf_scan_next

    mov r4, r10
    mov r3, r5

edf_scan_next:
    add r5, r5, #1
    b edf_scan

edf_scan_end:
    cmp r3, #255
    bne edf_run

    mov r5, #0
    str r5, [r2, r0, lsl #2]

    add r0, r0, #1

    CHECK_MISSES edf_done_flags, edf_miss_flags, edf_deadline_miss, edf_deadline_miss_count

    b edf_select

edf_run:
    ldr r5, =tasks
    mov r6, #TASK_SIZE
    mla r5, r3, r6, r5

    ldr r6, [r5, #0]
    str r6, [r2, r0, lsl #2]

    ldr r5, =edf_remaining
    ldr r6, [r5, r3, lsl #2]

    sub r6, r6, #1
    str r6, [r5, r3, lsl #2]

    add r0, r0, #1

    CHECK_MISSES edf_done_flags, edf_miss_flags, edf_deadline_miss, edf_deadline_miss_count

    ldr r5, =edf_remaining
    ldr r6, [r5, r3, lsl #2]

    cmp r6, #0
    bne edf_select

    ldr r5, =edf_done_flags
    mov r6, #1
    str r6, [r5, r3, lsl #2]

    add r1, r1, #1
    b edf_select

edf_done:
    ldr r5, =edf_output_length
    str r0, [r5]
    b rr_begin

.ltorg
rr_begin:

@ Round Robin - Quantum 2

    CLEAR_THREE rr_done_flags
    CLEAR_THREE rr_miss_flags
    CLEAR_THREE rr_added

    ldr r5, =rr_deadline_miss_count
    mov r6, #0
    str r6, [r5]

    ldr r5, =rr_head
    str r6, [r5]

    ldr r5, =rr_tail
    str r6, [r5]

    ldr r5, =tasks
    ldr r6, =rr_remaining
    mov r7, #0

rr_init:
    cmp r7, #TASK_COUNT
    bge rr_start

    ldr r8, [r5, #8]
    str r8, [r6, r7, lsl #2]

    add r5, r5, #TASK_SIZE
    add r7, r7, #1
    b rr_init

rr_start:
    mov r0, #0
    ldr r1, =rr_output
    mov r4, #0

rr_main:
    cmp r4, #TASK_COUNT
    bge rr_done

    RR_ADD_ARRIVALS

    ldr r5, =rr_head
    ldr r6, [r5]

    ldr r7, =rr_tail
    ldr r8, [r7]

    cmp r6, r8
    blt rr_dequeue

    mov r5, #0
    str r5, [r1, r0, lsl #2]

    add r0, r0, #1

    CHECK_MISSES rr_done_flags, rr_miss_flags, rr_deadline_miss, rr_deadline_miss_count

    b rr_main

rr_dequeue:
    ldr r5, =rr_queue
    ldr r2, [r5, r6, lsl #2]

    add r6, r6, #1

    ldr r5, =rr_head
    str r6, [r5]

    mov r3, #0

rr_quantum:
    ldr r5, =tasks
    mov r6, #TASK_SIZE
    mla r5, r2, r6, r5

    ldr r6, [r5, #0]
    str r6, [r1, r0, lsl #2]

    ldr r5, =rr_remaining
    ldr r6, [r5, r2, lsl #2]

    sub r6, r6, #1
    str r6, [r5, r2, lsl #2]

    add r0, r0, #1
    add r3, r3, #1

    CHECK_MISSES rr_done_flags, rr_miss_flags, rr_deadline_miss, rr_deadline_miss_count

    RR_ADD_ARRIVALS

    ldr r5, =rr_remaining
    ldr r6, [r5, r2, lsl #2]

    cmp r6, #0
    beq rr_task_done

    cmp r3, #2
    blt rr_quantum

    ldr r5, =rr_tail
    ldr r6, [r5]

    ldr r7, =rr_queue
    str r2, [r7, r6, lsl #2]

    add r6, r6, #1
    str r6, [r5]

    b rr_main

rr_task_done:
    ldr r5, =rr_done_flags
    mov r6, #1
    str r6, [r5, r2, lsl #2]

    add r4, r4, #1
    b rr_main

rr_done:
    ldr r5, =rr_output_length
    str r0, [r5]

stop:
    b stop

.ltorg

.section .data
.balign 4

task_count:
    .word TASK_COUNT

tasks:
    .word 1, 0, 4, 12, 1
    .word 2, 1, 3, 7, 3
    .word 3, 2, 2, 5, 2

rr_queue:
    .word 0, 0, 0, 0
    .word 0, 0, 0, 0
    .word 0, 0, 0, 0
    .word 0, 0, 0, 0

fcfs_output:
    .space 64

fcfs_output_length:
    .word 0

fcfs_deadline_miss:
    .space 12

fcfs_deadline_miss_count:
    .word 0

fcfs_done_flags:
    .space 12

fcfs_miss_flags:
    .space 12

sjf_output:
    .space 64

sjf_output_length:
    .word 0

sjf_deadline_miss:
    .space 12

sjf_deadline_miss_count:
    .word 0

sjf_done_flags:
    .space 12

sjf_miss_flags:
    .space 12

priority_output:
    .space 64

priority_output_length:
    .word 0

priority_deadline_miss:
    .space 12

priority_deadline_miss_count:
    .word 0

priority_remaining:
    .space 12

priority_done_flags:
    .space 12

priority_miss_flags:
    .space 12

edf_output:
    .space 64

edf_output_length:
    .word 0

edf_deadline_miss:
    .space 12

edf_deadline_miss_count:
    .word 0

edf_remaining:
    .space 12

edf_done_flags:
    .space 12

edf_miss_flags:
    .space 12

rr_output:
    .space 64

rr_output_length:
    .word 0

rr_deadline_miss:
    .space 12

rr_deadline_miss_count:
    .word 0

rr_remaining:
    .space 12

rr_done_flags:
    .space 12

rr_miss_flags:
    .space 12

rr_added:
    .space 12

rr_head:
    .word 0

rr_tail:
    .word 0

data_end:
    .word 0