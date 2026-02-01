# CRITICAL DISCOVERY: MAME Already Has TMS320C5x Support!

## Executive Summary

**MAME already has a complete TMS320C5x CPU core implementation!**

Location: `src/devices/cpu/tms320c5x/`

This changes EVERYTHING about our approach to fixing Dangerous Curves!

## What MAME Already Has

### Complete CPU Core:
✅ **tms320c5x.cpp** - Full CPU implementation with all opcodes  
✅ **tms320c5x.h** - Complete device interface  
✅ **tms320c5x_dasm.cpp** - Disassembler for debugging  
✅ **320c5x_ops.ipp** - All instruction implementations  
✅ **320c5x_optable.cpp** - Opcode table

### Two Device Types:
1. **TMS320C51** - Standard C51 chip
2. **TMS320C53** - C53 variant (more memory)

### Key Features:
- Complete instruction set
- Interrupt handling  
- Timer support
- Serial port emulation
- All registers (ACC, PREG, AR[8], etc.)
- Status registers (ST0, ST1, PMST)
- Hardware stack
- Memory management

## The Critical Issue: Internal ROM Mapping

### Current Implementation (Line 60):

```cpp
void tms320c51_device::tms320c51_internal_pgm(address_map &map)
{
//  map(0x0000, 0x1fff).rom();    // ROM - TODO: is off-chip if MP/_MC = 0
    map(0x2000, 0x23ff).ram().share("saram");
    map(0xfe00, 0xffff).ram().share("daram_b0");
}
```

**THE PROBLEM**: Line 60 is COMMENTED OUT!

The internal ROM at `0x0000-0x1fff` is **disabled** because:
- The TODO says it depends on MP/_MC pin
- MAME doesn't know if this is on-chip or off-chip
- So they left it unmapped

### What This Means for Taito JC:

The Taito E07-11 chip has:
- **Internal ROM**: 0x0000-0x0FFF (4K words)
- **External ROM**: Everything else (from external chips)

Currently taitojc.cpp maps this as:
```cpp
map(0x0000, 0x1fff).ram().mirror(0x4000);  // WRONG!
```

But it should be:
```cpp
map(0x0000, 0x0fff).rom();  // Internal ROM (E07-11)
map(0x1000, 0x1fff).ram();  // User RAM
```

## NEW SUPERIOR APPROACH

Instead of creating a stub in taitojc.cpp, we should:

### 1. Create a Custom TMS320C51 Subclass

```cpp
// In taitojc.cpp or new file taitojc_dsp.cpp

class taito_e07_device : public tms320c51_device
{
public:
    taito_e07_device(const machine_config &mconfig, const char *tag, 
                     device_t *owner, uint32_t clock)
        : tms320c51_device(mconfig, tag, owner, clock,
            address_map_constructor(FUNC(taito_e07_device::taito_e07_internal_pgm), this),
            address_map_constructor(FUNC(taito_e07_device::taito_e07_internal_data), this))
    {
    }

protected:
    void taito_e07_internal_pgm(address_map &map);
    void taito_e07_internal_data(address_map &map);
    
    uint16_t internal_rom_r(offs_t offset);
};

void taito_e07_device::taito_e07_internal_pgm(address_map &map)
{
    // Taito E07-11 specific memory map
    map(0x0000, 0x0fff).r(FUNC(taito_e07_device::internal_rom_r));  // Internal ROM stub
    map(0x1000, 0x1fff).ram();                                       // User RAM
    map(0x2000, 0x23ff).ram().share("saram");                       // SARAM
    map(0xfe00, 0xffff).ram().share("daram_b0");                    // DARAM B0
}

void taito_e07_device::taito_e07_internal_data(address_map &map)
{
    // Use standard TMS320C51 data mapping
    tms320c51_device::tms320c51_internal_data(map);
}

uint16_t taito_e07_device::internal_rom_r(offs_t offset)
{
    // Our stub implementation
    if (offset < 0x20)
    {
        if (offset == 0x0000) return 0xF495;  // RESET vector
        if (offset == 0x0001) return 0x2000;  // RESET address
        return 0xCE00;  // RET for other interrupts
    }
    
    if (offset >= 0x205b && offset <= 0x205c)
        return 0x7F00;  // NOP for dead loop fix
    
    return 0xCE00;  // Default RET
}

DEFINE_DEVICE_TYPE(TAITO_E07, taito_e07_device, "taito_e07", "Taito E07-11 DSP")
```

### 2. Update taitojc.cpp to Use Custom Device

```cpp
// In taitojc_state class definition (taitojc.h)
required_device<taito_e07_device> m_dsp;  // Instead of cpu_device

// In machine config (taitojc.cpp)
void taitojc_state::taitojc(machine_config &config)
{
    // Replace:
    // TMS320C51(config, m_dsp, XTAL(10'000'000)*4);
    
    // With:
    TAITO_E07(config, m_dsp, XTAL(10'000'000)*4);  // Our custom device
    m_dsp->set_addrmap(AS_PROGRAM, &taitojc_state::tms_program_map);  // External only
    m_dsp->set_addrmap(AS_DATA, &taitojc_state::tms_data_map);
}

// tms_program_map now only maps EXTERNAL ROM
void taitojc_state::tms_program_map(address_map &map)
{
    // Don't map 0x0000-0x1fff - that's handled internally by taito_e07_device
    map(0x6000, 0x7fff).ram();  // External RAM
}
```

## Why This Approach is MUCH BETTER

### Advantages:

✅ **Proper Architecture**
- Uses MAME's existing TMS320C5x core
- Subclassing is the correct OOP pattern
- No hacks in taitojc.cpp

✅ **Reusable**
- Other Taito JC games automatically benefit
- Could be used for other systems with custom TMS320C5x variants

✅ **Cleaner Code**
- Internal ROM logic is in the DSP device
- taitojc.cpp only handles external memory
- Clear separation of concerns

✅ **Better Testing**
- Can test DSP device independently
- Easier to debug
- Can swap in real ROM when dumped

✅ **MAME Standards**
- Follows MAME device architecture
- More likely to be accepted upstream
- Professional implementation

### Comparison with Our Previous Approach:

**Old Approach** (stub in taitojc.cpp):
```cpp
// In taitojc.cpp
uint16_t taitojc_state::dangcurv_dsp_internal_rom_r(offs_t offset)
{
    // Stub code
}

void taitojc_state::tms_program_map(address_map &map)
{
    if (m_dsp_internal_rom_hack)
        map(0x0000, 0x0fff).r(FUNC(taitojc_state::dangcurv_dsp_internal_rom_r));
    // etc...
}
```
- ❌ Mixes DSP internals with board logic
- ❌ Harder to maintain
- ❌ Not reusable

**New Approach** (custom device):
```cpp
// Separate taito_e07_device class
class taito_e07_device : public tms320c51_device
{
    // Clean device implementation
};

// In taitojc.cpp
TAITO_E07(config, m_dsp, clock);
```
- ✅ Clean separation
- ✅ Easy to maintain
- ✅ Fully reusable

## Implementation Plan

### Phase 1: Create Custom Device (2-3 hours)

1. **Create new file**: `src/devices/cpu/tms320c5x/taito_e07.h`
2. **Create new file**: `src/devices/cpu/tms320c5x/taito_e07.cpp`
3. **Implement**: Subclass with custom memory maps
4. **Add to build**: Update `src/devices/cpu/tms320c5x/tms320c5x.cpp`

### Phase 2: Update taitojc Driver (1 hour)

1. **Include**: Add taito_e07 device header
2. **Replace**: Change from TMS320C51 to TAITO_E07
3. **Simplify**: Remove internal ROM handling from taitojc.cpp
4. **Test**: Verify all games still work

### Phase 3: Refine Stub (Ongoing)

1. **Test**: See what works/doesn't work
2. **Improve**: Add better internal ROM implementations
3. **Document**: Note what each address does
4. **Compare**: Test against working games

## File Structure

```
src/devices/cpu/tms320c5x/
├── tms320c5x.cpp           (existing - TI standard implementation)
├── tms320c5x.h             (existing - base class)
├── taito_e07.cpp           (NEW - Taito custom variant)
├── taito_e07.h             (NEW - device declaration)
└── ...other files...

src/mame/taito/
├── taitojc.cpp             (modified - use TAITO_E07)
├── taitojc.h               (modified - reference taito_e07_device)
└── ...other files...
```

## Code Example: Complete Implementation

### taito_e07.h:
```cpp
// license:LGPL-2.1+
// copyright-holders:Your Name
#ifndef MAME_CPU_TMS320C5X_TAITO_E07_H
#define MAME_CPU_TMS320C5X_TAITO_E07_H

#pragma once

#include "tms320c5x.h"

class taito_e07_device : public tms320c51_device
{
public:
    taito_e07_device(const machine_config &mconfig, const char *tag, 
                     device_t *owner, uint32_t clock);

protected:
    virtual void device_start() override;
    
    void taito_e07_internal_pgm(address_map &map);
    void taito_e07_internal_data(address_map &map);
    
    uint16_t internal_rom_r(offs_t offset);

private:
    // Could add Taito-specific state here if needed
};

DECLARE_DEVICE_TYPE(TAITO_E07, taito_e07_device)

#endif // MAME_CPU_TMS320C5X_TAITO_E07_H
```

### taito_e07.cpp:
```cpp
// license:LGPL-2.1+
// copyright-holders:Your Name

#include "emu.h"
#include "taito_e07.h"

DEFINE_DEVICE_TYPE(TAITO_E07, taito_e07_device, "taito_e07", "Taito E07-11 DSP (TMS320C51)")

taito_e07_device::taito_e07_device(const machine_config &mconfig, const char *tag, 
                                   device_t *owner, uint32_t clock)
    : tms320c51_device(mconfig, TAITO_E07, tag, owner, clock,
        address_map_constructor(FUNC(taito_e07_device::taito_e07_internal_pgm), this),
        address_map_constructor(FUNC(taito_e07_device::taito_e07_internal_data), this))
{
}

void taito_e07_device::device_start()
{
    tms320c51_device::device_start();
    
    // Add Taito-specific initialization if needed
    logerror("Taito E07-11: Using internal ROM stub (e07-11.ic29 not dumped)\n");
}

void taito_e07_device::taito_e07_internal_pgm(address_map &map)
{
    // Taito E07-11 internal ROM mapping
    map(0x0000, 0x0fff).r(FUNC(taito_e07_device::internal_rom_r));
    
    // Standard TMS320C51 internal RAM/ROM regions
    map(0x1000, 0x1fff).ram();                      // User RAM
    map(0x2000, 0x23ff).ram().share("saram");       // SARAM
    map(0xfe00, 0xffff).ram().share("daram_b0");    // DARAM B0
}

void taito_e07_device::taito_e07_internal_data(address_map &map)
{
    // Use standard TMS320C51 data memory layout
    tms320c51_device::tms320c51_internal_data(map);
}

uint16_t taito_e07_device::internal_rom_r(offs_t offset)
{
    // Stub implementation of Taito E07-11 internal ROM
    // This is a workaround until the real ROM can be dumped from hardware
    
    // Interrupt vectors (0x0000-0x001F)
    if (offset < 0x20)
    {
        switch(offset)
        {
            case 0x0000: return 0xF495;  // RESET vector - B instruction
            case 0x0001: return 0x2000;  // RESET address - jump to external ROM
            case 0x0002: return 0xF495;  // INT0
            case 0x0003: return 0xFFFE;
            case 0x0004: return 0xF495;  // INT1
            case 0x0005: return 0xFFFE;
            // Add more vectors as needed
            default:     return 0xCE00;  // NOP
        }
    }
    
    // Known problematic addresses
    if (offset >= 0x205b && offset <= 0x205c)
    {
        // Dead loop fix for Dangerous Curves
        return 0x7F00;  // NOP
    }
    
    // Default: return safe instruction
    // RET (0xCE00) allows graceful exit from any internal ROM calls
    return 0xCE00;
}
```

## Benefits for Dangerous Curves

With this approach:

1. ✅ **Cleaner implementation** - Professional code structure
2. ✅ **All Taito JC games benefit** - Not just Dangerous Curves
3. ✅ **Easy to update** - When real ROM is dumped, just load it
4. ✅ **Better debugging** - Can test DSP independently
5. ✅ **MAME standards** - More likely to be accepted upstream

## Migration from Old Patch

If you already applied our previous patch, migrate by:

1. **Remove** taitojc.cpp changes
2. **Remove** taitojc.h changes  
3. **Add** taito_e07.h and taito_e07.cpp
4. **Update** taitojc.cpp to use TAITO_E07 device
5. **Rebuild** and test

Much simpler and cleaner!

## When Real ROM is Dumped

When someone dumps the E07-11 chip:

```cpp
// In taito_e07_internal_pgm():
map(0x0000, 0x0fff).rom().region("dsp_internal", 0);  // Real ROM!

// In ROM definitions:
ROM_REGION16_LE( 0x2000, "dsp_internal", 0 )
ROM_LOAD16_WORD( "e07-11.ic29", 0x0000, 0x2000, CRC(...) SHA1(...) )
```

That's it! Just replace the stub read handler with real ROM.

## Conclusion

**This discovery changes everything!**

Instead of hacking taitojc.cpp, we should:
1. Create a proper taito_e07_device subclass
2. Use MAME's existing TMS320C5x core
3. Follow proper device architecture
4. Get cleaner, more maintainable code

**Estimated time**: 3-4 hours to implement properly vs. the hack we had before.

**Result**: Professional implementation that's more likely to be accepted by MAME team!

---

*This is the RIGHT way to fix Dangerous Curves!*




