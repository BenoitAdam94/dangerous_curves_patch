# Investigating the 0x205b Dead Loop

## What We Know

From the developer's comment and investigation:
- DSP crashes in a dead loop at PC=0x205b
- Patching address 0x205c to 0x7F00 (NOP) bypasses the loop
- After bypass: "shows pics but not enough for 3D"
- Location 0x205b is in **external ROM**, not internal ROM
- This confirms the missing internal ROM is NOT the cause

## The Developer's Workaround

In MAME debugger:
```
focus 2              # Switch to DSP
bp 205b              # Breakpoint at the loop
# When it triggers:
Ctrl+M               # Open memory window
# Edit address 205c to value 7f00
```

## What 0x7F00 Does

0x7F00 is not actually a true NOP on TMS320C5x. Looking at the instruction set:
- It's likely `DMST` (data memory status) with operand 0
- Acts like a NOP because it does nothing harmful
- True NOP doesn't exist on this DSP

The loop at 205b is likely:
```asm
205b: BCND  continue, condition    ; branch forward if ready
205c: B     0x205b                 ; else jump back (infinite loop)
205e: continue: ...
```

Patching 205c to 7F00 turns it into:
```asm
205b: BCND  continue, condition
205c: NOP                          ; does nothing
205d: (continues execution)
```

So execution falls through even if condition isn't met.

## What Is It Waiting For?

The DSP is polling for something. Possibilities:

### 1. Shared RAM Flag
Most likely. The main CPU writes a "ready" flag to shared RAM, DSP polls it.

**How to test:**
Add logging to `dsp_shared_r` for the address range the DSP is reading:

```cpp
uint16_t taitojc_state::dsp_shared_r(offs_t offset)
{
    if (m_dsp->pc() == 0x205b)
        logerror("DSP @ 205b reading shared_ram[%03X] = %04X\n", offset, m_dsp_shared_ram[offset]);
    return m_dsp_shared_ram[offset];
}
```

This will tell you exactly which shared RAM address it's polling.

### 2. Hardware Flag (TC0770CMU or I/O)
The DSP reads an MMIO register to check if TC0770CMU is ready.

**How to test:**
Add logging to the math MMIO handlers (dsp_math_* functions):

```cpp
uint16_t taitojc_state::dsp_math_projection_y_r()
{
    if (m_dsp->pc() == 0x205b)
        logerror("DSP @ 205b reading projection_y\n");
    return muldiv(m_projection_data[0], m_viewport_data[0], m_projection_data[2]);
}
```

### 3. Interrupt Flag
The DSP waits for an interrupt to arrive before continuing.

**How to test:**
Check IFR (interrupt flag register) in the debugger when it's stuck at 205b.

## Permanent Fix Approaches

### Option A: Patch the External ROM
If we determine what it's waiting for, we can:
1. Identify the correct value/condition
2. Patch the external ROM to skip the wait or use a timeout
3. Distribute a patched ROM set

### Option B: Simulate the Hardware Response
If it's waiting for TC0770CMU or another chip:
1. Implement that chip's response in MAME
2. Return the expected value when polled

### Option C: Main CPU Timing Fix
If it's waiting for main CPU to write a flag:
1. The `m_has_dsp_hack` timing might need adjustment
2. Or main CPU code has a bug specific to dangcurv

## Testing Plan

1. **Apply the 205c patch in MAME source** (so you don't have to do it manually every time)
2. **Add comprehensive logging** around address 205b
3. **Run and capture** what it's reading/waiting for
4. **Compare with working games** (Side by Side, Landing Gear) at the same point in their DSP code
5. **Determine root cause** and implement proper fix

## Applying the Patch Automatically

Add this to `taitojc.cpp` in `machine_start()` or as a separate init function for dangcurv:

```cpp
void taitojc_state::init_dangcurv()
{
    init_taitojc();
    m_has_dsp_hack = false;
    
    // Patch the dead loop at 0x205c (external ROM)
    // This is a workaround until we understand what it's waiting for
    uint16_t *dsp_rom = (uint16_t *)memregion("dspgfx")->base();
    // 0x205c is in program space, which maps to dspgfx region
    // Need to calculate the exact offset based on how the boot loader copies it
    // For now, this is a placeholder - the real patch would go here
    logerror("DANGCURV: Would patch DSP 0x205c to 0x7F00 here\n");
}
```

Actually patching ROM data in MAME init is tricky because it depends on how the boot loader copies it. Easier to patch at runtime using the debugger's scripting, or by modifying the ROM files directly and redistributing.

## The Bottom Line

The developer was right: *"maybe not"* the internal ROM. Our reconstructed internal ROM is fine. The problem is in dangcurv's **external** DSP program at 0x205b, where it waits for something that never arrives in emulation. Once we identify what it's waiting for, we can either:
- Fix the emulation to provide it
- Patch the external ROM to skip/timeout the wait
- Fix the main CPU timing so it arrives

This is a **hardware timing or communication issue**, not a missing ROM issue.
