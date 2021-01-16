#include <iostream>
#include <cstdint>
#include <iomanip>
#include <string>

using namespace std;

namespace SNESHawk
{
	class R5A22;
	class Timer;
	class PPU;
	class SNES_Audio;
	class SerialPort;
	class Mapper;
	
	class MemoryManager
	{
	public:
				
		MemoryManager()
		{

		};

		uint8_t ReadMemory(uint32_t);
		void WriteMemory(uint32_t, uint8_t);
		uint8_t Read_Registers(uint32_t);
		void Write_Registers(uint32_t, uint8_t);
		uint8_t read_joyport(uint32_t);
		void write_joyport(uint8_t);
		void SendVideoBuffer();

		uint8_t PeekPPU(uint32_t);
		void AddressPPU(uint32_t);
		void RunCpuOne();
		void ClockPPU();

		uint32_t num_samples_L, num_samples_R;
		int32_t samples_L[25000] = {};
		int32_t samples_R[25000] = {};

		#pragma region Declarations

		PPU* ppu_pntr = nullptr;
		SNES_Audio* apu_pntr = nullptr;
		R5A22* cpu_pntr = nullptr;
		Mapper* mapper_pntr = nullptr;
		uint8_t* ROM = nullptr;
		uint8_t* Cart_RAM = nullptr;
		uint8_t* bios_rom = nullptr;

		// initialized by core loading, not savestated
		uint32_t ROM_Length;
		uint32_t ROM_Mapper;
		uint32_t Cart_RAM_Length;

		// passed in on frame advace, not stated
		uint8_t new_controller_1, new_controller_2;

		// State
		bool lagged;
		bool in_vblank;
		bool in_vblank_old;
		bool vblank_rise;
		bool Use_MT;
		bool has_bat;
		bool is_latching;
		bool APU_IRQ;
		bool oam_dma_exec = false;
		bool dmc_dma_exec = false;
		bool dmc_realign;
		bool IRQ_delay;
		bool special_case_delay; // very ugly but the only option
		bool do_the_reread;

		uint8_t controller_1_state, controller_2_state;
		uint8_t oam_dma_byte;
		uint8_t DB; //old data bus values from previous reads
		uint8_t input_register;

		uint16_t oam_dma_addr;

		int latch_count;
		int oam_dma_index;
		int cpu_deadcounter;
		int old_s = 0;
		int sprdma_countdown;

		int32_t _scanlineCallbackLine;

		uint32_t Frame;

		uint8_t RAM[0x20000] = {};
		uint32_t frame_buffer[256 * 240] = {};

		#pragma endregion

		#pragma region Functions

		// NOTE: only called when checks pass that the files are correct
		void Load_BIOS(uint8_t* bios)
		{

		}

		void Load_PRG_ROM(uint8_t* ext_prg_rom_1, uint32_t ext_prg_rom_size_1)
		{
			ROM = new uint8_t[ext_prg_rom_size_1];

			memcpy(ROM, ext_prg_rom_1, ext_prg_rom_size_1);

			ROM_Length = ext_prg_rom_size_1;
		}

		void Register_Reset()
		{
			input_register = 0xCF; // not reading any input
		}

		#pragma endregion

		#pragma region State Save / Load

		uint8_t* SaveState(uint8_t* saver)
		{
			saver = bool_saver(lagged, saver);
			saver = bool_saver(in_vblank, saver);
			saver = bool_saver(in_vblank_old, saver);
			saver = bool_saver(vblank_rise, saver);
			saver = bool_saver(Use_MT, saver);
			saver = bool_saver(has_bat, saver);
			saver = bool_saver(is_latching, saver);
			saver = bool_saver(APU_IRQ, saver);
			saver = bool_saver(oam_dma_exec, saver);
			saver = bool_saver(dmc_dma_exec, saver);
			saver = bool_saver(dmc_realign, saver);
			saver = bool_saver(IRQ_delay, saver);
			saver = bool_saver(special_case_delay, saver);
			saver = bool_saver(do_the_reread, saver);

			saver = byte_saver(controller_1_state, saver);
			saver = byte_saver(controller_2_state, saver);
			saver = byte_saver(oam_dma_byte, saver);
			saver = byte_saver(DB, saver);
			saver = byte_saver(input_register, saver);

			saver = short_saver(oam_dma_addr, saver);

			saver = int_saver(latch_count, saver);
			saver = int_saver(oam_dma_index, saver);
			saver = int_saver(cpu_deadcounter, saver);
			saver = int_saver(old_s, saver);
			saver = int_saver(sprdma_countdown, saver);

			saver = int_saver(_scanlineCallbackLine, saver);

			saver = int_saver(Frame, saver);

			saver = byte_array_saver(RAM, saver, 0x20000);

			if (Cart_RAM_Length != 0) 
			{
				saver = byte_array_saver(Cart_RAM, saver, Cart_RAM_Length);
			}

			return saver;
		}

		uint8_t* LoadState(uint8_t* loader)
		{
			loader = bool_loader(&lagged, loader);
			loader = bool_loader(&in_vblank, loader);
			loader = bool_loader(&in_vblank_old, loader);
			loader = bool_loader(&vblank_rise, loader);
			loader = bool_loader(&Use_MT, loader);
			loader = bool_loader(&has_bat, loader);
			loader = bool_loader(&is_latching, loader);
			loader = bool_loader(&APU_IRQ, loader);
			loader = bool_loader(&oam_dma_exec, loader);
			loader = bool_loader(&dmc_dma_exec, loader);
			loader = bool_loader(&dmc_realign, loader);
			loader = bool_loader(&IRQ_delay, loader);
			loader = bool_loader(&special_case_delay, loader);
			loader = bool_loader(&do_the_reread, loader);

			loader = byte_loader(&controller_1_state, loader);
			loader = byte_loader(&controller_2_state, loader);
			loader = byte_loader(&oam_dma_byte, loader);
			loader = byte_loader(&DB, loader);
			loader = byte_loader(&input_register, loader);

			loader = short_loader(&oam_dma_addr, loader);

			loader = sint_loader(&latch_count, loader);
			loader = sint_loader(&oam_dma_index, loader);
			loader = sint_loader(&cpu_deadcounter, loader);
			loader = sint_loader(&old_s, loader);
			loader = sint_loader(&sprdma_countdown, loader);

			loader = sint_loader(&_scanlineCallbackLine, loader);

			loader = int_loader(&Frame, loader);

			loader = byte_array_loader(RAM, loader, 0x20000);

			if (Cart_RAM_Length != 0)
			{
				loader = byte_array_loader(Cart_RAM, loader, Cart_RAM_Length);
			}

			return loader;
		}

		uint8_t* bool_saver(bool to_save, uint8_t* saver)
		{
			*saver = (uint8_t)(to_save ? 1 : 0); saver++;

			return saver;
		}

		uint8_t* byte_saver(uint8_t to_save, uint8_t* saver)
		{
			*saver = to_save; saver++;

			return saver;
		}

		uint8_t* short_saver(uint16_t to_save, uint8_t* saver)
		{
			*saver = (uint8_t)(to_save & 0xFF); saver++; *saver = (uint8_t)((to_save >> 8) & 0xFF); saver++;

			return saver;
		}

		uint8_t* int_saver(uint32_t to_save, uint8_t* saver)
		{
			*saver = (uint8_t)(to_save & 0xFF); saver++; *saver = (uint8_t)((to_save >> 8) & 0xFF); saver++;
			*saver = (uint8_t)((to_save >> 16) & 0xFF); saver++; *saver = (uint8_t)((to_save >> 24) & 0xFF); saver++;

			return saver;
		}

		uint8_t* byte_array_saver(uint8_t* to_save, uint8_t* saver, int length)
		{
			for (int i = 0; i < length; i++) { *saver = to_save[i]; saver++; }

			return saver;
		}

		uint8_t* int_array_saver(uint32_t* to_save, uint8_t* saver, int length)
		{
			for (int i = 0; i < length; i++)
			{
				*saver = (uint8_t)(to_save[i] & 0xFF); saver++; *saver = (uint8_t)((to_save[i] >> 8) & 0xFF); saver++;
				*saver = (uint8_t)((to_save[i] >> 16) & 0xFF); saver++; *saver = (uint8_t)((to_save[i] >> 24) & 0xFF); saver++;
			}

			return saver;
		}

		uint8_t* bool_loader(bool* to_load, uint8_t* loader)
		{
			to_load[0] = *to_load == 1; loader++;

			return loader;
		}

		uint8_t* byte_loader(uint8_t* to_load, uint8_t* loader)
		{
			to_load[0] = *loader; loader++;

			return loader;
		}

		uint8_t* short_loader(uint16_t* to_load, uint8_t* loader)
		{
			to_load[0] = *loader; loader++; to_load[0] |= (*loader << 8); loader++;

			return loader;
		}

		uint8_t* int_loader(uint32_t* to_load, uint8_t* loader)
		{
			to_load[0] = *loader; loader++; to_load[0] |= (*loader << 8); loader++;
			to_load[0] |= (*loader << 16); loader++; to_load[0] |= (*loader << 24); loader++;

			return loader;
		}

		uint8_t* sint_loader(int32_t* to_load, uint8_t* loader)
		{
			to_load[0] = *loader; loader++; to_load[0] |= (*loader << 8); loader++;
			to_load[0] |= (*loader << 16); loader++; to_load[0] |= (*loader << 24); loader++;

			return loader;
		}

		uint8_t* byte_array_loader(uint8_t* to_load, uint8_t* loader, int length)
		{
			for (int i = 0; i < length; i++) { to_load[i] = *loader; loader++; }

			return loader;
		}

		uint8_t* int_array_loader(uint32_t* to_load, uint8_t* loader, int length)
		{
			for (int i = 0; i < length; i++)
			{
				to_load[i] = *loader; loader++; to_load[i] |= (*loader << 8); loader++;
				to_load[i] |= (*loader << 16); loader++; to_load[i] |= (*loader << 24); loader++;
			}

			return loader;
		}

		#pragma endregion
	};
}