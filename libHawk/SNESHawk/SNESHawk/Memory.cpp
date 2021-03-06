#include <iostream>
#include <cstdint>
#include <iomanip>
#include <string>

#include "Memory.h"
#include "R5A22.h"
#include "PPU.h"
#include "SNES_Audio.h"
#include "Mappers.h"

using namespace std;

namespace SNESHawk
{
	uint8_t MemoryManager::ReadMemory(uint32_t addr)
	{
		uint8_t ret = 0;

		if (addr < 0x400000) 
		{
			if ((addr & 0x8000) == 0)
			{
				// system region
				addr &= 0x7FFF;
				if (addr < 0x2000)
				{
					return RAM[addr & 0x1FFF];
				}
				else if (addr < 0x2100)
				{
					return 0xFF;
				}
				else if (addr < 0x2200)
				{
					// I/O Ports
					return 0;
				}
				else if (addr < 0x4000)
				{
					return 0xFF;
				}
				else if (addr < 0x4200)
				{
					// I/O Ports
					return 0;
				}
				else if (addr < 0x6000)
				{
					// I/O Ports
					return 0;
				}
				else
				{
					// Expansion
					return 0;
				}
			}
			else 
			{
				mapped_ROM[addr];
			}
		}
		else if (addr < 0x800000) 
		{
			if (addr >= 0x7E0000)
			{
				ret = RAM[addr - 0x7E0000];
			}

			else return mapped_ROM[addr];
		}
		else if (addr < 0xC00000)
		{
			if ((addr & 0x8000) == 0) 
			{
				// system region
				addr &= 0x7FFF;
				if (addr < 0x2000) 
				{
					return RAM[addr & 0x1FFF];
				}
				else if (addr < 0x2100) 
				{
					return 0xFF;
				}
				else if (addr < 0x2200)
				{
					// I/O Ports
					return 0;
				}
				else if (addr < 0x4000)
				{
					return 0xFF;
				}
				else if (addr < 0x4200)
				{
					// I/O Ports
					return 0;
				}
				else if (addr < 0x6000)
				{
					// I/O Ports
					return 0;
				}
				else 
				{
					// Expansion
					return 0;
				}
			}
			else 
			{
				mapped_ROM[addr];
			}		
		}
		else
		{
			mapped_ROM[addr];
		}

		DB = ret;
		return ret;
	}
	
	void MemoryManager::WriteMemory(uint32_t addr, uint8_t value)
	{
		if (addr < 0x400000)
		{
			if ((addr & 0x8000) == 0)
			{
				// system region
				addr &= 0x7FFF;
				if (addr < 0x2000)
				{
					RAM[addr & 0x1FFF] = value;
				}
				else if (addr < 0x2100)
				{

				}
				else if (addr < 0x2200)
				{
					// I/O Ports

				}
				else if (addr < 0x4000)
				{

				}
				else if (addr < 0x4200)
				{
					// I/O Ports

				}
				else if (addr < 0x6000)
				{
					// I/O Ports

				}
				else
				{
					// Expansion

				}
			}
			else
			{

			}
		}
		else if (addr < 0x800000)
		{
			if (addr >= 0x7E0000)
			{
				RAM[addr - 0x7E0000] = value;
			}
			else 
			{

			}
		}
		else if (addr < 0xC00000)
		{
			if ((addr & 0x8000) == 0)
			{
				// system region
				addr &= 0x7FFF;
				if (addr < 0x2000)
				{
					RAM[addr & 0x1FFF] = value;
				}
				else if (addr < 0x2100)
				{

				}
				else if (addr < 0x2200)
				{
					// I/O Ports

				}
				else if (addr < 0x4000)
				{

				}
				else if (addr < 0x4200)
				{
					// I/O Ports

				}
				else if (addr < 0x6000)
				{
					// I/O Ports

				}
				else
				{
					// Expansion

				}
			}
			else
			{

			}
		}
		else
		{

		}

		DB = value;
	}

	uint8_t MemoryManager::Read_Registers(uint32_t addr)
	{
		uint8_t ret = 0;


		return ret;
	}

	void MemoryManager::Write_Registers(uint32_t addr, uint8_t value)
	{

	}

	uint8_t MemoryManager::read_joyport(uint32_t addr)
	{
		//InputCallbacks.Call();
		lagged = false;
		uint8_t ret;
		ret = addr == 0x4016 ? controller_1_state : controller_2_state;

		if (latch_count <= 7) 
		{
			ret = (ret >> latch_count) & 1;
		}
		else 
		{
			ret = 1;
		}

		ret &= 0x1f;
		ret |= (uint8_t)(0xe0 & DB);

		if (!is_latching) 
		{
			latch_count += 1;
		}
		return ret;
	}

	void MemoryManager::write_joyport(uint8_t value)
	{
		if ((value & 1) == 1) 
		{
			controller_1_state = new_controller_1;
			controller_2_state = new_controller_2;
			latch_count = 0;
		}

		is_latching = (value & 1) == 1;
	}

	void MemoryManager::SendVideoBuffer()
	{
		for (int j = 0; j < (256 * 240); j++) { frame_buffer[j] = 0; }
	}

	uint8_t MemoryManager::PeekPPU(uint32_t addr) 
	{
		return 0;
	}

	void MemoryManager::AddressPPU(uint32_t addr)
	{

	}

	void MemoryManager::RunCpuOne()
	{

	}
	void MemoryManager::ClockPPU()
	{

	}
}