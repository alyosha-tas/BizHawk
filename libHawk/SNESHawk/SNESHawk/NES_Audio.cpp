#include <cstdint>
#include <iomanip>
#include <string>

#include "Memory.h"
#include "SNES_Audio.h"

using namespace std;

namespace SNESHawk
{
	uint8_t SNES_Audio::ReadMemory(uint32_t addr)
	{
		return mem_ctrl->ReadMemory(addr);
	}
}