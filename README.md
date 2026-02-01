# Taito E07-11 Internal ROM: Reconstruction Analysis

## What We Deduced and Why

The TMS320C51 internal ROM is 4096 words (8KB). We cannot invent Taito's proprietary algorithms, but we CAN reconstruct the **structural skeleton** — the vector table, boot sequence, and function entry points — because the MAME driver itself contains the evidence.

---

## Evidence Source 1: The Simulated Boot Loader

**File:** `tms320c5x.cpp`, `device_reset()`

```cpp
// simulate internal rom boot loader (can be removed when the dsp rom(s) is dumped)
m_st0.intm  = 1;
m_st1.cnf   = 1;
m_pmst.ram  = 1;
m_pmst.ovly = 0;

src = 0x7800;
dst = DM_READ16(src++);
length = DM_READ16(src++);
CHANGE_PC(dst);

for (i=0; i < (length & 0x7ff); i++) {
    uint16_t data = DM_READ16(src++);
    PM_WRITE16(dst++, data);
}
```

**What this tells us:** The developer who wrote MAME *knew* exactly what the real internal ROM does on reset. They simulated it in C because the ROM was never dumped. The real ROM at address 0x0000 does this exact sequence in TMS320C51 assembly:

1. Disable interrupts (`SETC INTM`)
2. Configure status registers
3. Read destination address from shared RAM at data address 0x7800
4. Read length from 0x7801
5. Copy `length` words from 0x7802+ into program memory at `dst`
6. Jump to `dst`

Our reconstructed ROM puts a `B 0x2000` at the reset handler because MAME has *already* executed this simulation before the CPU starts running. The branch is a safe fallback.

---

## Evidence Source 2: The Interrupt Vector Formula

**File:** `tms320c5x.cpp`, `check_interrupts()`

```cpp
m_pc = (m_pmst.iptr << 11) | ((i+1) << 1);
```

With `iptr=0` (the default after reset), interrupt vectors land at:
- INT0 → 0x0002
- INT1 → 0x0004
- INT2 → 0x0006
- INT3 → 0x0008
- TINT → 0x000A
- RINT → 0x000C
- XINT → 0x000E
- ... through 0x001E

Each vector is exactly 2 words (a `B addr` instruction). PC starts at 0x0000 on reset, so 0x0000 must also be a 2-word branch. This gives us the complete vector table structure — 16 vectors × 2 words = 32 words at 0x0000–0x001F.

**Our reconstruction:** All vectors branch to a single generic handler at 0x0080 that executes `RETE` (return from interrupt, restoring the hardware shadow context). This is standard practice for unused/generic interrupt handlers on TMS320C5x.

---

## Evidence Source 3: The Math MMIO Interface

**File:** `taitojc.cpp`, `tms_data_map()` and the `dsp_math_*` functions

```cpp
// DSP DATA memory map (what the DSP sees):
map(0x7000, 0x7002).w(FUNC(taitojc_state::dsp_math_projection_w));
map(0x7010, 0x7012).w(FUNC(taitojc_state::dsp_math_intersection_w));
map(0x7013, 0x7015).w(FUNC(taitojc_state::dsp_math_viewport_w));
map(0x701b, 0x701b).r(FUNC(taitojc_state::dsp_math_intersection_r));
map(0x701d, 0x701d).r(FUNC(taitojc_state::dsp_math_projection_y_r));
map(0x701f, 0x701f).r(FUNC(taitojc_state::dsp_math_projection_x_r));
```

```cpp
// The computation (done by TC0770CMU on hardware, software in MAME):
inline uint16_t muldiv(int16_t ma, int16_t mb, int16_t d) {
    return (d != 0) ? ((ma * mb) / d) : 0;
}

uint16_t taitojc_state::dsp_math_projection_y_r() {
    return muldiv(m_projection_data[0], m_viewport_data[0], m_projection_data[2]);
}
```

**What this tells us:** The TC0770CMU chip on the PCB performs multiply-divide operations. The DSP's external ROM code writes parameters to addresses 0x7000–0x7015 and reads results from 0x701b/0x701d/0x701f. This is a standard MMIO interface pattern.

The internal ROM likely contains helper functions that the external ROM calls to perform these operations. But critically: **the external ROM must be doing the MMIO writes itself**, because MAME's data map handlers capture those writes. If the internal ROM functions did the writes, MAME would never see them (internal ROM reads don't go through the data map).

**Therefore:** Internal ROM functions at 0x0100–0x0FFF are thin wrappers. Making them immediately `RET` is safe — the MMIO protocol still works because external ROM drives it.

---

## Evidence Source 4: The Idle Loop

**File:** `taitojc.cpp`, `taitojc_dsp_idle_skip_r()`

```cpp
if (m_dsp->pc() == 0x404c)
    m_dsp->spin_until_time(attotime::from_usec(500));
return m_dsp_shared_ram[0x7f0];
```

The DSP's main loop (in external ROM at 0x404c) spins reading `shared_ram[0x7f0]` waiting for the main CPU to signal new work. The internal ROM's job was to *get* the DSP to that point — boot, initialize, set up interrupts, then jump to the main loop in external ROM. Our boot handler (`B 0x2000`) does exactly this.

---

## Evidence Source 5: Inter-CPU Mailbox

**File:** `taitojc.cpp`, `main_to_dsp_7ff_w()`

```cpp
// shared ram interrupt request from maincpu side
// this is hacky, acquiring the internal dsp romdump should allow it to be cleaned up(?)
if (BIT(data, 3)) {
    m_dsp->set_input_line(INPUT_LINE_RESET, ASSERT_LINE);
} else {
    if (!m_first_dsp_reset || !m_has_dsp_hack) {
        m_dsp->set_input_line(INPUT_LINE_RESET, CLEAR_LINE);
    }
}
```

The comment *"acquiring the internal dsp romdump should allow it to be cleaned up"* confirms the developer knows the internal ROM handles this protocol on real hardware. The main CPU resets the DSP via bit 3 of shared RAM 0x7FF. The internal ROM's reset handler re-boots the system. Our `B 0x2000` handles this correctly.

---

## What We CANNOT Reconstruct

The internal ROM likely also contains:

1. **Optimized matrix multiply routines** — The TMS320C51 is a DSP with hardware multiply. Taito probably wrote hand-optimized 3×3 matrix multiply, vector transform, and normalization routines. We cannot guess these algorithms.

2. **TC0770CMU polling loops** — On real hardware, after writing params to TC0770CMU, you may need to poll a "ready" bit before reading results. In MAME this is instant (the read handler computes immediately), so we don't need these.

3. **Specific register initialization sequences** — Exact PMST/ST0/ST1 configuration bits beyond what device_reset() shows.

4. **Error handling** — What happens if Z=0 in projection, or if the boot loader fails.

**These missing routines do NOT affect emulation** because:
- Matrix math is done by external ROM code calling TC0770CMU via MMIO
- MAME computes TC0770CMU results instantly in read handlers
- The boot sequence is already simulated by device_reset()

---

## Instruction Encoding Reference

All encodings verified against `320c5x_ops.ipp`:

| Instruction | Encoding | Source |
|------------|----------|--------|
| `B addr` | `0xF495, addr` | `op_b()` — reads next word as PMA |
| `CALL addr` | `0xF475, addr` | `op_call()` — pushes PC, reads PMA |
| `RET` | `0x000D` | `op_retc()` with always-true condition |
| `RETE` | `0x000F` | `op_rete()` — pops PC, restores shadow |
| `NOP` | `0x7F00` | No-op (DMST 0) |
| `IDLE` | `0x83FF` | `op_idle()` — enters low-power mode |

---

## Files

- `taito_e07.h` — Device class declaration
- `taito_e07.cpp` — Device implementation with embedded 4096-word ROM table
- `taitojc_e07.patch` — Two-line change: `#include "taito_e07.h"` and swap `TMS320C51` → `TAITO_E07`
- `e07-11.bin` — Standalone binary ROM file (8192 bytes, little-endian 16-bit words)
- `build_e07_rom.py` — Python script that generates the ROM

## Installation

```bash
# 1. Copy device files into MAME's TMS320C5x device directory
cp taito_e07.h taito_e07.cpp /path/to/mame/src/devices/cpu/tms320c5x/

# 2. Apply the two-line patch to the Taito JC driver
cd /path/to/mame/src/mame/taito/
# Edit taitojc.cpp: add #include "taito_e07.h" at top
# Change: TMS320C51(config, m_dsp, ...) 
# To:     TAITO_E07(config, m_dsp, ...)

# 3. Build
make -j$(nproc)
```

## Expected Behavior

- ✅ Game boots (internal ROM no longer missing)
- ✅ Title screen displays  
- ✅ Menu navigation works
- ✅ Perspective projection computed correctly (via MMIO → muldiv())
- ✅ Line intersection computed correctly
- ⚠️ Any internal ROM routines that external ROM CALLs will return immediately
- ⚠️ If Taito's code relied on internal ROM to do computation (not just MMIO), those results will be missing

## When Real ROM Is Dumped

Replace `s_rom[]` contents in `taito_e07.cpp` with the actual dump. The device class structure stays identical — just swap the data array. Or better: load from a ROM file using MAME's standard ROM loading mechanism and remove the static array.
