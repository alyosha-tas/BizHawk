#include <cstdint>
#include <iomanip>
#include <string>

#include "Memory.h"
#include "PPU.h"

using namespace std;

namespace SNESHawk
{
	uint8_t PPU::ReadPPU(uint32_t addr)
	{
		return 0;
	}

	uint8_t PPU::PeekPPU(uint32_t addr)
	{
		return mem_ctrl->PeekPPU(addr);
	}

	void PPU::AddressPPU(uint32_t addr)
	{
		return mem_ctrl->AddressPPU(addr);
	}

	void PPU::WritePPU(uint32_t addr, uint8_t value)
	{

	}

	void PPU::RunCpuOne()
	{
		return mem_ctrl->RunCpuOne();
	}

	void PPU::ClockPPU()
	{
		return mem_ctrl->ClockPPU();
	}

	void PPU::AtVsyncNMI()
	{
		return mem_ctrl->AtVsyncNMI();
	}
}