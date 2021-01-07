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
	void MemoryManager::Load_CHR_ROM(uint8_t* ext_chr_rom_1, uint32_t ext_chr_rom_size_1)
	{
		ppu_pntr->CHR = new uint8_t[ext_chr_rom_size_1];

		memcpy(ppu_pntr->CHR, ext_chr_rom_1, ext_chr_rom_size_1);

		ppu_pntr->CHR_Length = ext_chr_rom_size_1;
	}
	
	uint8_t MemoryManager::ReadMemory(uint32_t addr)
	{
		uint8_t ret;

		if (addr >= 0x8000)
		{
			// easy optimization, since rom reads are so common, move this up (reordering the rest of these else ifs is not easy)
			//ret = mapper_pntr->ReadPrg(addr - 0x8000);
			ret = ROM[addr - 0x8000];
		}
		else if (addr < 0x0800)
		{
			ret = RAM[addr];
		}
		else if (addr < 0x2000)
		{
			ret = RAM[addr & 0x7FF];
		}
		else if (addr < 0x4000)
		{
			ret = ppu_pntr->ReadReg(addr & 7);
		}
		else if (addr < 0x4020)
		{
			ret = Read_Registers(addr); // we're not rebasing the register just to keep register names canonical
		}
		else if (addr < 0x6000)
		{
			ret = 0;// mapper_pntr->ReadExp(addr - 0x4000);
		}
		else
		{
			ret = 0;// mapper_pntr->ReadWram(addr - 0x6000);
		}

		DB = ret;
		return ret;
	}
	
	void MemoryManager::WriteMemory(uint32_t addr, uint8_t value)
	{
		if (addr < 0x0800)
		{
			RAM[addr] = value;
		}
		else if (addr < 0x2000)
		{
			RAM[addr & 0x7FF] = value;
		}
		else if (addr < 0x4000)
		{
			ppu_pntr->WriteReg(addr & 7, value);
		}
		else if (addr < 0x4020)
		{
			Write_Registers(addr, value);  //we're not rebasing the register just to keep register names canonical
		}
		else if (addr < 0x6000)
		{
			//mapper_pntr->WriteExp(addr - 0x4000, value);
		}
		else if (addr < 0x8000)
		{
			//mapper_pntr->WriteWram(addr - 0x6000, value);
		}
		else
		{
			//mapper_pntr->WritePrg(addr - 0x8000, value);
		}
	}

	uint8_t MemoryManager::Read_Registers(uint32_t addr)
	{
		uint8_t ret_spec;
		switch (addr)
		{
		case 0x4000:
		case 0x4001:
		case 0x4002:
		case 0x4003:
		case 0x4004:
		case 0x4005:
		case 0x4006:
		case 0x4007:
		case 0x4008:
		case 0x4009:
		case 0x400A:
		case 0x400B:
		case 0x400C:
		case 0x400D:
		case 0x400E:
		case 0x400F:
		case 0x4010:
		case 0x4011:
		case 0x4012:
		case 0x4013:
			return DB;
			//return apu.ReadReg(addr);
		case 0x4014: /*OAM DMA*/ break;
		case 0x4015: return (uint8_t)((uint8_t)(apu_pntr->ReadReg(addr) & 0xDF) + (uint8_t)(DB & 0x20));
		case 0x4016:
			// special hardware glitch case
			ret_spec = read_joyport(addr);
			if (do_the_reread && ppu_pntr->_region == ppu_pntr->NTSC)
			{
				ret_spec = read_joyport(addr);
				do_the_reread = false;
			}
			return ret_spec;
		case 0x4017:
			// special hardware glitch case
			ret_spec = read_joyport(addr);
			if (do_the_reread && ppu_pntr->_region == ppu_pntr->NTSC)
			{
				ret_spec = read_joyport(addr);
				do_the_reread = false;
			}
			return ret_spec;
		default:
			//Console.WriteLine("read register: {0:x4}", addr);
			break;

		}
		return DB;
	}

	void MemoryManager::Write_Registers(uint32_t addr, uint8_t value)
	{
		switch (addr)
		{
		case 0x4000:
		case 0x4001:
		case 0x4002:
		case 0x4003:
		case 0x4004:
		case 0x4005:
		case 0x4006:
		case 0x4007:
		case 0x4008:
		case 0x4009:
		case 0x400A:
		case 0x400B:
		case 0x400C:
		case 0x400D:
		case 0x400E:
		case 0x400F:
		case 0x4010:
		case 0x4011:
		case 0x4012:
		case 0x4013:
			apu_pntr->WriteReg(addr, value);
			break;
		case 0x4014:
			//schedule a sprite dma event for beginning 1 cycle in the future.
			//this receives 2 because that's just the way it works out.
			oam_dma_addr = (uint16_t)(value << 8);
			sprdma_countdown = 1;

			if (sprdma_countdown > 0)
			{
				sprdma_countdown--;
				if (sprdma_countdown == 0)
				{
					if (cpu_pntr->TotalExecutedCycles % 2 == 0)
					{
						cpu_deadcounter = 2;
					}
					else
					{
						cpu_deadcounter = 1;
					}
					oam_dma_exec = true;
					cpu_pntr->RDY = false;
					oam_dma_index = 0;
					special_case_delay = true;
				}
			}
			break;
		case 0x4015: apu_pntr->WriteReg(addr, value); break;
		case 0x4016:
			write_joyport(value);
			break;
		case 0x4017: apu_pntr->WriteReg(addr, value); break;
		default:
			//Console.WriteLine("wrote register: {0:x4} = {1:x2}", addr, val);
			break;
		}
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
		for (int j = 0; j < (256 * 240); j++) { frame_buffer[j] = palette_table[(ppu_pntr->xbuf[j] & 0x1FF)]; }
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
		///////////////////////////
		// OAM DMA start
		///////////////////////////

		if (oam_dma_exec && apu_pntr->dmc_dma_countdown != 1 && !dmc_realign)
		{
			if (cpu_deadcounter == 0)
			{

				if (oam_dma_index % 2 == 0)
				{
					oam_dma_byte = ReadMemory(oam_dma_addr);
					oam_dma_addr++;
				}
				else
				{
					WriteMemory(0x2004, oam_dma_byte);
				}
				oam_dma_index++;
				if (oam_dma_index == 512) oam_dma_exec = false;

			}
			else
			{
				cpu_deadcounter--;
			}
		}

		dmc_realign = false;

		/////////////////////////////
		// OAM DMA end
		/////////////////////////////


		/////////////////////////////
		// dmc dma start
		/////////////////////////////

		if (apu_pntr->dmc_dma_countdown > 0)
		{
			if (apu_pntr->dmc_dma_countdown == 1)
			{
				dmc_realign = true;
			}

			cpu_pntr->RDY = false;
			dmc_dma_exec = true;
			apu_pntr->dmc_dma_countdown--;
			if (apu_pntr->dmc_dma_countdown == 0)
			{
				apu_pntr->RunDMCFetch();
				dmc_dma_exec = false;
				apu_pntr->dmc_dma_countdown = -1;
				do_the_reread = true;
			}
		}

		/////////////////////////////
		// dmc dma end
		/////////////////////////////
		apu_pntr->RunOneFirst();

		if (cpu_pntr->RDY && !IRQ_delay)
		{
			cpu_pntr->IRQ = APU_IRQ;// ||  mapper_pntr->IrqSignal;
		}
		else if (special_case_delay || apu_pntr->dmc_dma_countdown == 3)
		{
			cpu_pntr->IRQ = APU_IRQ;// || mapper_pntr->IrqSignal;
			special_case_delay = false;
		}

		cpu_pntr->ExecuteOne();
		//mapper_pntr->ClockCpu();

		int s = apu_pntr->EmitSample();

		if (s != old_s)
		{
			samples_L[num_samples_L * 2] = apu_pntr->sampleclock;
			samples_L[num_samples_L * 2 + 1] = s - old_s;

			samples_R[num_samples_R * 2] = apu_pntr->sampleclock;
			samples_R[num_samples_R * 2 + 1] = s - old_s;

			num_samples_L++;
			num_samples_R++;

			old_s = s;
		}
		apu_pntr->sampleclock++;

		apu_pntr->RunOneLast();

		if (do_the_reread && cpu_pntr->RDY)
			do_the_reread = false;

		IRQ_delay = false;

		if (!dmc_dma_exec && !oam_dma_exec && !cpu_pntr->RDY)
		{
			cpu_pntr->RDY = true;
			IRQ_delay = true;
		}
	}
	void MemoryManager::ClockPPU()
	{

	}

	void MemoryManager::AtVsyncNMI()
	{

	}
}