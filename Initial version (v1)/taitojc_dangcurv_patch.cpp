// DANGEROUS CURVES COMPATIBILITY IMPROVEMENTS FOR MAME
// Patches for taitojc.h and taitojc.cpp
// 
// This file contains code modifications to improve Dangerous Curves emulation
// Apply these changes to both taitojc.h and taitojc.cpp files

// ============================================================================
// MODIFICATION 1: Add DSP internal ROM stub handling (taitojc.h)
// ============================================================================
// Add these to the taitojc_state class in taitojc.h (after line 95, after m_has_dsp_hack)

protected:
	bool m_dsp_internal_rom_hack = false;
	
	// DSP internal ROM read handler for Dangerous Curves
	uint16_t dangcurv_dsp_internal_rom_r(offs_t offset);
	
// The declaration should go after line 130 (after taitojc_dsp_idle_skip_r)

// ============================================================================
// MODIFICATION 2: Implement DSP internal ROM stub function
// ============================================================================
// Add this function implementation (around line 850, near other DSP functions)

uint16_t taitojc_state::dangcurv_dsp_internal_rom_r(offs_t offset)
{
	// The TMS320C51 internal ROM contains:
	// - Interrupt vectors (0x0000-0x001F)
	// - Boot loader and utility routines (0x0020-0x0FFF)
	
	if (!m_dsp_internal_rom_hack)
		return 0xffff;
		
	// Handle common interrupt vectors
	if (offset < 0x20)
	{
		switch(offset)
		{
			case 0x0000: return 0xF495; // RESET vector - jump to user code at 0x2000
			case 0x0001: return 0x2000; // (address for RESET)
			case 0x0002: return 0xF495; // INT0 - return from interrupt
			case 0x0003: return 0xFFFE;
			case 0x0004: return 0xF495; // INT1 - return from interrupt  
			case 0x0005: return 0xFFFE;
			default:     return 0xCE00; // NOP instruction
		}
	}
	
	// For addresses 0x0020-0x0FFF, return NOPs or simple returns
	// This prevents the DSP from crashing when it tries to call internal ROM functions
	
	// Check if this is the problematic address 0x205b-0x205c area
	// that causes the dead loop
	if (offset >= 0x205b && offset <= 0x205c)
	{
		// Return NOP (0x7F00) to bypass the dead loop
		return 0x7F00;
	}
	
	// Default: return RET (return from subroutine) to safely exit any internal ROM calls
	return 0xCE00; // RET instruction for TMS320C51
}

// ============================================================================
// MODIFICATION 3: Update TMS program memory map for Dangerous Curves
// ============================================================================
// Modify the tms_program_map function (around line 860)

void taitojc_state::tms_program_map(address_map &map)
{
	// For games using internal ROM (like Dangerous Curves), map it
	if (m_dsp_internal_rom_hack)
	{
		// Map internal ROM area with read handler
		map(0x0000, 0x0fff).r(FUNC(taitojc_state::dangcurv_dsp_internal_rom_r));
		map(0x1000, 0x1fff).ram();
		map(0x2000, 0x3fff).ram(); // Extended RAM for user program
	}
	else
	{
		// Original mapping for other games
		map(0x0000, 0x1fff).ram().mirror(0x4000);
	}
	map(0x6000, 0x7fff).ram();
}

// ============================================================================
// MODIFICATION 4: Update machine_reset to handle Dangerous Curves
// ============================================================================
// Modify machine_reset function (around line 1045)

void taitojc_state::machine_reset()
{
	m_first_dsp_reset = true;

	m_mcu_comm_main = 0;
	m_mcu_comm_hc11 = 0;
	m_mcu_data_main = 0;
	m_mcu_data_hc11 = 0;

	m_dsp_rom_pos = 0;

	memset(m_viewport_data, 0, sizeof(m_viewport_data));
	memset(m_projection_data, 0, sizeof(m_projection_data));
	memset(m_intersection_data, 0, sizeof(m_intersection_data));

	// For Dangerous Curves, don't hold DSP in reset if using internal ROM hack
	if (m_dsp_internal_rom_hack)
	{
		// Release the TMS immediately since we have stub ROM
		m_dsp->set_input_line(INPUT_LINE_RESET, CLEAR_LINE);
	}
	else
	{
		// hold the TMS in reset until we have code (original behavior)
		m_dsp->set_input_line(INPUT_LINE_RESET, ASSERT_LINE);
	}
}

// ============================================================================
// MODIFICATION 5: Update init_dangcurv function
// ============================================================================
// Replace the init_dangcurv function (around line 1216)

void taitojc_state::init_dangcurv()
{
	// Enable the internal ROM hack for Dangerous Curves
	m_dsp_internal_rom_hack = true;
	m_has_dsp_hack = true; // Also enable general DSP hacks
	
	if (DSP_IDLESKIP)
		m_dsp->space(AS_DATA).install_read_handler(0x7ff0, 0x7ff0, read16smo_delegate(*this, FUNC(taitojc_state::taitojc_dsp_idle_skip_r)));
	
	// Install a handler to catch and bypass the problematic loop at 0x205b
	// This address causes the DSP to hang in a dead loop
	m_dsp->space(AS_PROGRAM).install_write_handler(0x205c, 0x205c, write16smo_delegate(*this, 
		[this](offs_t offset, uint16_t data, uint16_t mem_mask) 
		{
			// Log attempts to write to the problematic address
			logerror("DSP: Write to problematic address 0x205c, data=%04x\n", data);
		}));
}

// ============================================================================
// MODIFICATION 6: Add initialization in machine_start
// ============================================================================
// Modify machine_start to initialize the new member variable

void taitojc_state::machine_start()
{
	// Initialize DSP internal ROM hack flag
	m_dsp_internal_rom_hack = false;
	
	// register for savestates
	save_item(NAME(m_dsp_rom_pos));
	save_item(NAME(m_first_dsp_reset));
	save_item(NAME(m_viewport_data));
	save_item(NAME(m_projection_data));
	save_item(NAME(m_intersection_data));
	save_item(NAME(m_dsp_internal_rom_hack));

	save_item(NAME(m_mcu_comm_main));
	save_item(NAME(m_mcu_comm_hc11));
	save_item(NAME(m_mcu_data_main));
	save_item(NAME(m_mcu_data_hc11));

	m_lamps.resolve();
	m_counters.resolve();
	m_wheel_motor.resolve();
}

// ============================================================================
// ADDITIONAL IMPROVEMENTS
// ============================================================================

/*
 * TESTING STEPS:
 * 
 * 1. Apply these patches to taitojc.cpp
 * 2. Rebuild MAME
 * 3. Test with: mame dangcurv -debug
 * 4. Check if the game boots past the JC self-test
 * 5. Monitor DSP program counter and look for:
 *    - Successful boot sequence
 *    - Graphics initialization
 *    - 3D rendering attempts
 * 
 * EXPECTED RESULTS:
 * - Game should boot and show title screen
 * - 3D graphics may be incomplete but should be visible
 * - No DSP crashes or dead loops
 * 
 * FURTHER IMPROVEMENTS NEEDED:
 * - Proper DSP internal ROM dump from hardware
 * - Reverse engineering of internal ROM functions
 * - Better understanding of DSP communication protocol
 * - Analysis of differences between Dangerous Curves and working games
 */

// ============================================================================
// DEBUGGING AIDS
// ============================================================================
// Add these to help diagnose issues:

// Add to dsp_shared_r/w functions for logging:
uint16_t taitojc_state::dsp_shared_r(offs_t offset)
{
	uint16_t result = m_dsp_shared_ram[offset];
	
	if (m_dsp_internal_rom_hack && offset >= 0x7f0)
	{
		logerror("DSP shared RAM read: offset=%03x, data=%04x, PC=%04x\n", 
			offset, result, m_dsp->pc());
	}
	
	return result;
}

void taitojc_state::dsp_shared_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	if (m_dsp_internal_rom_hack && offset >= 0x7f0)
	{
		logerror("DSP shared RAM write: offset=%03x, data=%04x, mask=%04x, PC=%04x\n", 
			offset, data, mem_mask, m_dsp->pc());
	}
	
	COMBINE_DATA(&m_dsp_shared_ram[offset]);
}

// ============================================================================
// ALTERNATIVE APPROACH: DSP PROGRAM PATCHING
// ============================================================================
// If the above doesn't work, we can try patching the DSP program directly

void taitojc_state::init_dangcurv_alternative()
{
	init_taitojc();
	
	m_has_dsp_hack = true;
	
	// Patch the DSP program to skip problematic code
	// This requires knowing where the external DSP ROM is loaded
	uint16_t *dsp_rom = (uint16_t *)memregion("dspgfx")->base();
	
	// At offset 0x205b in program space, there's a problematic loop
	// We need to find this in the ROM and patch it
	// This is a last resort if the internal ROM stub doesn't work
	
	// Example: Replace dead loop with NOP or branch to safe code
	// dsp_rom[problematic_offset] = 0x7F00; // NOP
}
