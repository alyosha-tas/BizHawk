using namespace std;
#include <algorithm>

namespace SNESHawk
{
	class MemoryManager;
	
	class PPU
	{
	public:
		#pragma region PPU Base

		PPU()
		{
			for (int i = 0; i < 0x100; i++) 
			{
				OAM[i] = 0;
			}

			_region = NTSC;
			SyncRegion();

			Reset();
		}

		const uint8_t PALRAM_init[0x20] = 
		{
				0x09,0x01,0x00,0x01,0x00,0x02,0x02,0x0D,0x08,0x10,0x08,0x24,0x00,0x00,0x04,0x2C,
				0x09,0x01,0x34,0x03,0x00,0x04,0x00,0x14,0x08,0x3A,0x00,0x02,0x00,0x20,0x2C,0x08
		};

		void (*NTViewCallback)(void);
		void (*PPUViewCallback)(void);

		int NTViewCallback_Scanline = 0;
		int PPUViewCallback_Scanline = 0;

		uint8_t ReadPPU(uint32_t);
		uint8_t PeekPPU(uint32_t);
		void AddressPPU(uint32_t);
		void WritePPU(uint32_t, uint8_t);
		void RunCpuOne();
		void ClockPPU();
		void AtVsyncNMI();

		MemoryManager* mem_ctrl;

		// pointers not stated
		bool* NMI = nullptr;

		uint32_t* Frame = nullptr;

		uint64_t* TotalExecutedCycles = nullptr;

		#pragma endregion

		#pragma region Variables

		bool frame_is_done;
		bool do_the_reread;
		bool do_vbl;
		bool do_active_sl;
		bool do_pre_vbl;

		int cpu_step;
		int cpu_stepcounter;

		uint64_t _totalCycles;

		uint8_t VRAM[0x10000] = {};
		uint8_t palette[0x100];
		uint8_t OAM[0x100];

		uint16_t xbuf[256 * 240];

		#pragma endregion

		#pragma region Constants

		#pragma endregion

		#pragma region PPU functions

		enum Region { NTSC, PAL, };

		Region _region;
		Region GetRegion() { return _region; }
		void SetRegion(Region value) { _region = value; SyncRegion(); }

		void SyncRegion()
		{

		}

		//when the ppu issues a write it goes through here and into the game board
		void ppubus_write(int addr, uint8_t value)
		{

		}

		//when the ppu issues a read it goes through here and into the game board
		uint8_t ppubus_read(int addr, bool ppu, bool addr_ppu)
		{
			return 0;
		}

		//debug tools peek into the ppu through this
		uint8_t ppubus_peek(int addr)
		{
			return 0;
		}

		void SNESSoftReset()
		{
			Reset();
		}

		void Reset()
		{

		}

		void runppu()
		{

		}

		uint8_t ReadReg(int addr)
		{
			return 0;
		}

		uint8_t PeekReg(int addr)
		{

		}

		void WriteReg(int addr, uint8_t value)
		{

		}

		void pipeline(int pixelcolor, int row_check)
		{

		}

		void ppu_init_frame()
		{

		}

		void TickPPU_VBL()
		{

		}

		void TickPPU_active()
		{

		}

		void TickPPU_preVBL()
		{

		}

		void NewDeadPPU()
		{

		}

		uint8_t BitReverse(uint8_t value)
		{
			return (uint8_t)(((value & 0x01) << 7) |
							((value & 0x02) << 5) |
							((value & 0x04) << 3) |
							((value & 0x08) << 1) |
							((value & 0x10) >> 1) |
							((value & 0x20) >> 3) |
							((value & 0x40) >> 5) |
							((value & 0x80) >> 7));
		}

		#pragma endregion

		#pragma region State Save / Load

		uint8_t* SaveState(uint8_t* saver)
		{
			saver = bool_saver(frame_is_done, saver);
			saver = bool_saver(do_the_reread, saver);

			saver = bool_saver(do_vbl, saver);
			saver = bool_saver(do_active_sl, saver);
			saver = bool_saver(do_pre_vbl, saver);

			saver = int_saver(cpu_step, saver);
			saver = int_saver(cpu_stepcounter, saver);

			*saver = (uint8_t)(_totalCycles & 0xFF); saver++; *saver = (uint8_t)((_totalCycles >> 8) & 0xFF); saver++;
			*saver = (uint8_t)((_totalCycles >> 16) & 0xFF); saver++; *saver = (uint8_t)((_totalCycles >> 24) & 0xFF); saver++;
			*saver = (uint8_t)((_totalCycles >> 32) & 0xFF); saver++; *saver = (uint8_t)((_totalCycles >> 40) & 0xFF); saver++;
			*saver = (uint8_t)((_totalCycles >> 48) & 0xFF); saver++; *saver = (uint8_t)((_totalCycles >> 56) & 0xFF); saver++;

			saver = byte_array_saver(VRAM, saver, 0x10000);
			saver = byte_array_saver(palette, saver, 0x100);
			saver = byte_array_saver(OAM, saver, 0x100);

			return saver;
		}

		uint8_t* LoadState(uint8_t* loader)
		{
			loader = bool_loader(&frame_is_done, loader);
			loader = bool_loader(&do_the_reread, loader);

			loader = bool_loader(&do_vbl, loader);
			loader = bool_loader(&do_active_sl, loader);
			loader = bool_loader(&do_pre_vbl, loader);

			loader = sint_loader(&cpu_step, loader);
			loader = sint_loader(&cpu_stepcounter, loader);

			_totalCycles = *loader; loader++; _totalCycles |= ((uint64_t)*loader << 8); loader++;
			_totalCycles |= ((uint64_t)*loader << 16); loader++; _totalCycles |= ((uint64_t)*loader << 24); loader++;
			_totalCycles |= ((uint64_t)*loader << 32); loader++; _totalCycles |= ((uint64_t)*loader << 40); loader++;
			_totalCycles |= ((uint64_t)*loader << 48); loader++; _totalCycles |= ((uint64_t)*loader << 56); loader++;

			loader = byte_array_loader(VRAM, loader, 0x10000);
			loader = byte_array_loader(palette, loader, 0x100);
			loader = byte_array_loader(OAM, loader, 0x100);

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
