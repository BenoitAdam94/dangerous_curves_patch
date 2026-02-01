# TMS320C5x MAME Emulator — Code Review

**Files reviewed:** `tms320c5x.h`, `tms320c5x.cpp`, `320c5x_ops.ipp`, `320c5x_optable.cpp`, `tms320c5x_dasm.cpp/.h`  
**Reference:** TI datasheet SPRS030A  
**Author:** Code review against official documentation

---

## 1. Executive Summary

The implementation is a solid foundation. The architecture follows MAME conventions correctly: function-pointer dispatch tables, separate address spaces for program/data/I/O, proper state saving, and a clean class hierarchy for the C51 vs C53 variants. The indirect addressing engine, circular buffer logic, condition evaluation, and P-register scaling are all well done.

However, there are **~68 unimplemented instructions** (out of roughly 200 unique opcodes), several of which are **critical to any real DSP workload** — most notably the entire MAC family (`mac`, `macd`, `mads`, `madd`), all short-immediate and long-immediate multiply variants, `pac`/`spac`, and the square-root instructions. Beyond completeness, there are concrete **correctness bugs** in the stack, the logical operations, the timer, the repeat mechanisms, the shadow register set, and several branch/call instructions. These are detailed below.

---

## 2. Bugs — Correctness Issues in Implemented Code

These are not missing features; they are errors in code that already exists and will silently produce wrong results.

---

### 2.1 POP_STACK corrupts the stack

**File:** `320c5x_ops.ipp`, lines 10–16

```cpp
uint16_t tms320c51_device::POP_STACK()
{
    uint16_t pc = m_pcstack[m_pcstack_ptr];
    m_pcstack_ptr = (m_pcstack_ptr + 1) & 7;
    m_pcstack[(m_pcstack_ptr + 7) & 7] = m_pcstack[(m_pcstack_ptr + 6) & 7];  // ← BUG
    return pc;
}
```

After advancing the pointer, line 14 **overwrites the slot that was just popped** with the value of the slot above it. The TMS320C5x has a simple 8-level LIFO stack with no hardware stack pointer — a pop just advances the pointer. There is no documented "shift" behaviour. This line should be removed entirely.

**Impact:** Any nested call/return sequence deeper than one level will return to the wrong address.

---

### 2.2 AND / OR / XOR with memory operand silently clears upper 16 bits

**File:** `320c5x_ops.ipp` — `op_and_mem`, `op_or_mem`, `op_xor_mem`

```cpp
void tms320c51_device::op_and_mem()
{
    uint16_t ea  = GET_ADDRESS();
    uint16_t data = DM_READ16(ea);
    m_acc &= (uint32_t)(data);      // ← zero-extends, ANDs upper 16 bits to 0
    ...
}
```

Casting a `uint16_t` to `uint32_t` produces `0x0000_xxxx`. ANDing the accumulator with that value **unconditionally clears bits 16–31 of ACC**. Per the datasheet, these instructions operate on the lower 16 bits of the accumulator only; the upper 16 bits are unaffected.

The correct operand mask for AND is `0xFFFF0000 | (uint32_t)data`. OR and XOR are fine as written for the lower half (OR and XOR with zero are no-ops), but AND is a definite bug. All three should ideally be documented with a comment about the upper-bit semantics for clarity.

**Fix for `op_and_mem`:**
```cpp
m_acc &= (0xFFFF0000u | (uint32_t)data);
```

---

### 2.3 Shadow register set is incomplete — AR[] array is missing

**File:** `tms320c5x.h` — the `m_shadow` struct; `tms320c5x.cpp` — `save_interrupt_context` / `restore_interrupt_context`

The shadow registers are used on interrupt entry/exit to save and restore context. The current struct saves ACC, ACCB, ARCR, INDX, PREG, TREG0–2, PMST, ST0, and ST1. However, **the AR[0]–AR[7] auxiliary register array is not saved or restored**, nor is BMAR.

Per the datasheet, the shadow register set on the C5x includes all auxiliary registers. If an ISR modifies any AR or BMAR (which is extremely common — address registers are the workhorses of DSP loops), the values are silently lost on RETE.

**Fix:** Add `uint16_t ar[8]` and `uint16_t bmar` to the `m_shadow` struct, and save/restore them in `save_interrupt_context` / `restore_interrupt_context`. Also add the corresponding `save_item` calls in `device_start`.

---

### 2.4 Timer fires spurious interrupts at startup

**File:** `tms320c5x.cpp` — `execute_run`, lines 520–533

The timer decrement logic runs unconditionally every cycle, including before the timer has been configured. At reset, `psc`, `tddr`, `tim`, and `prd` are all zero. The decrement immediately drives `psc` to −1 (it's an `int`), which satisfies `<= 0`, reloads it to `tddr` (0), decrements `tim` to −1, which again satisfies `<= 0`, reloads from `prd` (0), and fires `TINT` — every single cycle until software configures the timer.

The timer should only tick when it has been enabled (typically gated on `prd != 0` or a dedicated enable bit if one exists in the TCR). At minimum, the reload logic should prevent the infinite-zero-period loop:

```cpp
if (m_timer.prd != 0)  // only tick if timer has been configured
{
    m_timer.psc--;
    ...
}
```

---

### 2.5 `op_lacc_limm` charges only 1 cycle instead of 2

**File:** `320c5x_ops.ipp`, line 516

`op_lacc_limm` reads a second word via `ROPCODE()` (the long immediate), making it a two-word instruction. All other two-word instructions in this file correctly charge `CYCLES(2)`. This one charges `CYCLES(1)`.

---

### 2.6 `op_out` applies a spurious `<< 1` shift to the port address

**File:** `320c5x_ops.ipp`, line 1381

```cpp
m_io.write_word(port << 1, data);
```

The port number is read directly from the instruction stream as a 16-bit PMA. The I/O space in MAME is already word-addressed (the space config uses a −1 byte shift, matching the program and data spaces). Shifting the port left by 1 here doubles the address and will cause writes to land at the wrong I/O port.

**Fix:** Remove the `<< 1`:
```cpp
m_io.write_word(port, data);
```

---

### 2.7 BLDD/BLDP/BLPD family does not update BMAR after transfer

**File:** `320c5x_ops.ipp` — `op_bldd_sbmar`, `op_bldd_dbmar`, `op_bldp`

The BMAR-sourced block transfer variants read the starting address from `m_bmar` into a local variable `pfc`, increment `pfc` in the loop, but never write it back to `m_bmar`. Per the datasheet, BMAR is updated to point past the last transferred word on completion. This matters whenever the same block transfer is called again or whenever software reads BMAR afterward.

**Fix:** After each loop, add `m_bmar = pfc;`

---

### 2.8 `op_setc_intm` calls `check_interrupts()` — it should not

**File:** `320c5x_ops.ipp`, line 1898

`SETC INTM` sets the interrupt-mask bit to 1, which **disables** interrupts. Calling `check_interrupts()` after setting `intm = 1` is harmless (the check will exit immediately because `intm != 0`), but it is semantically wrong and misleading. `check_interrupts()` is only needed after `CLRC INTM` (which correctly calls it). Remove the call from `op_setc_intm`.

---

### 2.9 `op_ccd` charges 2 cycles even on the taken path

**File:** `320c5x_ops.ipp`, lines 1150–1167

```cpp
void tms320c51_device::op_ccd()
{
    ...
    if (condition_met)
    {
        PUSH_STACK(m_pc+2);
        ...
        delay_slot(m_pc);
        CHANGE_PC(pma);
    }
    CYCLES(2);   // ← runs unconditionally
}
```

When the condition is taken, the delay slot executes one or two additional instructions (each with their own cycle charges inside `delay_slot`), and then the branch itself should cost 4 cycles total (matching `op_cc`). The unconditional `CYCLES(2)` is only correct for the not-taken path. The taken path should charge `CYCLES(4)` inside the `if` block, and the `else` path should charge `CYCLES(2)`.

---

### 2.10 `op_calad` pushes `m_pc + 2` before `delay_slot` executes

**File:** `320c5x_ops.ipp`, lines 1095–1104

```cpp
void tms320c51_device::op_calad()
{
    uint16_t pma = m_acc;
    PUSH_STACK(m_pc+2);    // ← return address calculated before delay slot runs
    delay_slot(m_pc);
    CHANGE_PC(pma);
    CYCLES(4);
}
```

`CALAD` is a delayed call — it executes the next instruction(s) in the delay slot before jumping. The return address should point to the instruction *after* the delay slot, not after the `CALAD` itself. `delay_slot()` advances `m_pc` as it fetches and executes the delay-slot instructions, so the correct return address is the value of `m_pc` *after* `delay_slot()` returns. The push should come after the delay slot:

```cpp
delay_slot(m_pc);
PUSH_STACK(m_pc);      // now points past the delay slot
CHANGE_PC(pma);
```

(The same issue should be audited in `op_calld`, `op_ccd`, and `op_retcd`.)

---

## 3. Unimplemented Instructions — Prioritized by Impact

68 instructions currently hit `fatalerror`. They are grouped below by how critical they are to running real DSP software.

---

### 3.1 Critical — MAC and Multiply (the core of any DSP workload)

These are the instructions that make a DSP a DSP. No real filter, convolution, or transform kernel will run without them.

| Instruction | Description |
|---|---|
| `mac` | Multiply-accumulate: `P = T × D(ea); ACC += P<<PM` |
| `macd` | MAC with delay slot |
| `madd` | `P = T × TREG1; ACC += P<<PM` |
| `mads` | `P = T × TREG1; ACC += P<<PM` (with store) |
| `mpy_simm` | `P = T × #simm` (short immediate multiply) |
| `mpy_limm` | `P = T × #limm` (long immediate multiply) |
| `mpya` | `P = ACC_low × T` |
| `mpys` | `P = ACC_low × T` (signed, updates ACC) |
| `mpyu` | `P = ACC_low × T` (unsigned) |
| `pac` | `ACC += P<<PM` (product accumulate) |
| `spac` | `ACC -= P<<PM` (product subtract-accumulate) |
| `sqra` | `P = ACC_low²` |
| `sqrs` | `P = ACC_low²; ACC += P<<PM` |

The single-operand `op_mpy_mem` is implemented, but it is the only multiply variant that works. Together these 13 instructions represent the majority of compute in any DSP algorithm.

---

### 3.2 High Priority — Arithmetic and Logic Completeness

| Instruction | Description |
|---|---|
| `abs` | Absolute value of ACC |
| `adcb` | `ACC += ACCB + C` |
| `addc` | `ACC += D(ea) + C` |
| `adds` | `ACC += D(ea)` (short address, no DP) |
| `addt` | `ACC += T` |
| `subb` | `ACC -= ACCB` |
| `subc` | `ACC -= D(ea) - C` |
| `subs` | `ACC -= D(ea)` (short address) |
| `subt` | `ACC -= T` |
| `sbbb` | `ACC -= ACCB - C` |
| `andb` | `ACC &= ACCB` |
| `xorb` | `ACC ^= ACCB` |
| `norm` | Normalize ACC |
| `zalr` | Zero ACC and ARCR |
| `zpr` | Zero P register |

`abs` and `norm` are particularly common in DSP post-processing. The `ACCB`-register ALU ops (`subb`, `adcb`, etc.) are used in butterfly structures.

---

### 3.3 High Priority — Memory and Register Transfer

| Instruction | Description |
|---|---|
| `lst st0` / `lst st1` | Load status register from memory |
| `sst st0` / `sst st1` | Store status register to memory |
| `push` / `pshd` | Push ACC / push D(ea) onto stack |
| `popd` | Pop stack into D(ea) |
| `lact` | Load ACC from T register |
| `ltd` | Load T, with delay slot |
| `ltp` | Load T from P register |
| `lts` | Load T with store to memory |
| `lph` | Load P high-word |
| `ldp mem` | Load DP from memory |
| `dmov` | Data move D(ea) → D(ea+1) |
| `in` | Port input |

`lst`/`sst` are essential for saving and restoring processor state (context switches, nested interrupts). `push`/`pop` are needed for any software that uses the stack beyond simple call/return.

---

### 3.4 Medium Priority — Control Flow

| Instruction | Description |
|---|---|
| `banzd` | Branch if AR ≠ 0, delayed |
| `rptz` | Zero-repeat (RPTZ clears RPT count) |
| `intr` | Software interrupt |
| `reti` | Return from interrupt (non-shadow version) |
| `nmi` | Non-maskable interrupt entry |
| `trap` | Trap/exception entry |

`banzd` is the delayed version of the already-implemented `banz` and is frequently used in optimized loop tails. `rptz` is needed to cancel an active repeat.

---

### 3.5 Medium Priority — Logical and Shift Completeness

| Instruction | Description |
|---|---|
| `rol` / `ror` | Rotate ACC left/right through carry |
| `rorb` | Rotate ACC:ACCB right through carry |
| `sfrb` | Shift ACC:ACCB right |
| `and_s16_limm` / `or_s16_limm` / `xor_s16_limm` | Logical ops with 16-bit shifted immediate |
| `xpl_dbmr` / `xpl_imm` | XOR port logic |
| `cpl_dbmr` | Compare port logic (DBMR variant) |
| `sath` | Store accumulator high (saturated) |

---

### 3.6 Lower Priority — Flags and Peripherals

| Instruction | Description |
|---|---|
| `clrc carry` / `setc carry` | Clear/set carry flag |
| `clrc hold` / `setc hold` | Clear/set hold flag |
| `clrc tc` | Clear TC flag |
| `idle2` | Extended idle mode |
| `blpd bmar` | Block load program-to-data via BMAR |

The carry and hold flag operations are trivial one-liners. `clrc tc` in particular is used after reading TC in conditional branches and should be implemented immediately — it is a single line: `m_st1.tc = 0;`.

---

## 4. Architectural Observations and Recommendations

### 4.1 The `delay_slot` function is fragile

`delay_slot` determines whether it needs to execute one or two instructions by comparing `m_pc - startpc < 2`. This assumes all delay-slot instructions are single-word. If a two-word instruction (like `LACC #limm`) appears in a delay slot, `delay_slot` will execute it correctly (because `ROPCODE` advances `m_pc` for the second word), but the loop condition becomes unreliable — it may try to execute a third instruction. The loop should simply execute exactly one instruction unconditionally for a 1-instruction delay slot, or be redesigned if 2-instruction delay slots are needed.

### 4.2 The boot-loader simulation in `device_reset` is non-standard

The C51 `device_reset` simulates an internal ROM boot loader by reading a source/destination/length from data memory address 0x7800 and copying a block into program memory. This is specific to one piece of hardware (noted in the comment as "Taito JC"). It should be behind a flag or moved to a machine-specific driver, because it will break any other system that uses this CPU. The C53 `device_reset` correctly just sets PC to 0.

### 4.3 The `op_invalid` / `fatalerror` pattern will crash MAME

Every unimplemented instruction calls `fatalerror`, which terminates the process. For a production emulator, unimplemented instructions should log a warning and NOP (or trap) so that the emulation can continue and the developer can see what needs to be added. MAME provides `logerror` for exactly this purpose.

### 4.4 Register type widths are inconsistent

`m_treg0`, `m_treg1`, `m_treg2` are declared as `uint16_t`, which is correct for the hardware. However, `m_rptc` and `m_brcr` are declared as `int32_t`. `RPTC` is a 16-bit register on the hardware; using a 32-bit signed type works but is confusing, especially since it is initialized to −1 as a sentinel value. Consider using a separate boolean flag for "repeat active" rather than overloading the sign bit of a wider type.

### 4.5 The I/O port forwarding in `cpuregs_r`/`cpuregs_w` uses fall-through case lists

The 16 I/O ports (0x50–0x5F) are handled via 16 consecutive `case` labels with no code between them, falling through to a single `m_io.read_word` / `m_io.write_word`. This works but is fragile to future edits. A range check (`if (offset >= 0x50 && offset <= 0x5f)`) at the top of the switch default would be cleaner and less error-prone.

---

## 5. Summary Table

| Category | Count | Severity |
|---|---|---|
| Correctness bugs in implemented code | 10 | **Critical** — will produce wrong results silently |
| Unimplemented instructions (MAC/MPY) | 13 | **Critical** — no DSP algorithm will run |
| Unimplemented instructions (ALU/logic) | 15 | High |
| Unimplemented instructions (memory/transfer) | 12 | High |
| Unimplemented instructions (control flow) | 6 | Medium |
| Unimplemented instructions (shift/rotate/logic) | 7 | Medium |
| Unimplemented instructions (flags/peripherals) | 6 | Lower |
| Architectural / style recommendations | 5 | Maintenance |

---

## 6. Recommended Implementation Order

1. **Fix the 10 correctness bugs** listed in Section 2 — especially POP_STACK (2.1), the AND mask (2.2), shadow registers (2.3), and the timer (2.4). These corrupt state silently.
2. **Implement the MAC family** (Section 3.1) — `mac`, `macd`, `mads`, `madd`, `pac`, `spac`, and the remaining multiply variants. These are the reason the chip exists.
3. **Implement `push`/`pop`/`lst`/`sst`** (Section 3.3) — needed for any context-switching or nested-interrupt scenario.
4. **Implement `banzd` and `rptz`** (Section 3.4) — common in optimized loop constructs.
5. **Fill in the remaining ALU ops** (Section 3.2) — most are trivial one- or two-line implementations.
6. **Replace `fatalerror` with `logerror`** for all unimplemented ops so that MAME does not crash on unknown instructions.
