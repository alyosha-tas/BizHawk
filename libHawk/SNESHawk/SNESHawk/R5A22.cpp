#include <cstdint>
#include <iomanip>
#include <string>

#include "Memory.h"
#include "R5A22.h"

using namespace std;

namespace SNESHawk
{
	void R5A22::WriteMemory(uint32_t addr, uint8_t value)
	{
		mem_ctrl->WriteMemory(addr, value);
	}

	uint8_t R5A22::ReadMemory(uint32_t addr)
	{
		return mem_ctrl->ReadMemory(addr);
	}

	uint8_t R5A22::DummyReadMemory(uint32_t addr)
	{
		return 0;
	}

	uint8_t R5A22::PeekMemory(uint32_t addr)
	{
		return mem_ctrl->ReadMemory(addr);
	}
}