// license:LGPL-2.1+
// copyright-holders:Ville Linde, Angelo Salese, hap
/*************************************************************************

  Taito E07-11 DSP (TMS320C51 variant with internal ROM)

*************************************************************************/

#include "emu.h"
#include "taito_e07.h"

DEFINE_DEVICE_TYPE(TAITO_E07, taito_e07_device, "taito_e07", "Taito E07-11 DSP (TMS320C51)")

taito_e07_device::taito_e07_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: tms320c51_device(mconfig, TAITO_E07, tag, owner, clock,
		address_map_constructor(FUNC(taito_e07_device::taito_e07_internal_pgm), this),
		address_map_constructor(FUNC(taito_e07_device::taito_e07_internal_data), this))
{
}

void taito_e07_device::device_start()
{
	tms320c51_device::device_start();
	
	logerror("Taito E07-11 DSP: Using internal ROM stub\n");
	logerror("NOTE: Internal ROM (e07-11.ic29) is not dumped from hardware\n");
	logerror("      Some games may not work correctly. For perfect emulation,\n");
	logerror("      the 4K internal ROM needs to be extracted from any Taito JC board.\n");
}

void taito_e07_device::taito_e07_internal_pgm(address_map &map)
{
	// Taito E07-11 specific memory layout
	// Internal ROM at 0x0000-0x0FFF (4K words) - NOT DUMPED
	map(0x0000, 0x0fff).r(FUNC(taito_e07_device::internal_rom_r));
	
	// User RAM
	map(0x1000, 0x1fff).ram();
	
	// Standard TMS320C51 internal memory regions
	map(0x2000, 0x23ff).ram().share("saram");      // SARAM (1K words)
	map(0xfe00, 0xffff).ram().share("daram_b0");   // DARAM B0 (512 words)
	
	// Note: External memory (0x6000+) is mapped by the board driver
}

void taito_e07_device::taito_e07_internal_data(address_map &map)
{
	// Taito E07-11 uses standard TMS320C51 data memory layout
	tms320c51_device::tms320c51_internal_data(map);
}

uint16_t taito_e07_device::internal_rom_r(offs_t offset)
{
	/*
	 * Stub implementation of Taito E07-11 internal ROM
	 * 
	 * This is a temporary workaround until the real ROM can be dumped from hardware.
	 * The internal ROM likely contains:
	 * - Interrupt vectors (0x0000-0x001F)
	 * - Boot sequence and initialization
	 * - 3D math library (matrix operations, transformations)
	 * - Graphics helper functions (polygon sorting, clipping)
	 * - Communication protocol handlers (DSP <-> Main CPU)
	 * - Rendering optimizations specific to Taito TC0780FPA chips
	 * 
	 * Different games use different subsets of these functions:
	 * - Side by Side, Landing Gear: Don't call many internal ROM functions (work OK without ROM)
	 * - Dangerous Curves: Heavily uses internal ROM functions (crashes without ROM)
	 */
	
	// Interrupt vectors (0x0000-0x001F)
	// TMS320C5x uses 2-word vectors: [instruction, address]
	if (offset < 0x20)
	{
		switch(offset)
		{
			// RESET vector (address 0x0000-0x0001)
			case 0x0000: 
				return 0xF495;  // B (branch) instruction
			case 0x0001: 
				return 0x2000;  // Branch to external ROM start
			
			// INT0 / Software interrupt (0x0002-0x0003)
			case 0x0002:
				return 0xFC00;  // RET (return from interrupt)
			case 0x0003:
				return 0x0000;
			
			// INT1 / External interrupt 1 (0x0004-0x0005)
			case 0x0004:
				return 0xFC00;  // RET
			case 0x0005:
				return 0x0000;
			
			// INT2 / External interrupt 2 (0x0006-0x0007)
			case 0x0006:
				return 0xFC00;  // RET
			case 0x0007:
				return 0x0000;
			
			// INT3 / External interrupt 3 (0x0008-0x0009)
			case 0x0008:
				return 0xFC00;  // RET
			case 0x0009:
				return 0x0000;
			
			// TINT / Timer interrupt (0x000A-0x000B)
			case 0x000a:
				return 0xFC00;  // RET
			case 0x000b:
				return 0x0000;
			
			// RINT / Serial receive interrupt (0x000C-0x000D)
			case 0x000c:
				return 0xFC00;  // RET
			case 0x000d:
				return 0x0000;
			
			// XINT / Serial transmit interrupt (0x000E-0x000F)
			case 0x000e:
				return 0xFC00;  // RET
			case 0x000f:
				return 0x0000;
			
			// TRNT / TDM receive interrupt (0x0010-0x0011)
			case 0x0010:
				return 0xFC00;  // RET
			case 0x0011:
				return 0x0000;
			
			// TXNT / TDM transmit interrupt (0x0012-0x0013)
			case 0x0012:
				return 0xFC00;  // RET
			case 0x0013:
				return 0x0000;
			
			// INT4 / External interrupt 4 (0x0014-0x0015)
			case 0x0014:
				return 0xFC00;  // RET
			case 0x0015:
				return 0x0000;
			
			// Reserved vectors (0x0016-0x001F)
			default:
				return 0xCE00;  // NOP
		}
	}
	
	/*
	 * Dangerous Curves specific fix:
	 * The game crashes in a dead loop at addresses 0x205b-0x205c
	 * This appears to be waiting for some internal ROM function to complete
	 * Returning NOP allows execution to continue
	 */
	if (offset >= 0x205b && offset <= 0x205c)
	{
		logerror("Taito E07-11: Dead loop workaround at PC=%04X\n", offset);
		return 0x7F00;  // NOP
	}
	
	/*
	 * For any other internal ROM address:
	 * Return RET instruction to gracefully exit
	 * 
	 * When external ROM calls an internal ROM function that we don't have,
	 * returning RET allows the external code to continue (though results
	 * will be incorrect). This is better than crashing.
	 * 
	 * Games that don't use many internal ROM functions will work reasonably
	 * well with this approach (Side by Side, Landing Gear, Densha de GO).
	 * 
	 * Games that rely heavily on internal ROM will have issues but at least
	 * they'll boot and partially work (Dangerous Curves).
	 */
	
	// Log calls to unmapped internal ROM (for debugging)
	if ((offset & 0x00ff) == 0)  // Only log every 256 addresses to avoid spam
	{
		logerror("Taito E07-11: Unmapped internal ROM read at %04X (returning RET)\n", offset);
	}
	
	return 0xCE00;  // RET (return from subroutine)
}
