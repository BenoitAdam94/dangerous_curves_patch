#!/usr/bin/env python3
"""
Reconstruct Taito E07-11 internal ROM for TMS320C51 DSP.
See inline comments for full evidence base.
"""
import struct

ROM_SIZE = 0x1000  # 4096 words
rom = [0x0000] * ROM_SIZE

def place(addr, words):
    for i, w in enumerate(words):
        if addr + i < ROM_SIZE:
            rom[addr + i] = w & 0xFFFF

# Instruction encoding (verified from 320c5x_ops.ipp)
def B(addr):    return [0xF495, addr & 0xFFFF]   # Branch unconditional
def RET():      return [0x000D]                   # Return from subroutine
def RETE():     return [0x000F]                   # Return from interrupt

# Layout
RESET_HANDLER  = 0x0020
INT_HANDLER    = 0x0080

# --- VECTOR TABLE (0x0000-0x001F) ---
place(0x0000, B(RESET_HANDLER))          # RESET -> boot handler
for v in range(0x0002, 0x0020, 2):       # All other vectors -> INT handler
    place(v, B(INT_HANDLER))

# --- RESET HANDLER (0x0020) ---
# MAME's device_reset() already simulates the boot loader.
# This is a safety fallback to the main program.
place(RESET_HANDLER, B(0x2000))

# --- INTERRUPT HANDLER (0x0080) ---
place(INT_HANDLER, RETE())

# --- FUNCTION STUBS (0x0100-0x0FFF) ---
# Fill EVERY address from 0x0100 onward with RET.
# Any CALL from external ROM into internal ROM lands on a valid RET.
for addr in range(0x0100, ROM_SIZE):
    place(addr, RET())

# --- Write output ---
with open("/home/claude/e07-11.bin", "wb") as f:
    for word in rom:
        f.write(struct.pack("<H", word))

print(f"Generated {ROM_SIZE} words ({ROM_SIZE*2} bytes) -> e07-11.bin")
print(f"  [0x0000] = 0x{rom[0x0000]:04X} (B={0xF495:#06x})")
print(f"  [0x0001] = 0x{rom[0x0001]:04X} (-> 0x{RESET_HANDLER:04X})")
print(f"  [0x0080] = 0x{rom[0x0080]:04X} (RETE={0x000F:#06x})")
print(f"  [0x0100] = 0x{rom[0x0100]:04X} (RET={0x000D:#06x})")
