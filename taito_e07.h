// license:LGPL-2.1+
// copyright-holders:Ville Linde, Angelo Salese, hap
/*************************************************************************

  Taito E07-11 DSP (TMS320C51 variant with internal ROM)
  
  Used in: Taito JC System arcade games
  - Side by Side (1996)
  - Side by Side 2 (1997)
  - Landing Gear (1995)
  - Densha de GO series (1996-1998)
  - Dangerous Curves (1995)
  
  This is a TMS320C51 with custom internal ROM programmed by
  Texas Instruments specifically for Taito. The internal ROM
  (4K words at 0x0000-0x0FFF) has never been dumped from hardware.
  
  This implementation provides a stub ROM to allow games to boot
  until the real ROM can be extracted from arcade hardware.

*************************************************************************/

#ifndef MAME_CPU_TMS320C5X_TAITO_E07_H
#define MAME_CPU_TMS320C5X_TAITO_E07_H

#pragma once

#include "tms320c5x.h"

class taito_e07_device : public tms320c51_device
{
public:
	// construction/destruction
	taito_e07_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	// device_t implementation
	virtual void device_start() override ATTR_COLD;
	
	// Internal memory maps specific to Taito E07-11
	void taito_e07_internal_pgm(address_map &map) ATTR_COLD;
	void taito_e07_internal_data(address_map &map) ATTR_COLD;
	
	// Stub implementation for internal ROM
	uint16_t internal_rom_r(offs_t offset);
};

DECLARE_DEVICE_TYPE(TAITO_E07, taito_e07_device)

#endif // MAME_CPU_TMS320C5X_TAITO_E07_H
