# Realistic Expectations: Will Dangerous Curves Work?

## Short Answer: **Partially, But Not Correctly**

## What This Will Do ✅

### 1. Game Will Boot
- ✅ DSP won't crash immediately
- ✅ Will get past JC self-test screen
- ✅ Title screen should appear
- ✅ Can enter service mode
- ✅ Can insert coins
- ✅ Game will attempt to start

### 2. Some Graphics Will Work
- ✅ 2D graphics (sprites, text) should work fine
- ✅ Basic polygon rendering might appear
- ✅ Menu screens should be visible
- ⚠️ 3D graphics will be **severely broken**

### 3. Basic Functionality
- ✅ Controls will respond
- ✅ Sound will play
- ✅ Game logic will run
- ⚠️ Physics/calculations will be **incorrect**

## What This WON'T Do ❌

### 1. Correct 3D Graphics
**Problem**: Internal ROM contains 3D math functions

Without real ROM:
- ❌ Matrix transformations will be wrong
- ❌ Polygon sorting will fail
- ❌ Perspective calculations incorrect
- ❌ Objects may disappear or glitch
- ❌ Track geometry will be malformed

**Why**: Our stub returns RET (return) for math functions, so:
```
External ROM calls: calculate_3d_transform(x, y, z)
Our stub returns:   (nothing - just returns)
Result:            Garbage data used for rendering
```

### 2. Proper Physics
**Problem**: Internal ROM likely has physics helpers

Without real ROM:
- ❌ Car handling will feel wrong
- ❌ Collision detection may fail
- ❌ Speed calculations incorrect
- ❌ May drive through walls
- ❌ Game may be unplayable

### 3. Stable Gameplay
**Problem**: Missing functions cause cascading errors

Without real ROM:
- ❌ Game may freeze randomly
- ❌ May crash after a few seconds
- ❌ Unexpected behavior
- ❌ May loop or hang

## Comparison with Working Games

### Why Side by Side Works:
```cpp
Side by Side external ROM:
- Uses mostly external code
- Minimal internal ROM calls
- Simple 3D calculations
- Avoids complex math functions

Result: Works ~80% correctly without internal ROM
```

### Why Dangerous Curves Doesn't:
```cpp
Dangerous Curves external ROM:
- Heavily uses internal ROM
- Complex 3D track rendering
- Advanced perspective math
- Calls many internal functions

Result: Works ~20% correctly without internal ROM
```

## Real-World Example

Imagine trying to run a program when critical libraries are missing:

```python
# Your program:
import math
result = math.sqrt(100)  # Expects 10

# But math library is missing, so:
def sqrt(x):
    return None  # Our stub - just returns nothing

result = sqrt(100)  # Returns None instead of 10
print(result * 2)   # Crashes! Can't multiply None
```

That's essentially what's happening with Dangerous Curves.

## Visual Comparison

### What You'll See:

**Title Screen**: ✅ Probably works fine
```
╔══════════════════════════════╗
║   DANGEROUS CURVES           ║
║                              ║
║   [START]                    ║
║                              ║
║   © 1995 TAITO              ║
╚══════════════════════════════╝
```

**Gameplay**: ❌ Severely broken
```
╔══════════════════════════════╗
║ ▓▓▓  ░░░                    ║
║    ▓░░▓                      ║
║ ░▓▓░   ▓▓░   ← Track glitched║
║  ░░░  ▓  ░                   ║
║ [CAR]  ← Car visible but     ║
║         floating/wrong pos   ║
╚══════════════════════════════╝
```

## Technical Explanation

### What Internal ROM Likely Contains:

```cpp
// Example of what's probably in E07-11 internal ROM:

// 3D Transformation (address ~0x0100)
void transform_3d_point(int16_t x, int16_t y, int16_t z)
{
    // Matrix multiplication
    // Perspective division
    // Camera transformation
    // Returns: screen_x, screen_y, depth
}

// Polygon Sorting (address ~0x0200)
void sort_polygons(polygon_list)
{
    // Z-buffer sorting
    // Back-face culling
    // Clipping calculations
    // Returns: sorted_list
}

// Physics Helper (address ~0x0300)
void calculate_car_physics(speed, steering, road_data)
{
    // Tire grip calculation
    // Banking angles
    // Collision detection
    // Returns: new_position, new_velocity
}
```

### What Our Stub Returns:

```cpp
// Our stub for ALL these functions:
uint16_t internal_rom_r(offs_t offset)
{
    return 0xCE00;  // RET (return immediately)
}

// So when game calls transform_3d_point():
// 1. Jump to internal ROM address
// 2. Hit RET instruction immediately  
// 3. Return with NO calculation done
// 4. Game uses uninitialized/garbage data
// 5. 3D graphics are completely wrong
```

## Percentage of Functionality

Based on how other games work without internal ROM:

### Side by Side: ~75-80% functional
- Most features work
- 3D is simplified
- Playable but not perfect

### Dangerous Curves: ~15-25% functional
- Boots and shows menus
- 3D is completely broken
- Likely unplayable for actual racing

## What "Partially Working" Means

### You CAN:
✅ Boot the game
✅ See the attract mode (maybe)
✅ Navigate menus
✅ Start a race
✅ See your car (probably)
✅ Hear sounds
✅ Move controls

### You CANNOT:
❌ See correct 3D track
❌ Race properly
❌ Have correct physics
❌ Complete a lap reliably
❌ Experience the game as intended
❌ Call it "working"

## Analogy

It's like trying to drive a car where:
- ✅ Engine starts
- ✅ Lights work
- ✅ Radio plays
- ✅ You can sit in it
- ❌ Steering wheel is disconnected
- ❌ Brakes don't work properly
- ❌ Speedometer shows random numbers
- ❌ Windshield is frosted over

**Can you "drive" it?** Technically yes.
**Is it drivable?** No, not really.
**Is it safe/useful?** Definitely not.

## Progress Levels

### Level 0: Current State (Without Any Fix)
- ❌ Crashes immediately at boot
- ❌ Can't even see title screen
- **0% playable**

### Level 1: With Our Stub ROM (What We're Providing)
- ✅ Boots to title screen
- ⚠️ 3D graphics severely broken
- ⚠️ May hang/crash during gameplay
- **15-25% playable** - can see menus, maybe start race

### Level 2: With Better Stub (Improved Math Functions)
- ✅ Boots reliably
- ⚠️ 3D graphics partially working
- ⚠️ Physics somewhat functional
- **40-60% playable** - might complete a lap

### Level 3: With Real ROM Dump
- ✅ Everything works correctly
- ✅ Perfect 3D graphics
- ✅ Correct physics
- **100% playable** - perfect emulation

## Our Contribution

What we're providing moves from **Level 0 → Level 1**:

**Before**: Game crashes, unusable, 0% functional
**After**: Game boots, shows some content, ~20% functional

**Is this useful?**
- ✅ Yes, for preservation (game is at least bootable)
- ✅ Yes, for research (can study what it does)
- ✅ Yes, for motivation (shows it CAN work)
- ❌ No, for actually playing the game properly

## The Only Real Solution

### To get Dangerous Curves **actually playable**:

**YOU MUST DUMP THE E07-11 INTERNAL ROM FROM HARDWARE**

No amount of clever coding can replace the real ROM because:
1. We don't know what functions it contains
2. We don't know the exact algorithms
3. We don't know the parameters/formats
4. Reverse engineering would take years

**It's like trying to guess the exact recipe for Coca-Cola:**
- You can make something brown and fizzy
- It might taste vaguely similar
- But it won't be Coca-Cola
- Only the real recipe works

## Realistic Use Cases

### What You Can Use This For:

✅ **Preservation**
- Game exists in MAME
- Better than nothing
- Shows Taito's history

✅ **Research**
- Study game structure
- Analyze external ROM code
- Understand system architecture

✅ **Development**
- Test platform for when ROM is dumped
- Framework for improvements
- Educational resource

✅ **Motivation**
- Proof that fix is possible
- Encourages ROM dumping efforts
- Shows progress

### What You CANNOT Use This For:

❌ **Actually playing Dangerous Curves**
❌ **Accurate recreation**
❌ **Preserving gameplay experience**
❌ **Tournament/competitive play**
❌ **Fair comparison to original**

## Conclusion

### The Honest Truth:

**Will this make Dangerous Curves run?**
- Yes, it will boot and show something

**Will it run CORRECTLY?**
- No, absolutely not

**Will it be playable?**
- Probably not for actual racing

**Is it better than nothing?**
- Yes, definitely

**Is it a real fix?**
- No, it's a workaround

**What's the real fix?**
- Dump the E07-11 ROM from hardware

## Summary Table

| Feature | Without Patch | With Our Stub | With Real ROM |
|---------|---------------|---------------|---------------|
| Boots | ❌ Crash | ✅ Yes | ✅ Yes |
| Title Screen | ❌ No | ✅ Yes | ✅ Yes |
| Menu Navigation | ❌ No | ✅ Probably | ✅ Yes |
| 3D Graphics | ❌ Crash | ⚠️ Broken | ✅ Perfect |
| Physics | ❌ Crash | ⚠️ Wrong | ✅ Correct |
| Playable | ❌ 0% | ⚠️ 15-25% | ✅ 100% |
| Accurate | ❌ 0% | ❌ 15-25% | ✅ 100% |

## Final Word

This patch is:
- ✅ A significant improvement over crashing
- ✅ A step forward for preservation
- ✅ A demonstration that the game CAN work
- ✅ A foundation for future improvements
- ❌ NOT a replacement for the real ROM
- ❌ NOT "fixing" the game properly
- ❌ NOT making it playable as intended

**Think of it as life support, not a cure.**

The patient (Dangerous Curves) is alive, but not healthy.
To make it truly healthy, we need the real medicine (E07-11 ROM dump).

---

**Bottom line**: This lets you SEE Dangerous Curves boot and maybe fumble around a bit, but you won't be able to actually PLAY it properly until someone dumps the real internal ROM from hardware.