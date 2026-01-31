# JTAG Dumping Guide for TMS320C51 Internal ROM

## What is JTAG?

**JTAG** stands for **Joint Test Action Group** - it's an industry-standard interface (IEEE 1149.1) originally designed for testing and debugging integrated circuits.

### Primary Uses:
1. **Testing** - Verify chip connections on PCBs
2. **Debugging** - Step through code, set breakpoints
3. **Programming** - Flash firmware to chips
4. **Dumping** - Read memory contents (including internal ROM)

## Why JTAG for ROM Dumping?

### Advantages:
✅ **Non-destructive** - Chip remains intact and functional  
✅ **Reversible** - Board can be used normally after dumping  
✅ **Accurate** - Direct memory access, bit-perfect dumps  
✅ **Fast** - Can read memory at reasonable speeds  
✅ **Standard** - Well-documented protocols  

### Disadvantages:
❌ **Requires JTAG support** - Not all chips have it enabled  
❌ **May be disabled** - Manufacturers sometimes lock JTAG  
❌ **Needs equipment** - JTAG adapter/programmer required  
❌ **Technical knowledge** - Requires understanding of chip architecture  

## JTAG vs Other Dumping Methods

### 1. JTAG (Non-destructive)
- **Method**: Connect to chip's JTAG pins, read memory
- **Cost**: $20-$500 (equipment)
- **Difficulty**: Medium
- **Risk**: Very low
- **Success rate**: High (if JTAG enabled)

### 2. In-System Programming (ISP)
- **Method**: Use chip's programming interface
- **Cost**: $50-$200
- **Difficulty**: Medium
- **Risk**: Low
- **Success rate**: Medium

### 3. Optical Reading (Decapping)
- **Method**: Remove chip packaging, photograph die
- **Cost**: $500-$5000 (professional service)
- **Difficulty**: Very high
- **Risk**: High (chip destroyed)
- **Success rate**: Very high

### 4. Physical ROM Reading
- **Method**: Desolder chip, use ROM reader
- **Cost**: $100-$500
- **Difficulty**: Medium-high
- **Risk**: Medium
- **Success rate**: High (for external ROMs only)

## TMS320C51 JTAG Capabilities

### JTAG Support:
The TMS320C51 **HAS** JTAG support! Texas Instruments included it for:
- Development and debugging
- Production testing
- In-system programming

### JTAG Pins on TMS320C51:
```
Pin Name    Function
------------------------
TCK         Test Clock
TMS         Test Mode Select
TDI         Test Data In
TDO         Test Data Out
TRST        Test Reset (optional)
```

### Memory Access via JTAG:
The TMS320C51 JTAG interface allows:
- ✅ Reading program memory (including internal ROM)
- ✅ Reading data memory
- ✅ Reading registers
- ✅ Single-stepping execution
- ⚠️ **BUT** - Internal ROM may be read-protected

## Equipment Needed for JTAG Dumping

### Hardware:

1. **JTAG Adapter/Programmer** ($20-$500)
   - **Budget option**: FT2232H-based adapter (~$20-50)
   - **Mid-range**: Bus Blaster v4 (~$80)
   - **Professional**: Segger J-Link (~$400-500)
   - **DSP-specific**: Texas Instruments XDS100 (~$100)

2. **Connection Hardware**
   - JTAG cable with appropriate connector
   - Test clips or probe pins
   - Multimeter (for pin identification)
   - Magnifying glass/microscope (helpful)

3. **Power Supply**
   - The board itself (for in-circuit dumping)
   - OR bench power supply (if chip removed)

### Software:

1. **JTAG Control Software**
   - **OpenOCD** (Open On-Chip Debugger) - FREE
   - **Texas Instruments Code Composer Studio** - FREE
   - **UrJTAG** - FREE
   - **Segger J-Link Software** - FREE (with J-Link)

2. **Chip-Specific Tools**
   - TI's DSP/BIOS tools
   - TMS320C5x development kit

## Step-by-Step JTAG Dumping Process

### Phase 1: Preparation

```bash
# 1. Identify JTAG pins on the TMS320C51
# Look for these pins on the chip (QFP132 package):
#   TCK  - Pin XX
#   TMS  - Pin XX
#   TDI  - Pin XX
#   TDO  - Pin XX
#   TRST - Pin XX (if available)

# 2. Document the board
# Take high-resolution photos
# Note all chip positions
# Mark JTAG connection points
```

### Phase 2: Hardware Connection

```
1. Power down the arcade board
2. Locate the TMS320C51 chip (labeled "Taito E07-11")
3. Identify JTAG pins using datasheet
4. Connect JTAG adapter:
   - TCK to adapter TCK
   - TMS to adapter TMS
   - TDI to adapter TDI
   - TDO to adapter TDO
   - GND to adapter GND
   - Optional: TRST to adapter TRST
5. Double-check all connections
6. Power on the board
```

### Phase 3: JTAG Communication Test

```bash
# Using OpenOCD
openocd -f interface/ftdi/ft2232h.cfg \
        -f target/ti-tms320c5x.cfg

# Expected output:
# Info : JTAG tap: tms320c51.cpu tap/device found
# Info : tms320c51.cpu: hardware has 1 breakpoints, 2 watchpoints

# If you see this, JTAG is working!
```

### Phase 4: Memory Dumping

```bash
# Using OpenOCD commands
telnet localhost 4444

# In OpenOCD terminal:
> halt                          # Stop CPU
> dump_image internal_rom.bin 0x0000 0x1000  # Dump 4K words
> resume                        # Resume CPU
> exit
```

### Phase 5: Verification

```bash
# Check the dump
hexdump -C internal_rom.bin | head -n 20

# Should show:
# - Non-zero data (not all 0x00 or 0xFF)
# - Recognizable patterns
# - Valid TMS320C5x instructions

# Verify size
ls -lh internal_rom.bin
# Should be exactly 8192 bytes (4K words × 16 bits)
```

## Potential Obstacles

### 1. JTAG Disabled/Locked
**Problem**: Chip manufacturer disabled JTAG for security  
**Solution**: 
- Try other dumping methods (decapping)
- Check for undocumented JTAG enable sequences
- Contact TI technical support

### 2. Read Protection
**Problem**: Internal ROM is read-protected  
**Solution**:
- Some protection can be bypassed with voltage glitching
- May require chip decapping
- Check for known exploits

### 3. Wrong JTAG Configuration
**Problem**: Can't communicate with chip  
**Solution**:
- Verify pin connections with multimeter
- Check JTAG chain configuration
- Try different TAP IDs
- Reduce clock speed

### 4. Board Interference
**Problem**: Other chips on board interfere with JTAG  
**Solution**:
- Desolder chip (more risky)
- Disable other chips temporarily
- Use current limiting resistors

## Example OpenOCD Configuration

```tcl
# ti-tms320c51.cfg
# Configuration for TMS320C51

interface ftdi
ftdi_vid_pid 0x0403 0x6010

# JTAG pins
ftdi_layout_init 0x0008 0x000b
ftdi_layout_signal nTRST -data 0x0010

# TMS320C51 TAP
jtag newtap tms320c51 cpu -irlen 8 -expected-id 0x0b7b302f

# Target configuration
target create tms320c51.cpu c5x -chain-position tms320c51.cpu

# Memory regions
# Internal ROM: 0x0000-0x0FFF (4K words)
# RAM: 0x1000-0x1FFF
# External: 0x2000+

init
halt
```

## JTAG Dumping Services

If you don't have the equipment or expertise:

### Professional Services:
1. **Siliconpr0n** - chip decapping and photography
   - Website: siliconpr0n.org
   - Cost: ~$500-1000 per chip
   - Method: Optical reading (destructive)

2. **MAME Dumping Community**
   - Forum: mameworld.info
   - Cost: Often free (if hardware loaned)
   - Method: Varies

3. **Reverse Engineering Services**
   - Various companies offer ROM extraction
   - Cost: $200-2000
   - Methods: JTAG, decapping, or other

### DIY Community Resources:
- EEVblog forums
- /r/ReverseEngineering
- Arcade preservation groups
- Vintage computer forums

## Legal and Ethical Considerations

### Legal:
✅ **Allowed**: Dumping ROMs you own for preservation  
✅ **Allowed**: Dumping for emulation development  
⚠️ **Gray area**: Sharing dumps publicly (check jurisdiction)  
❌ **Not allowed**: Dumping for piracy  

### Ethical:
✅ **Do**: Preserve gaming history  
✅ **Do**: Share with MAME team for emulation  
✅ **Do**: Document your process  
❌ **Don't**: Damage rare hardware unnecessarily  
❌ **Don't**: Distribute for commercial gain  

## Alternatives to JTAG for TMS320C51

### 1. Manufacturer Assistance
**Contact Texas Instruments**:
- They may have reference ROMs
- Might provide development tools
- Could assist with legitimate preservation

### 2. Test Mode
**Some chips have test modes**:
- Special pin combinations
- May allow ROM reading
- Check TMS320C51 datasheet for undocumented modes

### 3. Side-Channel Analysis
**Advanced technique**:
- Monitor power consumption
- Electromagnetic emissions
- Timing analysis
- Requires specialized equipment

### 4. Firmware Re-creation
**Last resort**:
- Reverse engineer what the ROM does
- Create functional equivalent
- Time-consuming but possible

## Dangerous Curves Specific Notes

### For the E07-11 chip (TMS320C51 with Taito internal ROM):

**Known Information**:
- Chip: TMS320C51-PQ (132-pin QFP)
- Label: "Taito E07-11"
- Size: 4K words (8KB) internal ROM
- Location: IC29 on motherboard

**JTAG Likelihood**:
- ✅ TMS320C51 has JTAG capability
- ⚠️ Unknown if Taito disabled it
- ⚠️ Unknown if ROM is read-protected
- ✅ Worth trying before destructive methods

**Best Approach**:
1. Try JTAG first (non-destructive)
2. If JTAG fails, try test modes
3. If all else fails, chip decapping

## Cost Estimate

### DIY JTAG Dumping:
- JTAG adapter: $50-100
- Cables/clips: $20
- Time: 4-8 hours
- **Total: ~$70-120 + time**

### Professional JTAG Service:
- Service fee: $200-500
- Shipping: $50
- Time: 1-2 weeks
- **Total: ~$250-550**

### Chip Decapping (if JTAG fails):
- Service fee: $500-1000
- Shipping: $50
- Time: 2-4 weeks
- **Total: ~$550-1050**

## Success Stories

### Example 1: Neo Geo Audio CPU
- Chip: Z80 with internal ROM
- Method: JTAG via custom adapter
- Time: 6 hours
- Cost: $80
- Result: ✅ Perfect dump

### Example 2: Taito C-Chip
- Chip: Custom Taito MCU
- Method: Decapping + optical reading
- Time: 3 weeks
- Cost: $800
- Result: ✅ Successfully dumped (see Caps0ff blog)

### Example 3: Namco C-Chips
- Multiple chips: Custom MCUs
- Method: JTAG and decapping
- Result: ✅ Arcade preservation improved

## Next Steps for Dangerous Curves

### Immediate Action:
1. **Locate hardware** - Find someone with a Dangerous Curves board
2. **Contact Video Game Esoteria** - They have confirmed hardware
3. **Reach out to MAME team** - They may know dumpers with equipment
4. **Check arcade collector forums** - Someone may volunteer

### If You Have Hardware:
1. **Don't panic** - JTAG is non-destructive
2. **Document everything** - Photos, notes, measurements
3. **Contact MAME dumping team** - They can guide you
4. **Consider loaning hardware** - To experienced dumpers

### If You Have JTAG Equipment:
1. **Offer to help** - Post on MAME forums
2. **Coordinate with hardware owner** - Arrange access
3. **Document the process** - Help future efforts
4. **Share results** - Contribute to preservation

## Resources

### Documentation:
- TMS320C51 Datasheet: https://www.ti.com/lit/ds/sprs030a/sprs030a.pdf
- JTAG Specification: IEEE 1149.1
- OpenOCD Manual: http://openocd.org/documentation/

### Tools:
- OpenOCD: http://openocd.org/
- UrJTAG: http://urjtag.org/
- TI Code Composer: https://www.ti.com/tool/CCSTUDIO

### Communities:
- MAME Forums: https://www.mameworld.info/ubbthreads/
- EEVblog: https://www.eevblog.com/forum/
- Caps0ff Blog: https://caps0ff.blogspot.com/

### Hardware Suppliers:
- Adafruit: JTAG adapters
- SparkFun: Development tools
- Digi-Key: Professional equipment

## Conclusion

JTAG dumping is the **best first attempt** for extracting the TMS320C51 internal ROM because:

1. ✅ Non-destructive - board remains functional
2. ✅ Relatively affordable - $50-200 equipment
3. ✅ Fast - can be done in hours
4. ✅ High success rate - if not locked

**For Dangerous Curves**: JTAG should be tried before resorting to destructive chip decapping. The TMS320C51 has JTAG support, so there's a good chance it will work.

**Call to Action**: If anyone has access to Dangerous Curves hardware and JTAG equipment, please contact the MAME development team. This ROM dump would make perfect emulation possible and preserve this rare arcade game for future generations!

---

*This guide is for educational and preservation purposes only.*
