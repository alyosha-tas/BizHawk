#include <iostream>
#include <cstdint>
#include <iomanip>
#include <string>

#include "R5A22.h"
#include "SNES_Audio.h"
#include "Memory.h"
#include "Mappers.h"
#include "PPU.h"

namespace SNESHawk
{
	class SNESCore
	{
	public:
		SNESCore() 
		{
			mapper = nullptr;
			MemMap.compile_palette();
		};

		PPU ppu;
		R5A22 cpu;
		SNES_Audio apu;
		MemoryManager MemMap;
		Mapper* mapper;

		void Load_BIOS(uint8_t* bios)
		{
			MemMap.Load_BIOS(bios);
		}

		void Load_ROM(uint8_t* ext_prg_rom_1, uint32_t ext_prg_rom_size_1, uint8_t* ext_chr_rom_1, uint32_t ext_chr_rom_size_1, string MD5, bool is_PAL)
		{
			MemMap.ppu_pntr = &ppu;
			ppu.setPAL(is_PAL);
			ppu.mem_ctrl = &MemMap;
			
			MemMap.Load_PRG_ROM(ext_prg_rom_1, ext_prg_rom_size_1);
			MemMap.Load_CHR_ROM(ext_chr_rom_1, ext_chr_rom_size_1);

			// initialize the proper mapper
			Setup_Mapper(MD5);

			MemMap.mapper_pntr = &mapper[0];

			// set up pointers
			MemMap.cpu_pntr = &cpu;
			cpu.mem_ctrl = &MemMap;
			
			MemMap.apu_pntr = &apu;
			apu.mem_ctrl = &MemMap;
			apu._irq_apu = &MemMap.APU_IRQ;

			ppu.mem_ctrl = &MemMap;
			ppu.NMI = &cpu.NMI;
			ppu.Frame = &MemMap.Frame;
			ppu.TotalExecutedCycles = &cpu.TotalExecutedCycles;

			MemMap.mapper_pntr->ROM_Length = &MemMap.ROM_Length;
			MemMap.mapper_pntr->Cart_RAM_Length = &MemMap.Cart_RAM_Length;
			MemMap.mapper_pntr->ROM = &MemMap.ROM[0];
			MemMap.mapper_pntr->Cart_RAM = &MemMap.Cart_RAM[0];
		}

		void Reset() 
		{
			MemMap.in_vblank = true; // we start off in vblank since the LCD is off
			MemMap.in_vblank_old = true;

			MemMap.Register_Reset();
			ppu.Reset();
			apu.NESHardReset();
			mapper->Reset();
			cpu.Reset();
		}

		bool FrameAdvance(uint8_t new_controller_1, uint8_t new_controller_2, bool render, bool rendersound, bool hard_reset, bool soft_reset)
		{
			MemMap.new_controller_1 = new_controller_1;
			MemMap.new_controller_2 = new_controller_2;
			
			MemMap.lagged = true;
			if (soft_reset)
			{
				mapper->Reset();
				cpu.Reset();
				apu.NESSoftReset();
				ppu.NESSoftReset();
			}
			else if (hard_reset)
			{
				Reset();
			}

			MemMap.Frame++;

			if (ppu.ppudead > 0)
			{
				while (ppu.ppudead > 0)
				{
					ppu.NewDeadPPU();
				}
			}
			else
			{
				// do the vbl ticks seperate, that will save us a few checks that don't happen in active region
				while (ppu.do_vbl)
				{
					ppu.TickPPU_VBL();
				}

				// now do the rest of the frame
				while (ppu.do_active_sl)
				{
					ppu.TickPPU_active();
				}

				// now do the pre-NMI lines
				while (ppu.do_pre_vbl)
				{
					ppu.TickPPU_preVBL();
				}
			}

			MemMap.SendVideoBuffer();

			return MemMap.lagged;
		}

		void do_single_step()
		{
	
		}

		void GetVideo(uint32_t* dest) 
		{
			uint32_t* src = MemMap.frame_buffer;
			uint32_t* dst = dest;

			std::memcpy(dst, src, sizeof uint32_t * 256 * 240);
		}

		uint32_t GetAudio(int32_t* dest_L, int32_t* n_samp_L, int32_t* dest_R, int32_t* n_samp_R)
		{
			int32_t* src = MemMap.samples_L;
			int32_t* dst = dest_L;

			std::memcpy(dst, src, sizeof int32_t * MemMap.num_samples_L * 2);
			n_samp_L[0] = MemMap.num_samples_L;

			src = MemMap.samples_R;
			dst = dest_R;

			std::memcpy(dst, src, sizeof int32_t * MemMap.num_samples_R * 2);
			n_samp_R[0] = MemMap.num_samples_R;

			uint32_t temp_int = apu.sampleclock;
			apu.sampleclock = 0;

			MemMap.num_samples_L = 0;
			MemMap.num_samples_R = 0;

			return temp_int;
		}

		void Setup_Mapper(string MD5)
		{
			mapper = new Mapper_Default();
		}

		#pragma region State Save / Load

		void SaveState(uint8_t* saver)
		{
			saver = MemMap.SaveState(saver);
			saver = ppu.SaveState(saver);
			saver = cpu.SaveState(saver);
						
			saver = apu.SaveState(saver);		
			saver = mapper->SaveState(saver);
		}

		void LoadState(uint8_t* loader)
		{
			loader = MemMap.LoadState(loader);
			loader = ppu.LoadState(loader);
			loader = cpu.LoadState(loader);
					
			loader = apu.LoadState(loader);
			loader = mapper->LoadState(loader);
		}

		#pragma endregion

		#pragma region Memory Domain Functions


		uint8_t GetRAM(uint32_t addr)
		{
			return MemMap.RAM[addr & 0x7FF];
		}

		uint8_t GetCIRAM(uint32_t addr)
		{
			return MemMap.ppu_pntr->CIRAM[addr & 0x7FF];
		}

		uint8_t GetOAM(uint32_t addr)
		{
			return ppu.OAM[addr & 0xFF];
		}

		uint8_t GetSysBus(uint32_t addr)
		{
			return MemMap.ReadMemory(addr);
		}

		void SetRAM(uint32_t addr, uint8_t value)
		{
			MemMap.RAM[addr & 0x7FF] = value;
		}

		void SetCIRAM(uint32_t addr, uint8_t value)
		{
			MemMap.ppu_pntr->CIRAM[addr & 0x7FF] = value;
		}

		void SetOAM(uint32_t addr, uint8_t value)
		{
			ppu.OAM[addr & 0xFF] = value;
		}

		void SetSysBus(uint32_t addr, uint8_t value)
		{
			// make poke?
			MemMap.WriteMemory(addr, value);
		}

		#pragma endregion

		#pragma region Tracer

		void SetTraceCallback(void (*callback)(int))
		{
			cpu.TraceCallback = callback;
		}

		void SetScanlineCallback(void (*callback)(void), int sl)
		{
			ppu.PPUViewCallback = callback;
			ppu.PPUViewCallback_Scanline = sl;
		}

		int GetHeaderLength()
		{
			return 90 + 1;
		}

		int GetDisasmLength()
		{
			return 48 + 1;
		}

		int GetRegStringLength()
		{
			return 81 + 1;
		}

		void GetHeader(char* h, int l)
		{
			memcpy(h, cpu.TraceHeader, l);
		}

		// the copy length l must be supplied ahead of time from GetRegStrngLength
		void GetRegisterState(char* r, int t, int l)
		{
			if (t == 0)
			{
				memcpy(r, cpu.CPURegisterState().c_str(), l);
			}
			else
			{
				memcpy(r, cpu.No_Reg, l);
			}
		}

		// the copy length l must be supplied ahead of time from GetDisasmLength
		void GetDisassembly(char* d, int t, int l)
		{
			if (t == 0)
			{
				memcpy(d, cpu.CPUDisassembly().c_str(), l);
			}
			else if (t == 1)
			{
				memcpy(d, cpu.IRQ_event, l);
			}
			else
			{
				memcpy(d, cpu.NMI_event, l);
			}
		}

		#pragma endregion		
	};
}

