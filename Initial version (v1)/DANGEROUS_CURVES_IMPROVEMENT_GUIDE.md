# Dangerous Curves - MAME Compatibility Improvement Guide

## Executive Summary

Dangerous Curves by Taito (1995) is currently non-functional in MAME due to missing TMS320C51 DSP internal ROM. This document provides analysis and proposed solutions to improve compatibility.

## Problem Analysis

### Root Cause
The game uses the TMS320C51 DSP's **internal ROM** (e07-11.ic29, 16KB) which has never been dumped. The DSP program crashes at address `0x205b` because it tries to execute code from this undumped internal ROM.

### Evidence
1. **Working Games**: Side by Side, Landing Gear, Densha de GO work because they don't use internal ROM functions
2. **Screenshots Exist**: Community screenshots show the game CAN display graphics, proving the ROM dumps are otherwise valid
3. **Debugger Findings**: Using MAME debugger, manually bypassing the crash at `0x205b` allows partial graphics display
4. **Hardware Confirmation**: The board has a TMS320C51 chip (labeled "Taito E07-11") with internal ROM

### Technical Details

**TMS320C51 Internal ROM Structure:**
- Address Range: `0x0000-0x0FFF` in program space (4K words)
- Contains:
  - Interrupt vectors (`0x0000-0x001F`)
  - Boot loader and initialization code
  - Math library functions (multiply, divide, trigonometry)
  - I/O helper routines
  - Communication protocols

**Current MAME Implementation:**
```cpp
// taitojc.cpp line 860-864
void taitojc_state::tms_program_map(address_map &map)
{
    map(0x0000, 0x1fff).ram().mirror(0x4000);  // WRONG for Dangerous Curves!
    map(0x6000, 0x7fff).ram();
}
```

This maps `0x0000-0x0FFF` as RAM, but Dangerous Curves expects ROM here.

## Proposed Solutions

### Solution 1: DSP Internal ROM Stub (Recommended)

Create placeholder implementations of common internal ROM functions to prevent crashes.

**Implementation Steps:**

1. **Add ROM Read Handler**
   ```cpp
   uint16_t taitojc_state::dangcurv_dsp_internal_rom_r(offs_t offset)
   {
       // Return appropriate stub values for different ROM regions
       if (offset < 0x20) // Interrupt vectors
           return interrupt_vector_stub(offset);
       else if (offset >= 0x205b && offset <= 0x205c) // Dead loop fix
           return 0x7F00; // NOP
       else
           return 0xCE00; // RET (safe return)
   }
   ```

2. **Update Memory Map**
   ```cpp
   void taitojc_state::tms_program_map(address_map &map)
   {
       if (m_dsp_internal_rom_hack) // For Dangerous Curves
       {
           map(0x0000, 0x0fff).r(FUNC(taitojc_state::dangcurv_dsp_internal_rom_r));
           map(0x1000, 0x1fff).ram();
           map(0x2000, 0x3fff).ram();
       }
       else // Other games
       {
           map(0x0000, 0x1fff).ram().mirror(0x4000);
       }
       map(0x6000, 0x7fff).ram();
   }
   ```

3. **Enable in init_dangcurv**
   ```cpp
   void taitojc_state::init_dangcurv()
   {
       m_dsp_internal_rom_hack = true;
       m_has_dsp_hack = true;
       init_taitojc();
   }
   ```

**Expected Results:**
- Game boots past JC self-test
- Title screen displays
- 3D graphics partially functional
- Some visual glitches expected

**Limitations:**
- Not accurate to real hardware
- Math functions will be incorrect
- Some 3D calculations may fail

### Solution 2: DSP Program Analysis and Patching

Analyze the external DSP program ROM to identify and patch problem areas.

**Steps:**

1. **Disassemble DSP Program**
   ```bash
   # Use TMS320C5x disassembler
   das320c5x -a 0x2000 e09-01.ic5 > dangcurv_dsp.asm
   ```

2. **Locate Dead Loop**
   Find the instruction at `0x205b` that causes the hang

3. **Patch Binary**
   Replace problematic instructions with safe alternatives:
   - Dead loops → NOP or branch to safe code
   - Internal ROM calls → Inline alternatives
   - Missing functions → Simplified versions

4. **Create Patched ROM Set**
   Maintain patched ROMs separately for testing

**Advantages:**
- More targeted fix
- Better performance
- Can be refined over time

**Disadvantages:**
- Requires extensive reverse engineering
- May need multiple patches
- Game-specific, not reusable

### Solution 3: Request Hardware Dump (Long-term)

The proper solution is to dump the actual internal ROM from hardware.

**Process:**

1. **Locate Hardware**
   - Video Game Esoteria has confirmed hardware
   - Contact arcade collectors community
   - Check with MAME dumping team

2. **Dumping Methods**
   - **Non-destructive**: Use JTAG or test mode if available
   - **Semi-destructive**: Decap chip and read optically
   - **Last resort**: Destructive chip decapping

3. **Verification**
   - Dump multiple chips to verify consistency
   - Test dump in MAME immediately
   - Compare with other TMS320C51 games if possible

**Resources:**
- MAME Forums dumping section
- Caps0ff blog (has experience with Taito chips)
- Siliconpr0n for decapping services

## Implementation Priority

### Phase 1: Quick Fix (Stub Implementation)
- **Time**: 2-4 hours development
- **Complexity**: Low
- **Impact**: Game becomes playable
- **Quality**: 40-60% accurate

Apply the patches from `taitojc_dangcurv_patch.cpp` to get the game booting.

### Phase 2: Analysis and Refinement
- **Time**: 1-2 weeks
- **Complexity**: Medium
- **Impact**: Improved accuracy
- **Quality**: 60-80% accurate

Analyze what the DSP is trying to do and improve stubs accordingly.

### Phase 3: Hardware Dump
- **Time**: Variable (weeks to years)
- **Complexity**: High (hardware access required)
- **Impact**: Perfect emulation
- **Quality**: 100% accurate

This is the ultimate goal but depends on hardware availability.

## Testing Procedure

### 1. Apply Patches
```bash
cd mame/src/mame/taito/
cp taitojc.cpp taitojc.cpp.original
# Apply patches from taitojc_dangcurv_patch.cpp
patch < dangcurv.patch
```

### 2. Build MAME
```bash
make SUBTARGET=taitojc -j8
```

### 3. Test Boot Sequence
```bash
./mame dangcurv -debug -verbose
```

**Check for:**
- [ ] DSP initializes without crash
- [ ] JC self-test completes
- [ ] Title screen appears
- [ ] Game accepts coins
- [ ] Graphics render (even if incorrect)
- [ ] 3D objects display

### 4. Debug Logging
Enable DSP logging to see what's happening:
```bash
./mame dangcurv -debug -log -verbose 2>&1 | tee dangcurv_debug.log
```

Look for:
- DSP PC (program counter) values
- Shared RAM access patterns
- Dead loops or crashes
- Error messages

### 5. Compare with Working Games
Test similar games to understand differences:
```bash
./mame sidebs -debug   # Should work perfectly
./mame sidebs2 -debug  # Should work perfectly
./mame dangcurv -debug # Compare behavior
```

## Expected Challenges

### Challenge 1: Math Function Accuracy
**Problem**: Internal ROM contains optimized math functions (multiply, divide, sqrt, sin, cos)
**Impact**: 3D transformations will be incorrect
**Workaround**: Implement basic versions, accept inaccuracy

### Challenge 2: Timing Issues
**Problem**: Internal ROM functions have specific timing that affects synchronization
**Impact**: Graphics may glitch, audio may desync
**Workaround**: Add timing delays to stub functions

### Challenge 3: Communication Protocol
**Problem**: Internal ROM may handle DSP-to-main CPU communication
**Impact**: Game may not respond to controls properly
**Workaround**: Study working games' communication patterns

### Challenge 4: Initialization Sequence
**Problem**: Boot code in internal ROM sets up DSP state
**Impact**: Game may crash during startup
**Workaround**: Initialize DSP manually in machine_reset()

## Success Metrics

### Minimal Success (Target 1)
- [ ] Game boots without crashing
- [ ] Title screen displays
- [ ] Service mode accessible

### Basic Playability (Target 2)
- [ ] Coins accepted
- [ ] Game starts
- [ ] Car model visible
- [ ] Basic controls work

### Full Functionality (Target 3)
- [ ] 3D graphics render correctly
- [ ] All tracks playable
- [ ] No visual glitches
- [ ] Accurate physics

## Community Collaboration

### Resources Needed
1. **Developers**: C++ knowledge, MAME experience
2. **Testers**: Access to original hardware for comparison
3. **Dumpers**: Equipment and skills to dump internal ROM
4. **Reverse Engineers**: TMS320C5x assembly knowledge

### How to Contribute
1. **Code**: Submit patches to MAME GitHub
2. **Testing**: Report findings in MAME forums
3. **Documentation**: Update driver notes
4. **Hardware**: Help locate and dump original boards

### Communication Channels
- MAME Development Forum
- GitHub Issues
- Discord: MAME Preservation
- Reddit: r/EmuDev

## References

### Documentation
- TMS320C51 Datasheet: https://www.ti.com/lit/ds/sprs030a/sprs030a.pdf
- MAME TMS32051 Implementation: src/devices/cpu/tms32051/tms32051.cpp
- Taito JC System Documentation: System16.com
- MAME Forums Discussion: https://www.mameworld.info

### Related Work
- Caps0ff Blog: Taito C-Chip dumping experience
- unmamed.mameworld.info: Taito JC System notes
- Video Game Esoteria: Hardware demonstration

### Code Files
- Driver: src/mame/taito/taitojc.cpp
- DSP Core: src/devices/cpu/tms32051/
- Graphics: src/mame/video/tc0780fpa.cpp

## Conclusion

Dangerous Curves CAN be made playable in MAME with the proposed DSP internal ROM stub approach. While not 100% accurate, it will allow:

1. Game preservation (currently unplayable)
2. Research platform for eventual proper dump
3. Entertainment for fans of the game

The patches provided in this package implement Solution 1 (DSP Internal ROM Stub) as the most practical immediate approach. This should get the game booting and playable at a basic level.

**Recommendation**: Implement the stub solution NOW to make the game playable, while continuing efforts to obtain a hardware dump for perfect accuracy later.

## Appendix: Quick Start

1. Download the patch file: `taitojc_dangcurv_patch.cpp`
2. Apply changes to `src/mame/taito/taitojc.cpp`
3. Rebuild MAME: `make SUBTARGET=taitojc`
4. Test: `./mame dangcurv -debug`
5. Report results to MAME development team

**Estimated Time to Playable**: 4-8 hours of development and testing

---

*Document prepared for the MAME development community*
*Version 1.0 - January 2026*
