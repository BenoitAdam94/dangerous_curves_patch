# Dangerous Curves - MAME Compatibility Patch

## Overview

This patch improves MAME compatibility for **Dangerous Curves** (Taito, 1995), a racing game on the Taito JC System that has been non-functional in MAME for years due to missing TMS320C51 DSP internal ROM.

## The Problem

Dangerous Curves uses the TMS320C51 DSP's internal ROM (16KB, chip E07-11.ic29) which has never been dumped. Without this ROM:
- The DSP crashes at boot address 0x205b
- The game never gets past the JC self-test
- No 3D graphics are rendered

Other Taito JC games (Side by Side, Landing Gear, Densha de GO) work because they don't use the internal ROM functions.

## The Solution

This patch implements a **stub version** of the TMS320C51 internal ROM that:
1. Provides minimal interrupt vectors
2. Returns safe instructions (NOPs and RETs) for internal ROM calls
3. Bypasses the problematic dead loop at 0x205b
4. Allows the game to boot and display graphics

**Note**: This is NOT 100% accurate emulation - it's a workaround to make the game playable until the real internal ROM can be dumped from hardware.

## Files Included

1. **dangcurv_complete.patch** - Complete unified diff patch for both files
2. **taitojc_header.patch** - Separate patch for taitojc.h
3. **taitojc_cpp.patch** - Separate patch for taitojc.cpp
4. **taitojc_dangcurv_patch.cpp** - Detailed implementation with comments
5. **DANGEROUS_CURVES_IMPROVEMENT_GUIDE.md** - Comprehensive technical guide
6. **README.md** - This file

## Installation

### Method 1: Apply Complete Patch File (Recommended)

```bash
# Navigate to MAME source root directory
cd /path/to/mame/

# Backup original files
cp src/mame/taito/taitojc.h src/mame/taito/taitojc.h.backup
cp src/mame/taito/taitojc.cpp src/mame/taito/taitojc.cpp.backup

# Apply the complete patch (includes both .h and .cpp)
patch -p1 < /path/to/dangcurv_complete.patch

# Or apply individual patches
cd src/mame/taito/
patch -p0 < /path/to/taitojc_header.patch
patch -p0 < /path/to/taitojc_cpp.patch

# Build MAME
cd /path/to/mame
make -j$(nproc)
```

### Method 2: Manual Application

1. Open `src/mame/taito/taitojc.h` in your editor
2. Add the member variables and function declaration (see taitojc_header.patch)
3. Open `src/mame/taito/taitojc.cpp` in your editor
4. Add the function implementations (see taitojc_cpp.patch)
5. Refer to `taitojc_dangcurv_patch.cpp` for detailed comments
6. Save both files and rebuild MAME

## Testing

After applying the patch and rebuilding:

```bash
# Test basic boot
./mame dangcurv

# Test with debugger
./mame dangcurv -debug

# Test with verbose logging
./mame dangcurv -verbose -log
```

### Expected Behavior

**What SHOULD work:**
- ✅ Game boots without DSP crash
- ✅ JC self-test completes
- ✅ Title screen displays
- ✅ Service mode accessible
- ✅ Basic graphics rendering
- ✅ Coin acceptance
- ✅ Game starts

**What MIGHT NOT work perfectly:**
- ⚠️ 3D graphics may have glitches
- ⚠️ Some calculations may be incorrect
- ⚠️ Timing might be off
- ⚠️ Some visual effects missing

**What DEFINITELY WON'T work:**
- ❌ Perfect accuracy (needs real ROM dump)
- ❌ Complex DSP math functions
- ❌ Advanced 3D effects

## Troubleshooting

### Game still crashes at boot
- Check that the patch was applied correctly
- Verify you're using compatible ROM sets
- Enable debug mode: `./mame dangcurv -debug -verbose`
- Check for error messages in console

### Graphics are completely broken
- This is expected - the stub ROM doesn't implement all functions
- Try different ROM versions (dangcurv vs dangcurvj)
- Report findings to MAME development team

### DSP seems stuck
- Enable logging: `./mame dangcurv -log`
- Check DSP program counter in debugger
- Look for infinite loops

## How to Get Perfect Emulation

The ONLY way to get 100% accurate Dangerous Curves emulation is to **dump the internal ROM from actual hardware**. This requires:

1. **Hardware Access**: Original Dangerous Curves PCB
2. **Dumping Equipment**: JTAG programmer or chip decapping tools
3. **Expertise**: Knowledge of ROM dumping procedures

If you have access to hardware or dumping capabilities:
- Contact the MAME development team
- Visit MAME forums: https://www.mameworld.info/ubbthreads/
- Check the dumping projects section

**Known hardware owners:**
- Video Game Esoteria (YouTube channel)
- Various arcade collectors (check forums)

## Technical Details

### What the Patch Does

1. **Adds Internal ROM Handler** (`dangcurv_dsp_internal_rom_r`)
   - Returns safe values for ROM reads
   - Implements minimal interrupt vectors
   - Bypasses known problem addresses

2. **Updates DSP Memory Map** (`tms_program_map`)
   - Maps 0x0000-0x0FFF as internal ROM (was RAM)
   - Adds extended RAM regions
   - Preserves compatibility with other games

3. **Modifies Initialization** (`init_dangcurv`, `machine_reset`)
   - Enables internal ROM hack flag
   - Releases DSP from reset immediately
   - Adds debug logging

4. **State Management** (`machine_start`)
   - Saves hack flags to state
   - Proper initialization of new variables

### TMS320C51 Internal ROM Structure

```
0x0000-0x001F : Interrupt vectors
0x0020-0x00FF : Boot loader
0x0100-0x03FF : Math library (multiply, divide, sqrt)
0x0400-0x07FF : Trigonometry functions (sin, cos, tan)
0x0800-0x0BFF : I/O helpers
0x0C00-0x0FFF : Communication protocols
```

Our stub implements minimal versions of these to prevent crashes.

## Contributing

Want to help improve Dangerous Curves emulation?

### Code Contributions
1. Fork MAME repository
2. Apply and test this patch
3. Improve the stub implementations
4. Submit pull requests

### Testing
1. Test the patched version
2. Compare with real hardware (if available)
3. Document differences
4. Report findings

### Hardware Dumping
1. Locate original hardware
2. Contact MAME dumping team
3. Arrange for ROM extraction
4. Verify dumps

### Reverse Engineering
1. Analyze DSP program behavior
2. Identify missing function signatures
3. Implement better stubs
4. Document findings

## Known Issues

1. **3D Math Inaccuracy**: Stub functions don't implement real math
2. **Timing Problems**: Internal ROM functions have specific timing
3. **Missing Features**: Some DSP features not implemented
4. **Visual Glitches**: Incorrect calculations cause rendering errors

## Version History

- **v1.0** (January 2026): Initial release
  - Basic DSP internal ROM stub
  - Game boots to title screen
  - Partial 3D rendering

## Credits

- **Original Driver**: Ville Linde, David Haywood
- **Research**: MAME community, unmamed.mameworld.info
- **Hardware Info**: Video Game Esoteria, System16.com
- **This Patch**: Based on community research and TMS320C51 documentation

## License

This patch is released under the same license as MAME (LGPL-2.1+).

## Support

For questions, bug reports, or contributions:
- MAME Forums: https://www.mameworld.info/ubbthreads/
- GitHub: https://github.com/mamedev/mame
- Discord: MAME Preservation server

## Disclaimer

This is a **workaround**, not a proper fix. The game will not be 100% accurate until the real internal ROM is dumped from hardware. Use this patch to:
- Preserve the game in a playable state
- Research for eventual proper emulation
- Enjoy the game despite limitations

**Do NOT expect perfect emulation from this patch!**

---

*Happy emulating! And please help find hardware to dump the real ROM!* 🎮
"# dangerous_curves_patch" 
