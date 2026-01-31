# DANGEROUS CURVES - QUICK START GUIDE

## What You Need
- MAME source code (latest version from GitHub)
- Dangerous Curves ROM set (dangcurv.zip or dangcurvj.zip)
- Build tools (make, gcc/clang)
- These patch files

## 5-Minute Installation

### Step 1: Backup Files
```bash
cd /path/to/mame/src/mame/taito/
cp taitojc.h taitojc.h.backup
cp taitojc.cpp taitojc.cpp.backup
```

### Step 2: Apply Patch
```bash
cd /path/to/mame/
patch -p1 < dangcurv_complete.patch
```

If you get errors, try the individual patches:
```bash
cd src/mame/taito/
patch -p0 < taitojc_header.patch
patch -p0 < taitojc_cpp.patch
```

### Step 3: Build MAME
```bash
cd /path/to/mame/
make -j$(nproc)
# Or for faster builds targeting only this driver:
make SUBTARGET=taitojc -j$(nproc)
```

### Step 4: Test
```bash
./mame dangcurv
```

## What to Expect

### ✅ SHOULD WORK:
- Game boots without crashing
- JC self-test completes
- Title screen displays
- Service menu accessible
- Coins accepted
- Game starts

### ⚠️ MAY HAVE ISSUES:
- 3D graphics glitches
- Some visual effects missing
- Timing inconsistencies
- Minor rendering errors

### ❌ WON'T WORK PERFECTLY:
- Advanced 3D calculations (needs real ROM)
- Complex DSP math functions
- 100% accuracy

## Troubleshooting

### Patch Fails to Apply
**Problem**: `patch: **** malformed patch at line X`
**Solution**: Download patches again, ensure no line ending conversion

**Problem**: `Hunk #1 FAILED`
**Solution**: Your MAME version may differ. Apply manually using taitojc_dangcurv_patch.cpp as reference

### Build Errors
**Problem**: `error: 'dangcurv_dsp_internal_rom_r' not declared`
**Solution**: Make sure BOTH taitojc.h and taitojc.cpp are patched

**Problem**: Linking errors
**Solution**: Clean build: `make clean && make -j$(nproc)`

### Game Still Crashes
**Problem**: DSP crash at boot
**Solution**: 
1. Check you have correct ROM set
2. Try debug mode: `./mame dangcurv -debug -verbose`
3. Check console output for error messages

**Problem**: Black screen
**Solution**: Wait 30 seconds, game may be initializing

## File Checksums (for verification)

After patching, verify your files changed correctly:

```bash
# taitojc.h should have these additions:
grep "m_dsp_internal_rom_hack" src/mame/taito/taitojc.h
grep "dangcurv_dsp_internal_rom_r" src/mame/taito/taitojc.h

# taitojc.cpp should have the implementation:
grep -A 5 "dangcurv_dsp_internal_rom_r" src/mame/taito/taitojc.cpp
```

Expected output should show the new function declaration and implementation.

## Quick Test Commands

```bash
# Basic test
./mame dangcurv

# With debugger
./mame dangcurv -debug

# With verbose logging
./mame dangcurv -verbose -log

# Skip nag screens
./mame dangcurv -skip_gameinfo

# Window mode at 2x scale
./mame dangcurv -window -scale 2
```

## Common Questions

**Q: Will this make the game 100% accurate?**
A: No. This is a workaround. For perfect accuracy, the real internal ROM needs to be dumped from hardware.

**Q: Can I submit this to MAME?**
A: Yes! The MAME team welcomes improvements. Submit via GitHub pull request.

**Q: Will it break other games?**
A: No. The patch only affects Dangerous Curves. Other Taito JC games are unaffected.

**Q: How can I help get perfect emulation?**
A: Help locate arcade hardware and arrange for the internal ROM to be dumped.

**Q: Does this work with dangcurvj (Japanese version)?**
A: Yes, both versions should work with this patch.

## Performance Tips

```bash
# Faster emulation (may reduce accuracy)
./mame dangcurv -nomouse -nosamples

# Better graphics
./mame dangcurv -video opengl -gl_glsl

# Record video
./mame dangcurv -aviwrite dangcurv.avi
```

## If Nothing Works

1. **Revert patches**: 
   ```bash
   cd /path/to/mame/src/mame/taito/
   cp taitojc.h.backup taitojc.h
   cp taitojc.cpp.backup taitojc.cpp
   ```

2. **Try manual application**:
   - Open files in text editor
   - Follow `taitojc_dangcurv_patch.cpp` line by line
   - Apply changes carefully

3. **Ask for help**:
   - MAME Forums: https://www.mameworld.info/ubbthreads/
   - GitHub Issues: https://github.com/mamedev/mame/issues
   - Include error messages and MAME version

## Success Report Template

If it works, please report success:

```
**MAME Version**: 0.XXX
**OS**: Windows/Linux/Mac
**ROM Version**: dangcurv / dangcurvj
**Status**: Working / Partial / Not Working

**What Works**:
- [x] Boots
- [x] Title screen
- [ ] 3D graphics
etc.

**What Doesn't**:
- [list issues]

**Additional Notes**:
[any other observations]
```

Post to MAME forums or GitHub!

## Next Steps After Success

1. **Test thoroughly** - Play through different modes
2. **Document issues** - Note any glitches or problems
3. **Compare with hardware** - If you have access
4. **Report findings** - Help improve the patch
5. **Spread the word** - Let other arcade enthusiasts know

## Contact & Support

- **MAME Forums**: https://www.mameworld.info/ubbthreads/
- **GitHub**: https://github.com/mamedev/mame
- **Documentation**: Read DANGEROUS_CURVES_IMPROVEMENT_GUIDE.md for technical details

---

**Remember**: This is a workaround, not a complete fix. The ultimate goal is to dump the real internal ROM from hardware. If you have access to Dangerous Curves arcade hardware or know someone who does, please contact the MAME development team!

Good luck and happy emulating! 🎮🏎️
