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

			//power-up palette verified by blargg's power_up_palette test.
			//he speculates that these may differ depending on the system tested..
			//and I don't see why the ppu would waste any effort setting these.. 
			//but for the sake of uniformity, we'll do it.
			for (int i = 0; i < 32; i++) 
			{
				PALRAM[i] = PALRAM_init[i];
			}

			Settings_DispBackground = true;
			Settings_DispSprites = true;

			_region = NTSC;
			SyncRegion();

			Reset();
		}

		void setPAL(bool is_pal) 
		{
			if (is_pal)
			{
				cpu_sequence[4] = 4;
			}
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

		uint8_t* CHR = nullptr;
		uint32_t CHR_Length;

		uint32_t* Frame = nullptr;

		uint64_t* TotalExecutedCycles = nullptr;

		int cpu_sequence[5] = { 3, 3, 3, 3, 3 };

		#pragma endregion

		#pragma region Variables

		bool frame_is_done;
		bool do_the_reread;

		bool Settings_DispBackground;
		bool Settings_DispSprites;
		bool Settings_AllowMoreThanEightSprites;

		bool chopdot;
		bool vtoggle;
		bool idleSynch;
		bool HasClockPPU = false;

		bool Reg_2000_vram_incr32; //(0: increment by 1, going across; 1: increment by 32, going down)
		bool Reg_2000_obj_pattern_hi; //Sprite pattern table address for 8x8 sprites (0: $0000; 1: $1000)
		bool Reg_2000_bg_pattern_hi; //Background pattern table address (0: $0000; 1: $1000)
		bool Reg_2000_obj_size_16; //Sprite size (0: 8x8 sprites; 1: 8x16 sprites)
		bool Reg_2000_ppu_layer; //PPU layer select (should always be 0 in the NES; some Nintendo arcade boards presumably had two PPUs)
		bool Reg_2000_vblank_nmi_gen; //Vertical blank NMI generation (0: off; 1: on)

		bool Reg_2001_color_disable; //Color disable (0: normal color; 1: AND all palette entries with 110000, effectively producing a monochrome display)
		bool Reg_2001_show_bg_leftmost; //Show leftmost 8 pixels of background
		bool Reg_2001_show_obj_leftmost; //Show sprites in leftmost 8 pixels
		bool Reg_2001_show_bg; //Show background
		bool Reg_2001_show_obj; //Show sprites
		bool Reg_2001_intense_green; //Intensify greens (and darken other colors)
		bool Reg_2001_intense_blue; //Intensify blues (and darken other colors)
		bool Reg_2001_intense_red; //Intensify reds (and darken other colors)

		bool Reg2002_objoverflow;  //Sprite overflow. The PPU can handle only eight sprites on one scanline and sets this bit if it starts drawing sprites.
		bool Reg2002_objhit; //Sprite 0 overlap.  Set when a nonzero pixel of sprite 0 is drawn overlapping a nonzero background pixel.  Used for raster timing.
		bool Reg2002_vblank_active;  //Vertical blank start (0: has not started; 1: has started)
		bool Reg2002_vblank_active_pending; //set if Reg2002_vblank_active is pending
		bool Reg2002_vblank_clear_pending; //ppu's clear of vblank flag is pending

		bool show_bg_new; //Show background
		bool show_obj_new; //Show sprites
		bool race_2006;
		bool race_2006_2;
		bool sprite_eval_write;
		bool is_even_cycle;
		bool sprite_zero_in_range = false;
		bool sprite_zero_go = false;
		bool ppu_was_on;
		bool do_vbl;
		bool do_active_sl;
		bool do_pre_vbl;
		bool nmi_destiny;
		bool evenOddDestiny;
		bool renderspritenow;
		bool junksprite;

		uint8_t _isVS2c05 = 0;
		uint8_t VRAMBuffer;
		uint8_t PPUGenLatch;
		uint8_t reg_2003;
		uint8_t reg_2006_2;
		uint8_t read_value;

		// this uint8_t is used to simulate open bus reads and writes
		// it should be modified by every read and write to a ppu register
		uint8_t ppu_open_bus = 0;

		int cpu_step;
		int cpu_stepcounter;
		int ppudead; //measured in frames
		int NMI_PendingInstructions;
		int preNMIlines;
		int postNMIlines;
		int ppuphase;
		int Reg_2001_intensity_lsl_6; //an optimization..

		int PPUSTATUS_sl;
		int PPUSTATUS_cycle;
		//normal clocked regs. as the game can interfere with these at any time, they need to be savestated
		int PPU_Regs_fv;//3
		int PPU_Regs_v;//1
		int PPU_Regs_h;//1
		int PPU_Regs_vt;//5
		int PPU_Regs_ht;//5
		//temp unlatched regs (need savestating, can be written to at any time)
		int PPU_Regs_int_fv;
		int PPU_Regs_int_vt;
		int PPU_Regs_int_v;
		int PPU_Regs_int_h;
		int PPU_Regs_int_ht;
		//other regs that need savestating
		int PPU_Regs_fh;//3 (horz scroll)
		int ppu_addr_temp;
		// attempt to emulate graphics pipeline behaviour
		// experimental
		int pixelcolor_latch_1;
		int soam_index;
		int soam_index_prev;
		int soam_m_index;
		int oam_index;
		int oam_index_aux;
		int soam_index_aux;
		// values here are used in sprite evaluation
		int spr_true_count;
		int yp;
		int target;
		int spriteHeight;
		// installing vram address is delayed after second write to 2006, set this up here
		int install_2006;
		int install_2001;
		int NMI_offset;
		int yp_shift;
		int sprite_eval_cycle;
		int xt;
		int xp;
		int xstart;
		int rasterpos;
		int s;
		int ppu_aux_index;
		int line;
		int patternNumber;
		int patternAddress;
		int temp_addr;

		uint64_t _totalCycles;
		uint64_t double_2007_read; // emulates a hardware bug of back to back 2007 reads

		uint8_t CIRAM[0x800] = {};
		uint8_t glitchy_reads_2003[8];
		uint8_t PALRAM[0x20];
		uint8_t OAM[0x100];
		uint8_t soam[256]; // in a real nes, this would only be 32, but we wish to allow more then 8 sprites per scanline
		uint8_t sl_sprites[3 * 256];
		
		uint16_t xbuf[256 * 240];

		uint32_t ppu_open_bus_decay_timer[8];
		uint32_t _currentLuma[64];

		uint32_t _mirroring[4] = { 0, 0, 1, 1 };


		uint32_t ApplyMirroring(uint32_t addr)
		{
			uint32_t block = (addr >> 10) & 3;
			block = _mirroring[block];
			uint32_t ofs = addr & 0x3FF;
			return (block << 10) | ofs;
		}

		struct BGDataRecord
		{
			uint8_t nt, at;
			uint8_t pt_0, pt_1;
		};

		BGDataRecord bgdata[34];

		struct TempOAM
		{
			uint8_t oam_y;
			uint8_t oam_ind;
			uint8_t oam_attr;
			uint8_t oam_x;
			uint8_t patterns_0;
			uint8_t patterns_1;
		};

		TempOAM t_oam[64];

		#pragma endregion

		#pragma region Constants

		const uint32_t start_up_offset = 2;
		const uint32_t PPU_PHASE_VBL = 0;
		const uint32_t PPU_PHASE_BG = 1;
		const uint32_t PPU_PHASE_OBJ = 2;

		// luminance of each palette value for lightgun calculations
		// this is all 101% guesses, and most certainly various levels of wrong
		const uint32_t PaletteLumaNES[64] =
		{
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 0, 0,
			32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 0, 0,
			48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 0, 0
		};

		const uint32_t PaletteLuma2C03[64] =
		{
			27, 9, 3, 22, 9, 11, 16, 20, 18, 14, 19, 25, 14, 0, 0, 0,
			45, 23, 17, 12, 14, 16, 13, 30, 27, 27, 25, 34, 28, 0, 0, 0,
			63, 42, 37, 35, 18, 37, 39, 45, 50, 44, 45, 52, 49, 0, 0, 0,
			63, 52, 48, 50, 43, 48, 54, 59, 60, 55, 54, 52, 50, 0, 0, 0,
		};

		const uint32_t PaletteLuma2C04_1[64] =
		{
			48, 35, 13, 37, 28, 14, 18, 16, 63, 27, 45, 11, 9, 50, 18, 63,
			42, 45, 12, 44, 50, 48, 54, 17, 52, 52, 0, 3, 54, 36, 18, 9,
			1, 52, 50, 45, 49, 14, 34, 14, 0, 20, 43, 16, 12, 3, 39, 0,
			0, 27, 45, 19, 55, 22, 58, 30, 12, 23, 25, 9, 60, 37, 27, 54,
		};

		const uint32_t PaletteLuma2C04_2[64] =
		{
			0, 45, 27, 55, 54, 37, 28, 52, 13, 12, 60, 43, 63, 35, 50, 25,
			12, 42, 16, 54, 34, 44, 3, 37, 18, 18, 1, 52, 48, 18, 0, 22,
			9, 54, 39, 50, 23, 12, 45, 3, 14, 52, 27, 14, 17, 0, 50, 63,
			45, 9, 45, 30, 14, 9, 16, 27, 0, 49, 20, 58, 48, 11, 19, 36,
		};

		const uint32_t PaletteLuma2C04_3[64] =
		{
			14, 37, 54, 45, 25, 63, 52, 14, 9, 0, 54, 18, 16, 54, 45, 50,
			37, 28, 11, 17, 27, 27, 30, 34, 27, 22, 0, 3, 13, 16, 43, 48,
			35, 12, 1, 58, 9, 45, 39, 63, 44, 9, 42, 18, 23, 36, 0, 12,
			49, 3, 55, 50, 20, 45, 50, 18, 19, 0, 48, 60, 12, 52, 52, 14,
		};

		const uint32_t PaletteLuma2C04_4[64] =
		{
			27, 22, 28, 50, 0, 48, 9, 30, 45, 12, 45, 1, 54, 58, 25, 55,
			37, 3, 17, 43, 0, 18, 16, 39, 45, 34, 37, 27, 9, 0, 54, 42,
			11, 19, 20, 3, 12, 14, 27, 16, 14, 54, 23, 12, 9, 60, 36, 18,
			50, 63, 18, 13, 52, 52, 63, 50, 0, 45, 35, 52, 44, 48, 49, 14,
		};


		#pragma endregion

		#pragma region PPU functions

		enum Region { NTSC, PAL, Dendy, RGB };

		Region _region;
		Region GetRegion() { return _region; }
		void SetRegion(Region value) { _region = value; SyncRegion(); }

		void SyncRegion()
		{
			switch (_region)
			{
			case NTSC:
				preNMIlines = 1; postNMIlines = 20; chopdot = true; break;
			case PAL:
				preNMIlines = 1; postNMIlines = 70; chopdot = false; break;
			case Dendy:
				preNMIlines = 51; postNMIlines = 20; chopdot = false; break;
			case RGB:
				preNMIlines = 1; postNMIlines = 20; chopdot = false; break;
			}
		}

		// true = light sensed
		bool LightGunCallback(int x, int y)
		{
			// the actual light gun circuit is very complex
			// and this doesn't do it justice at all, as expected

			const int radius = 10; // look at pixel values up to this far away, roughly
			
			int sum = 0;
			int ymin = std::max(std::max(y - radius, PPUSTATUS_sl - 20), 0);
			if (ymin > 239) { ymin = 239; }
			int ymax = std::min(y + radius, 239);
			int xmin = std::max(x - radius, 0);
			int xmax = std::min(x + radius, 255);

			int ystop = PPUSTATUS_sl - 2;
			int xstop = PPUSTATUS_cycle - 20;

			bool all_stop = false;

			int j = ymin;
			int i = xmin;
			short s = 0;
			short palcolor = 0;
			short intensity = 0;

			if (j >= ystop && i >= xstop || j > ystop) { all_stop = true; }

			while (!all_stop)
			{
				s = xbuf[j * 256 + i];
				palcolor = (short)(s & 0x3F);
				intensity = (short)((s >> 6) & 0x7);

				sum += _currentLuma[palcolor];

				i++;
				if (i > xmax)
				{
					i = xmin;
					j++;

					if (j > ymax)
					{
						all_stop = true;
					}
				}

				if (j >= ystop && i >= xstop || j > ystop) { all_stop = true; }
			}

			return sum >= 2000;
		}

		//when the ppu issues a write it goes through here and into the game board
		void ppubus_write(int addr, uint8_t value)
		{
			if (PPUSTATUS_sl >= 241 || !PPUON())
				AddressPPU(addr);

			WritePPU(addr, value);
		}

		//when the ppu issues a read it goes through here and into the game board
		uint8_t ppubus_read(int addr, bool ppu, bool addr_ppu)
		{
			//hardware doesnt touch the bus when the PPU is disabled
			if (!PPUON() && ppu)
				return 0xFF;

			if (addr_ppu)
				AddressPPU(addr);

			return ReadPPU(addr);
		}

		//debug tools peek into the ppu through this
		uint8_t ppubus_peek(int addr)
		{
			return PeekPPU(addr);
		}

		void NESSoftReset()
		{
			//this hasn't been brought up to date since NEShawk was first made.
			//in particular http://wiki.nesdev.com/w/index.php/PPU_power_up_state should be studied, but theres no use til theres test cases
			Reset();
		}

		void Reset()
		{
			regs_reset();
			ppudead = 1;
			idleSynch = false;
			ppu_open_bus = 0;
			for (int i = 0; i < 8; i++) 
			{
				ppu_open_bus_decay_timer[i] = 0;
			}
			double_2007_read = 0;
		}

		void runppu()
		{
			//run one ppu cycle at a time so we can interact with the ppu and clockPPU at high granularity			
			if (install_2006 > 0)
			{
				install_2006--;
				if (install_2006 == 0)
				{
					if (!race_2006) { PPU_Regs_install_latches(); }
					else { race_2006_2 = true; }

					//LogLine("addr wrote vt = {0}, ht = {1}", PPU_Regs_int_vt, PPU_Regs_int_ht);
					//normally the address isnt observed by the board till it gets clocked by a read or write.
					//but maybe that's just because a ppu read/write shoves it on the address bus
					//apparently this shoves it on the address bus, too, or else blargg's mmc3 tests don't pass
					//ONLY if the ppu is not rendering
					if (PPUSTATUS_sl >= 241 || !PPUON())
						AddressPPU(PPU_Regs_get_2007access());
				}
			}

			race_2006 = false;

			if (install_2001 > 0)
			{
				install_2001--;
				if (install_2001 == 0)
				{
					show_bg_new = Reg_2001_show_bg;
					show_obj_new = Reg_2001_show_obj;
				}
			}

			PPUSTATUS_cycle++;
			is_even_cycle = !is_even_cycle;

			if (PPUSTATUS_cycle >= 257 && PPUSTATUS_cycle <= 320 && PPUSTATUS_sl <= 240 && PPUON())
			{
				reg_2003 = 0;
			}

			// Here we execute a CPU instruction if enough PPU cycles have passed
			// also do other things that happen at instruction level granularity
			cpu_stepcounter++;
			if (cpu_stepcounter == 3)//cpu_sequence[cpu_step])
			{
				cpu_step++;
				if (cpu_step == 5) cpu_step = 0;
				cpu_stepcounter = 0;

				// this is where the CPU instruction is called
				RunCpuOne();

				// decay the ppu bus, approximating real behaviour
				PpuOpenBusDecay(Decay_None);

				// Check for NMIs
				if (NMI_PendingInstructions > 0)
				{
					NMI_PendingInstructions--;
					if (NMI_PendingInstructions <= 0)
					{
						NMI[0] = true;
					}
				}
			}
			/*
			if (HasClockPPU)
			{
				ClockPPU();
			}
			*/
			_totalCycles += 1;
		}

		uint8_t Reg_2000_Value_Get()
		{
			(uint8_t)(PPU_Regs_int_h |
				(PPU_Regs_int_v << 1) |
				(Reg_2000_vram_incr32 << 2) |
				(Reg_2000_obj_pattern_hi << 3) |
				(Reg_2000_bg_pattern_hi << 4) |
				(Reg_2000_obj_size_16 << 5) |
				(Reg_2000_ppu_layer << 6) |
				(Reg_2000_vblank_nmi_gen << 7));
		}

		void Reg_2000_Value_Set(uint8_t value)
		{
			PPU_Regs_int_h = value & 1;
			PPU_Regs_int_v = (value >> 1) & 1;
			Reg_2000_vram_incr32 = (value >> 2) & 1;
			Reg_2000_obj_pattern_hi = (value >> 3) & 1;
			Reg_2000_bg_pattern_hi = (value >> 4) & 1;
			Reg_2000_obj_size_16 = (value >> 5) & 1;
			Reg_2000_ppu_layer = (value >> 6) & 1;
			Reg_2000_vblank_nmi_gen = (value >> 7) & 1;
		}

		uint8_t Reg_2001_Value_Get() 
		{
			return (uint8_t)(Reg_2001_color_disable | 
							(Reg_2001_show_bg_leftmost << 1) | 
							(Reg_2001_show_obj_leftmost << 2) | 
							(Reg_2001_show_bg << 3) | 
							(Reg_2001_show_obj << 4) | 
							(Reg_2001_intense_green << 5) | 
							(Reg_2001_intense_blue << 6) | 
							(Reg_2001_intense_red << 7));
		}

		void Reg_2001_Value_Set(uint8_t value) 
		{
			Reg_2001_color_disable = (value & 1);
			Reg_2001_show_bg_leftmost = (value >> 1) & 1;
			Reg_2001_show_obj_leftmost = (value >> 2) & 1;
			Reg_2001_show_bg = (value >> 3) & 1;
			Reg_2001_show_obj = (value >> 4) & 1;
			Reg_2001_intense_blue = (value >> 6) & 1;
			Reg_2001_intense_red = (value >> 7) & 1;
			Reg_2001_intense_green = (value >> 5) & 1;
			Reg_2001_intensity_lsl_6 = ((value >> 5) & 7) << 6;
		}

		inline bool PPUON() { return show_bg_new || show_obj_new; }
		inline bool PPUSTATUS_rendering() { return ((PPUSTATUS_sl >= 0) && (PPUSTATUS_sl < 241)); }

		//cached state data. these are always reset at the beginning of a frame and don't need saving
		//but just to be safe, we're gonna save it

		void PPU_Regs_reset()
		{
			PPU_Regs_fv = PPU_Regs_v = PPU_Regs_h = PPU_Regs_vt = PPU_Regs_ht = 0;
			PPU_Regs_fh = 0;
			PPU_Regs_int_fv = PPU_Regs_int_v = PPU_Regs_int_h = PPU_Regs_int_vt = PPU_Regs_int_ht = 0;
			PPUSTATUS_cycle = 0;
			PPUSTATUS_sl = 0;
		}

		void PPU_Regs_install_latches()
		{
			PPU_Regs_fv = PPU_Regs_int_fv;
			PPU_Regs_v = PPU_Regs_int_v;
			PPU_Regs_h = PPU_Regs_int_h;
			PPU_Regs_vt = PPU_Regs_int_vt;
			PPU_Regs_ht = PPU_Regs_int_ht;
		}

		void PPU_Regs_install_h_latches()
		{
			PPU_Regs_ht = PPU_Regs_int_ht;
			PPU_Regs_h = PPU_Regs_int_h;
		}

		void PPU_Regs_increment_hsc()
		{
			//The first one, the horizontal scroll counter, consists of 6 bits, and is
			//made up by daisy-chaining the HT counter to the H counter. The HT counter is
			//then clocked every 8 pixel dot clocks (or every 8/3 CPU clock cycles).
			PPU_Regs_ht++;
			PPU_Regs_h += (PPU_Regs_ht >> 5);
			PPU_Regs_ht &= 31;
			PPU_Regs_h &= 1;
		}

		void PPU_Regs_increment_vs()
		{
			PPU_Regs_fv++;
			int fv_overflow = (PPU_Regs_fv >> 3);
			PPU_Regs_vt += fv_overflow;
			PPU_Regs_vt &= 31; //fixed tecmo super bowl
			if (PPU_Regs_vt == 30 && fv_overflow == 1) //caution here (only do it at the exact instant of overflow) fixes p'radikus conflict
			{
				PPU_Regs_v++;
				PPU_Regs_vt = 0;
			}
			PPU_Regs_fv &= 7;
			PPU_Regs_v &= 1;
		}

		int PPU_Regs_get_ntread()
		{
			return 0x2000 | (PPU_Regs_v << 0xB) | (PPU_Regs_h << 0xA) | (PPU_Regs_vt << 5) | PPU_Regs_ht;
		}

		int PPU_Regs_get_2007access()
		{
			return ((PPU_Regs_fv & 3) << 0xC) | (PPU_Regs_v << 0xB) | (PPU_Regs_h << 0xA) | (PPU_Regs_vt << 5) | PPU_Regs_ht;
		}

		//The PPU has an internal 4-position, 2-bit shifter, which it uses for
		//obtaining the 2-bit palette select data during an attribute table uint8_t
		//fetch. To represent how this data is shifted in the diagram, letters a..c
		//are used in the diagram to represent the right-shift position amount to
		//apply to the data read from the attribute data (a is always 0). This is why
		//you only see bits 0 and 1 used off the read attribute data in the diagram.
		int PPU_Regs_get_atread()
		{
			return 0x2000 | (PPU_Regs_v << 0xB) | (PPU_Regs_h << 0xA) | 0x3C0 | ((PPU_Regs_vt & 0x1C) << 1) | ((PPU_Regs_ht & 0x1C) >> 2);
		}

		void PPU_Regs_increment2007(bool rendering, bool by32)
		{
			if (rendering)
			{
				//don't do this:
				//if (by32) increment_vs();
				//else increment_hsc();
				//do this instead:
				PPU_Regs_increment_vs(); //yes, even if we're moving by 32
				return;
			}

			//If the VRAM address increment bit (2000.2) is clear (inc. amt. = 1), all the
			//scroll counters are daisy-chained (in the order of HT, VT, H, V, FV) so that
			//the carry out of each counter controls the next counter's clock rate. The
			//result is that all 5 counters function as a single 15-bit one. Any access to
			//2007 clocks the HT counter here.
			//
			//If the VRAM address increment bit is set (inc. amt. = 32), the only
			//difference is that the HT counter is no longer being clocked, and the VT
			//counter is now being clocked by access to 2007.
			if (by32)
			{
				PPU_Regs_vt++;
			}
			else
			{
				PPU_Regs_ht++;
				PPU_Regs_vt += (PPU_Regs_ht >> 5) & 1;
			}
			PPU_Regs_h += (PPU_Regs_vt >> 5);
			PPU_Regs_v += (PPU_Regs_h >> 1);
			PPU_Regs_fv += (PPU_Regs_v >> 1);
			PPU_Regs_ht &= 31;
			PPU_Regs_vt &= 31;
			PPU_Regs_h &= 1;
			PPU_Regs_v &= 1;
			PPU_Regs_fv &= 7;
		}

		void regs_reset()
		{
			PPU_Regs_reset();
			Reg2002_objoverflow = false;
			Reg2002_objhit = false;
			Reg2002_vblank_active = false;
			PPUGenLatch = 0;
			reg_2003 = 0;
			vtoggle = false;
			VRAMBuffer = 0;
		}

		//PPU CONTROL (write)
		void write_2000(uint8_t value)
		{
			if (!Reg_2000_vblank_nmi_gen && ((value & 0x80) != 0) && (Reg2002_vblank_active) && !Reg2002_vblank_clear_pending)
			{
				//if we just unleashed the vblank interrupt then activate it now
				//if (ppudead != 1)
				NMI_PendingInstructions = 2;
			}
			//if (ppudead != 1)
			Reg_2000_Value_Set(value);
		}

		uint8_t read_2000() { return ppu_open_bus; }
		uint8_t peek_2000() { return ppu_open_bus; }

		//PPU MASK (write)
		void write_2001(uint8_t value)
		{
			//printf("%04x:$%02x, %d\n",A,V,scanline);
			Reg_2001_Value_Set(value);
			install_2001 = 2;
		}

		uint8_t read_2001() { return ppu_open_bus; }
		uint8_t peek_2001() { return ppu_open_bus; }

		//PPU STATUS (read)
		void write_2002(uint8_t value) { }

		uint8_t read_2002()
		{
			uint8_t ret = peek_2002();

			// reading from $2002 resets the destination for $2005 and $2006 writes
			vtoggle = false;
			Reg2002_vblank_active = 0;
			Reg2002_vblank_active_pending = false;

			// update the open bus here
			ppu_open_bus = ret;
			PpuOpenBusDecay(Decay_High);
			return ret;
		}

			uint8_t peek_2002()
			{
				//I'm not happy with this, but apparently this is how mighty bobm jack VS works.
				//quite strange that is makes the sprite hit flag go high like this
				if (_isVS2c05 == 2)
				{
					if (Frame[0] < 4)
					{

						return (uint8_t)((Reg2002_vblank_active << 7) | (Reg2002_objhit << 6) | (1 << 5) | (0x1D));
					}
					else
					{
						return (uint8_t)((Reg2002_vblank_active << 7) | (Reg2002_objhit << 6) | (Reg2002_objoverflow << 5) | (0x1D));
					}

				}
				if (_isVS2c05 == 3)
				{
					return (uint8_t)((Reg2002_vblank_active << 7) | (Reg2002_objhit << 6) | (Reg2002_objoverflow << 5) | (0x1C));
				}
				if (_isVS2c05 == 4)
				{
					return (uint8_t)((Reg2002_vblank_active << 7) | (Reg2002_objhit << 6) | (Reg2002_objoverflow << 5) | (0x1B));
				}

				return (uint8_t)((Reg2002_vblank_active << 7) | (Reg2002_objhit << 6) | (Reg2002_objoverflow << 5) | (ppu_open_bus & 0x1F));
			}

		//OAM ADDRESS (write)
		void write_2003(int addr, uint8_t value)
		{
			if (_region == NTSC)
			{
				// in NTSC this does several glitchy things to corrupt OAM
				// commented out for now until better understood
				uint8_t temp = (uint8_t)(reg_2003 & 0xF8);
				uint8_t temp_2 = (uint8_t)(addr >> 16 & 0xF8);
				/*
				for (int i=0;i<8;i++)
				{
					glitchy_reads_2003[i] = OAM[temp + i];
					//OAM[temp_2 + i] = glitchy_reads_2003[i];
				}
				*/
				reg_2003 = value;
			}
			else
			{
				// in PAL, just record the oam buffer write target
				reg_2003 = value;
			}
		}

		uint8_t read_2003() { return ppu_open_bus; }
		uint8_t peek_2003() { return ppu_open_bus; }

		//OAM DATA (write)
		void write_2004(uint8_t value)
		{
			if ((reg_2003 & 3) == 2)
			{
				//some of the OAM bits are unwired so we mask them out here
				//otherwise we just write this value and move on to the next oam uint8_t
				value &= 0xE3;
			}
			if (PPUSTATUS_rendering())
			{
				// don't write to OAM if the screen is on and we are in the active display area
				// this impacts sprite evaluation
				if (show_bg_new || show_obj_new)
				{
					// glitchy increment of OAM index
					oam_index += 4;
				}
				else
				{
					OAM[reg_2003] = value;
					reg_2003++;
				}
			}
			else
			{
				OAM[reg_2003] = value;
				reg_2003++;
			}
		}

		uint8_t read_2004()
		{
			uint8_t ret;
			// Console.WriteLine("read 2004");
			// behaviour depends on whether things are being rendered or not
			if (PPUON())
			{
				if (PPUSTATUS_sl < 241)
				{
					if (PPUSTATUS_cycle <= 64)
					{
						ret = 0xFF; // during this time all reads return FF
					}
					else if (PPUSTATUS_cycle <= 256)
					{
						ret = read_value;
					}
					else if (PPUSTATUS_cycle <= 320)
					{
						ret = read_value;
					}
					else
					{
						ret = soam[0];
					}
				}
				else
				{
					ret = OAM[reg_2003];
				}
			}
			else
			{
				ret = OAM[reg_2003];
			}

			ppu_open_bus = ret;
			PpuOpenBusDecay(Decay_All);
			return ret;
		}

		uint8_t peek_2004() { return OAM[reg_2003]; }

		//SCROLL (write)
		void write_2005(uint8_t value)
		{
			if (!vtoggle)
			{
				PPU_Regs_int_ht = value >> 3;
				PPU_Regs_fh = value & 7;
				//LogLine("scroll wrote ht = {0} and fh = {1}", PPU_Regs_int_ht, PPU_Regs_fh);
			}
			else
			{
				PPU_Regs_int_vt = value >> 3;
				PPU_Regs_int_fv = value & 7;
				//LogLine("scroll wrote vt = {0} and fv = {1}", PPU_Regs_int_vt, PPU_Regs_int_fv);
			}
			vtoggle ^= true;
		}

		uint8_t read_2005() { return ppu_open_bus; }
		uint8_t peek_2005() { return ppu_open_bus; }

		//VRAM address register (write)
		void write_2006(uint8_t value)
		{

			if (!vtoggle)
			{
				PPU_Regs_int_vt &= 0x07;
				PPU_Regs_int_vt |= (value & 0x3) << 3;
				PPU_Regs_int_h = (value >> 2) & 1;
				PPU_Regs_int_v = (value >> 3) & 1;
				PPU_Regs_int_fv = (value >> 4) & 3;
				//LogLine("addr wrote fv = {0}", PPU_Regs_int_fv);
				reg_2006_2 = value;
			}
			else
			{
				PPU_Regs_int_vt &= 0x18;
				PPU_Regs_int_vt |= (value >> 5);
				PPU_Regs_int_ht = value & 31;

				// testing indicates that this operation is delayed by 3 pixels
				//PPU_Regs_install_latches();				
				install_2006 = 3;
			}

			vtoggle ^= true;
		}

		uint8_t read_2006() { return ppu_open_bus; }
		uint8_t peek_2006() { return ppu_open_bus; }

		//VRAM data register (r/w)
		void write_2007(uint8_t value)
		{
			//does this take 4x longer? nestopia indicates so perhaps...

			int addr = PPU_Regs_get_2007access();
			if (ppuphase == PPU_PHASE_BG)
			{
				if (show_bg_new)
				{
					addr = PPU_Regs_get_ntread();
				}
			}

			if ((addr & 0x3F00) == 0x3F00)
			{
				//handle palette. this is being done nestopia style, because i found some documentation for it (appendix 1)
				addr &= 0x1F;
				uint8_t color = (uint8_t)(value & 0x3F); //are these bits really unwired? can they be read back somehow?

				//this little hack will help you debug things while the screen is black
				//color = (uint8_t)(addr & 0x3F);

				PALRAM[addr] = color;
				if ((addr & 3) == 0)
				{
					PALRAM[addr ^ 0x10] = color;
				}
			}
			else
			{
				addr &= 0x3FFF;

				ppubus_write(addr, value);

			}

			PPU_Regs_increment2007(PPUSTATUS_rendering() && PPUON(), Reg_2000_vram_incr32 != 0);

			//see comments in $2006
			if (PPUSTATUS_sl >= 241 || !PPUON())
				AddressPPU(PPU_Regs_get_2007access());
		}

		uint8_t read_2007()
		{
			int addr = PPU_Regs_get_2007access() & 0x3FFF;
			int bus_case = 0;
			//ordinarily we return the buffered values
			uint8_t ret = VRAMBuffer;

			//in any case, we read from the ppu bus
			VRAMBuffer = ppubus_read(addr, false, false);

			//but reads from the palette are implemented in the PPU and return immediately
			if ((addr & 0x3F00) == 0x3F00)
			{
				//TODO apply greyscale shit?
				ret = (uint8_t)(PALRAM[addr & 0x1F] + ((uint8_t)(ppu_open_bus & 0xC0)));
				bus_case = 1;
			}

			PPU_Regs_increment2007(PPUSTATUS_rendering() && PPUON(), Reg_2000_vram_incr32 != 0);

			//see comments in $2006
			if (PPUSTATUS_sl >= 241 || !PPUON())
				AddressPPU(PPU_Regs_get_2007access());

			// update open bus here
			ppu_open_bus = ret;
			if (bus_case == 0)
			{
				PpuOpenBusDecay(Decay_All);
			}
			else
			{
				PpuOpenBusDecay(Decay_Low);
			}

			return ret;
		}

		uint8_t peek_2007()
		{
			int addr = PPU_Regs_get_2007access() & 0x3FFF;

			//ordinarily we return the buffered values
			uint8_t ret = VRAMBuffer;

			//in any case, we read from the ppu bus
			// can't do this in peek; updates the value that will be used later
			// VRAMBuffer = ppubus_peek(addr);

			//but reads from the palette are implemented in the PPU and return immediately
			if ((addr & 0x3F00) == 0x3F00)
			{
				//TODO apply greyscale shit?
				ret = PALRAM[addr & 0x1F];
			}

			return ret;
		}

		uint8_t ReadReg(int addr)
		{
			uint8_t ret_spec;
			switch (addr)
			{
			case 0:
			{
				if (_isVS2c05 > 0)
					return read_2001();
				else
					return read_2000();
			}
			case 1:
			{
				if (_isVS2c05 > 0)
					return read_2000();
				else
					return read_2001();
			}
			case 2: return read_2002();
			case 3: return read_2003();
			case 4: return read_2004();
			case 5: return read_2005();
			case 6: return read_2006();
			case 7:
			{
				if (TotalExecutedCycles[0] == double_2007_read)
				{
					return ppu_open_bus;
				}
				else
				{
					ret_spec = read_2007();
					double_2007_read = TotalExecutedCycles[0] + 1;
				}

				if (do_the_reread)
				{
					ret_spec = read_2007();
					ret_spec = read_2007();
					do_the_reread = false;
				}
				return ret_spec;
			}
			default: /*throw new InvalidOperationException();*/ break;
			}
		}

		uint8_t PeekReg(int addr)
		{
			switch (addr)
			{
			case 0: return peek_2000(); case 1: return peek_2001(); case 2: return peek_2002(); case 3: return peek_2003();
			case 4: return peek_2004(); case 5: return peek_2005(); case 6: return peek_2006(); case 7: return peek_2007();
			default: /*throw new InvalidOperationException();*/break;
			}
		}

		void WriteReg(int addr, uint8_t value)
		{
			PPUGenLatch = value;
			ppu_open_bus = value;

			switch (addr & 0x07)
			{
			case 0:
				if (_isVS2c05 > 0)
					write_2001(value);
				else
					write_2000(value);
				break;
			case 1:
				if (_isVS2c05 > 0)
					write_2000(value);
				else
					write_2001(value);
				break;
			case 2: write_2002(value); break;
			case 3: write_2003(addr, value); break;
			case 4: write_2004(value); break;
			case 5: write_2005(value); break;
			case 6: write_2006(value); break;
			case 7: write_2007(value); break;
			default: /*throw new InvalidOperationException();*/ break;
			}
		}

		enum DecayType
		{
			Decay_None = 0, // if there is no action, decrement the timer
			Decay_All = 1, // reset the timer for all bits (reg 2004 / 2007 (non-palette)
			Decay_High = 2, // reset the timer for high 3 bits (reg 2002)
			Decay_Low = 3 // reset the timer for all low 6 bits (reg 2007 (palette))

			// other values of action are reserved for possibly needed expansions, but this passes
			// ppu_open_bus for now.
		};

		void PpuOpenBusDecay(DecayType action)
		{
			switch (action)
			{
			case Decay_None:
				for (int i = 0; i < 8; i++)
				{
					if (ppu_open_bus_decay_timer[i] == 0)
					{
						ppu_open_bus = (uint8_t)(ppu_open_bus & (0xff - (1 << i)));
						ppu_open_bus_decay_timer[i] = 1786840; // about 1 second worth of cycles
					}
					else
					{
						ppu_open_bus_decay_timer[i]--;
					}
				}
				break;
			case Decay_All:
				for (int i = 0; i < 8; i++)
				{
					ppu_open_bus_decay_timer[i] = 1786840;
				}
				break;
			case Decay_High:
				ppu_open_bus_decay_timer[7] = 1786840;
				ppu_open_bus_decay_timer[6] = 1786840;
				ppu_open_bus_decay_timer[5] = 1786840;
				break;
			case Decay_Low:
				for (int i = 0; i < 6; i++)
				{
					ppu_open_bus_decay_timer[i] = 1786840;
				}
				break;
			}
		}

		void pipeline(int pixelcolor, int row_check)
		{
			if (row_check > 0)
			{
				if (Reg_2001_color_disable) { pixelcolor_latch_1 &= 0x30; }
					
				//TODO - check flashing sirens in werewolf
				//tack on the deemph bits. THESE MAY BE ORDERED WRONG. PLEASE CHECK IN THE PALETTE CODE
				xbuf[target - 1] = (uint16_t)(pixelcolor_latch_1 | Reg_2001_intensity_lsl_6);
			}

			pixelcolor_latch_1 = pixelcolor;
		}

		//address line 3 relates to the pattern table fetch occuring (the PPU always makes them in pairs).
		int get_ptread(int par)
		{
			int hi = Reg_2000_bg_pattern_hi;
			return (hi << 0xC) | (par << 0x4) | PPU_Regs_fv;
		}

		void Read_bgdata(int cycle, int i)
		{
			uint8_t at = 0;

			switch (cycle)
			{
			case 0:
				ppu_addr_temp = PPU_Regs_get_ntread();
				bgdata[i].nt = ppubus_read(ppu_addr_temp, true, true);
				break;
			case 1:
				break;
			case 2:
				ppu_addr_temp = PPU_Regs_get_atread();
				at = ppubus_read(ppu_addr_temp, true, true);

				//modify at to get appropriate palette shift
				if ((PPU_Regs_vt & 2) != 0) at >>= 4;
				if ((PPU_Regs_ht & 2) != 0) at >>= 2;
				at &= 0x03;
				at <<= 2;
				bgdata[i].at = at;
				break;
			case 3:
				break;
			case 4:
				ppu_addr_temp = get_ptread(bgdata[i].nt);
				bgdata[i].pt_0 = ppubus_read(ppu_addr_temp, true, true);
				break;
			case 5:
				break;
			case 6:
				ppu_addr_temp |= 8;
				bgdata[i].pt_1 = ppubus_read(ppu_addr_temp, true, true);
				break;
			case 7:
				break;
			}
		}

		void ppu_init_frame()
		{
			PPUSTATUS_sl = 241 + preNMIlines;
			PPUSTATUS_cycle = 0;

			// These things happen at the start of every frame
			ppuphase = PPU_PHASE_VBL;

			for (int i = 0; i < 34; i++) 
			{
				bgdata[i].at = 0;
				bgdata[i].nt = 0;
				bgdata[i].pt_0 = 0;
				bgdata[i].pt_1 = 0;
			}
		}

		void TickPPU_VBL()
		{
			if (PPUSTATUS_cycle == 0 && PPUSTATUS_sl == 241 + preNMIlines)
			{
				nmi_destiny = Reg_2000_vblank_nmi_gen && Reg2002_vblank_active;
				if (cpu_stepcounter == 2) { NMI_offset = 1; }
				else if (cpu_stepcounter == 1) { NMI_offset = 2; }
				else { NMI_offset = 0; }
			}
			else if (PPUSTATUS_cycle <= 2 && nmi_destiny)
			{
				nmi_destiny &= Reg_2000_vblank_nmi_gen && Reg2002_vblank_active;
			}
			else if (PPUSTATUS_cycle == (3 + NMI_offset) && PPUSTATUS_sl == 241 + preNMIlines)
			{
				if (nmi_destiny) { NMI[0] = true; }
				AtVsyncNMI();
			}

			if (PPUSTATUS_cycle == 340)
			{
				if (PPUSTATUS_sl == 241 + preNMIlines + postNMIlines - 1)
				{
					Reg2002_vblank_clear_pending = true;
					idleSynch ^= true;
					Reg2002_objhit = Reg2002_objoverflow = 0;
				}
			}

			runppu(); // note cycle ticks inside runppu

			if (PPUSTATUS_cycle == 341)
			{
				if (Reg2002_vblank_clear_pending)
				{
					Reg2002_vblank_active = 0;
					Reg2002_vblank_clear_pending = false;
				}

				PPUSTATUS_cycle = 0;
				PPUSTATUS_sl++;
				if (PPUSTATUS_sl == 241 + preNMIlines + postNMIlines)
				{
					do_vbl = false;
					PPUSTATUS_sl = 0;
					do_active_sl = true;
				}
			}
		}

		void TickPPU_active()
		{
			if (PPUSTATUS_cycle < 256)
			{
				if (PPUSTATUS_cycle == 0)
				{
					PPUSTATUS_cycle = 0;

					spr_true_count = 0;
					soam_index = 0;
					soam_m_index = 0;
					oam_index_aux = 0;
					oam_index = 0;
					is_even_cycle = true;
					sprite_eval_write = true;
					sprite_zero_go = sprite_zero_in_range;

					sprite_zero_in_range = false;

					yp = PPUSTATUS_sl - 1;
					ppuphase = PPU_PHASE_BG;

					// "If PPUADDR is not less then 8 when rendering starts, the first 8 uint8_ts in OAM are written to from 
					// the current location of PPUADDR"			
					if (PPUSTATUS_sl == 0 && PPUON() && reg_2003 >= 8 && _region == NTSC)
					{
						for (int i = 0; i < 8; i++)
						{
							OAM[i] = OAM[(reg_2003 & 0xF8) + i];
						}
					}

					if (NTViewCallback && yp == NTViewCallback_Scanline) NTViewCallback();
					if (PPUViewCallback && yp == PPUViewCallback_Scanline) PPUViewCallback();

					// set up intial values to use later
					yp_shift = yp << 8;
					xt = 0;
					xp = 0;

					sprite_eval_cycle = 0;

					xstart = xt << 3;
					target = yp_shift + xstart;
					rasterpos = xstart;

					spriteHeight = Reg_2000_obj_size_16 ? 16 : 8;

					//check all the conditions that can cause things to render in these 8px
					renderspritenow = show_obj_new && (xt > 0 || Reg_2001_show_obj_leftmost);
				}

				if (PPUSTATUS_sl != 0)
				{
					/////////////////////////////////////////////
					// Sprite Evaluation Start
					/////////////////////////////////////////////

					if (sprite_eval_cycle < 64)
					{
						// the first 64 cycles of each scanline are used to initialize sceondary OAM 
						// the actual effect setting a flag that always returns 0xFF from a OAM read
						// this is a bit of a shortcut to save some instructions
						// data is read from OAM as normal but never used
						if (!is_even_cycle)
						{
							soam[soam_index] = 0xFF;
							soam_index++;
						}
					}
					// otherwise, scan through OAM and test if sprites are in range
					// if they are, they get copied to the secondary OAM 
					else
					{
						if (sprite_eval_cycle == 64)
						{
							soam_index = 0;
							oam_index = reg_2003;
						}

						if (oam_index >= 256)
						{
							oam_index = 0;
							sprite_eval_write = false;
						}

						if (is_even_cycle)
						{
							if ((oam_index + soam_m_index) < 256)
								read_value = OAM[oam_index + soam_m_index];
							else
								read_value = OAM[oam_index + soam_m_index - 256];
						}
						else if (sprite_eval_write)
						{
							//look for sprites 
							if (spr_true_count == 0 && soam_index < 8)
							{
								soam[soam_index * 4] = read_value;
							}

							if (soam_index < 8)
							{
								if (yp >= read_value && yp < read_value + spriteHeight && spr_true_count == 0)
								{
									//a flag gets set if sprite zero is in range
									if (oam_index == reg_2003) { sprite_zero_in_range = true; }

									spr_true_count++;
									soam_m_index++;
								}
								else if (spr_true_count > 0 && spr_true_count < 4)
								{
									soam[soam_index * 4 + soam_m_index] = read_value;

									soam_m_index++;

									spr_true_count++;
									if (spr_true_count == 4)
									{
										oam_index += 4;
										soam_index++;
										if (soam_index == 8)
										{
											// oam_index could be pathologically misaligned at this point, so we have to find the next 
											// nearest actual sprite to work on >8 sprites per scanline option
											oam_index_aux = (oam_index % 4) * 4;
										}

										soam_m_index = 0;
										spr_true_count = 0;
									}
								}
								else
								{
									oam_index += 4;
								}
							}
							else
							{
								if (yp >= read_value && yp < read_value + spriteHeight && PPUON())
								{
									Reg2002_objoverflow = true;
								}

								if (yp >= read_value && yp < read_value + spriteHeight && spr_true_count == 0)
								{
									spr_true_count++;
									soam_m_index++;
								}
								else if (spr_true_count > 0 && spr_true_count < 4)
								{
									soam_m_index++;

									spr_true_count++;
									if (spr_true_count == 4)
									{
										oam_index += 4;
										soam_index++;
										soam_m_index = 0;
										spr_true_count = 0;
									}
								}
								else
								{
									oam_index += 4;
									if (soam_index == 8)
									{
										soam_m_index++; // glitchy increment
										soam_m_index &= 3;
									}

								}

								read_value = soam[0]; //writes change to reads 
							}
						}
						else
						{
							// if we don't write sprites anymore, just scan through the oam
							read_value = soam[0];
							oam_index += 4;
						}
					}

					/////////////////////////////////////////////
					// Sprite Evaluation End
					/////////////////////////////////////////////

					int pixel = 0, pixelcolor = PALRAM[pixel];

					//process the current clock's worth of bg data fetching
					//this needs to be split into 8 pieces or else exact sprite 0 hitting wont work 
					// due to the cpu not running while the sprite renders below
					if (PPUON()) { Read_bgdata(xp, xt + 2); }
					//according to qeed's doc, use palette 0 or $2006's value if it is & 0x3Fxx
					//at one point I commented this out to fix bottom-left garbage in DW4. but it's needed for full_nes_palette. 
					//solution is to only run when PPU is actually OFF (left-suppression doesnt count)
					else
					{
						// if there's anything wrong with how we're doing this, someone please chime in
						int addr = PPU_Regs_get_2007access();
						if ((addr & 0x3F00) == 0x3F00)
						{
							pixel = addr & 0x1F;
						}
						pixelcolor = PALRAM[pixel];
						pixelcolor |= 0x8000;
					}

					//generate the BG data
					if (show_bg_new && (xt > 0 || Reg_2001_show_bg_leftmost))
					{
						int bgtile = (rasterpos + PPU_Regs_fh) >> 3;
						uint8_t pt_0 = bgdata[bgtile].pt_0;
						uint8_t pt_1 = bgdata[bgtile].pt_1;
						int sel = 7 - (rasterpos + PPU_Regs_fh) & 7;
						pixel = ((pt_0 >> sel) & 1) | (((pt_1 >> sel) & 1) << 1);
						if (pixel != 0)
							pixel |= bgdata[bgtile].at;
						pixelcolor = PALRAM[pixel];
					}

					if (!Settings_DispBackground)
						pixelcolor = 0x8000;

					//check if the pixel has a sprite in it
					if (sl_sprites[256 + xt * 8 + xp] != 0 && renderspritenow)
					{
						int s = sl_sprites[xt * 8 + xp];
						int spixel = sl_sprites[256 + xt * 8 + xp];
						int temp_attr = sl_sprites[512 + xt * 8 + xp];

						//TODO - make sure we don't trigger spritehit if the edges are masked for either BG or OBJ
						//spritehit:
						//1. is it sprite#0?
						//2. is the bg pixel nonzero?
						//then, it is spritehit.
						Reg2002_objhit |= (sprite_zero_go && s == 0 && pixel != 0 && rasterpos < 255 && show_bg_new && show_obj_new);
						//priority handling, if in front of BG:
						bool drawsprite = !(((temp_attr & 0x20) != 0) && ((pixel & 3) != 0));
						if (drawsprite && Settings_DispSprites)
						{
							//bring in the palette bits and palettize
							spixel |= (temp_attr & 3) << 2;
							//save it for use in the framebuffer
							pixelcolor = PALRAM[0x10 + spixel];
						}
					} //oamcount loop

					runppu();

					if (xp == 6 && PPUON())
					{
						ppu_was_on = true;
						if (PPUSTATUS_cycle == 255) { race_2006 = true; }
					}

					if (xp == 7 && PPUON())
					{
						PPU_Regs_increment_hsc();

						if ((PPUSTATUS_cycle == 256) && ppu_was_on)
						{
							PPU_Regs_increment_vs();
						}

						if (race_2006_2)
						{
							if (PPUSTATUS_cycle == 256)
							{
								PPU_Regs_fv &= PPU_Regs_int_fv;
								PPU_Regs_v &= PPU_Regs_int_v;
								PPU_Regs_h &= PPU_Regs_int_h;
								PPU_Regs_vt &= PPU_Regs_int_vt;
								PPU_Regs_ht &= PPU_Regs_int_ht;
							}
							else
							{
								PPU_Regs_fv = PPU_Regs_int_fv;
								PPU_Regs_v = PPU_Regs_int_v;
								PPU_Regs_h &= PPU_Regs_int_h;
								PPU_Regs_vt = PPU_Regs_int_vt;
								PPU_Regs_ht &= PPU_Regs_int_ht;
							}
						}

						ppu_was_on = false;
					}

					race_2006_2 = false;

					pipeline(pixelcolor, xt * 8 + xp);
					target++;

					// clear out previous sprites from scanline buffer
					sl_sprites[256 + xt * 8 + xp] = 0;

					// end of visible part of the scanline
					sprite_eval_cycle++;
					xp++;
					rasterpos++;

					if (xp == 8)
					{
						xp = 0;
						xt++;

						xstart = xt << 3;
						target = yp_shift + xstart;
						rasterpos = xstart;

						spriteHeight = Reg_2000_obj_size_16 ? 16 : 8;

						//check all the conditions that can cause things to render in these 8px
						renderspritenow = show_obj_new && (xt > 0 || Reg_2001_show_obj_leftmost);
					}
				}
				else
				{
					// if scanline is the pre-render line, we just read BG data
					Read_bgdata(xp, xt + 2);

					runppu();

					if (xp == 6 && PPUON())
					{
						ppu_was_on = true;
						if (PPUSTATUS_cycle == 255) { race_2006 = true; }

					}

					if (xp == 7 && PPUON())
					{
						PPU_Regs_increment_hsc();

						if ((PPUSTATUS_cycle == 256) && ppu_was_on)
						{
							PPU_Regs_increment_vs();
						}

						if (race_2006_2)
						{
							if (PPUSTATUS_cycle == 256)
							{
								PPU_Regs_fv &= PPU_Regs_int_fv;
								PPU_Regs_v &= PPU_Regs_int_v;
								PPU_Regs_h &= PPU_Regs_int_h;
								PPU_Regs_vt &= PPU_Regs_int_vt;
								PPU_Regs_ht &= PPU_Regs_int_ht;
							}
							else
							{
								PPU_Regs_fv = PPU_Regs_int_fv;
								PPU_Regs_v = PPU_Regs_int_v;
								PPU_Regs_h &= PPU_Regs_int_h;
								PPU_Regs_vt = PPU_Regs_int_vt;
								PPU_Regs_ht &= PPU_Regs_int_ht;
							}
						}

						ppu_was_on = false;
					}

					race_2006_2 = false;

					xp++;

					if (xp == 8)
					{
						xp = 0;
						xt++;
					}
				}
			}
			else if (PPUSTATUS_cycle < 320)
			{
				// after we are done with the visible part of the frame, we reach sprite transfer to temp OAM tables and such
				if (PPUSTATUS_cycle == 256)
				{
					// do the more then 8 sprites stuff here where it is convenient
					// normally only 8 sprites are allowed, but with a particular setting we can have more then that
					// this extra bit takes care of it quickly
					soam_index_aux = 8;

					if (Settings_AllowMoreThanEightSprites)
					{
						while (oam_index_aux < 64 && soam_index_aux < 64)
						{
							//look for sprites 
							soam[soam_index_aux * 4] = OAM[oam_index_aux * 4];
							if (yp >= OAM[oam_index_aux * 4] && yp < OAM[oam_index_aux * 4] + spriteHeight)
							{
								soam[soam_index_aux * 4 + 1] = OAM[oam_index_aux * 4 + 1];
								soam[soam_index_aux * 4 + 2] = OAM[oam_index_aux * 4 + 2];
								soam[soam_index_aux * 4 + 3] = OAM[oam_index_aux * 4 + 3];
								soam_index_aux++;
								oam_index_aux++;
							}
							else
							{
								oam_index_aux++;
							}
						}
					}

					soam_index_prev = soam_index_aux;

					if (soam_index_prev > 8 && !Settings_AllowMoreThanEightSprites)
						soam_index_prev = 8;

					ppuphase = PPU_PHASE_OBJ;

					spriteHeight = Reg_2000_obj_size_16 ? 16 : 8;

					s = 0;
					ppu_aux_index = 0;

					junksprite = !PPUON();

					t_oam[s].oam_y = soam[s * 4];
					t_oam[s].oam_ind = soam[s * 4 + 1];
					t_oam[s].oam_attr = soam[s * 4 + 2];
					t_oam[s].oam_x = soam[s * 4 + 3];

					line = yp - t_oam[s].oam_y;
					if ((t_oam[s].oam_attr & 0x80) != 0) //vflip
						line = spriteHeight - line - 1;

					patternNumber = t_oam[s].oam_ind;
				}

				switch (ppu_aux_index)
				{
				case 0:
					//8x16 sprite handling:
					if (Reg_2000_obj_size_16)
					{
						int bank = (patternNumber & 1) << 12;
						patternNumber = patternNumber & ~1;
						patternNumber |= (line >> 3) & 1;
						patternAddress = (patternNumber << 4) | bank;
					}
					else
						patternAddress = (patternNumber << 4) | (Reg_2000_obj_pattern_hi << 12);

					//offset into the pattern for the current line.
					//tricky: tall sprites have already had lines>8 taken care of by getting a new pattern number above.
					//so we just need the line offset for the second pattern
					patternAddress += line & 7;

					ppubus_read(PPU_Regs_get_ntread(), true, true);

					read_value = t_oam[s].oam_y;
					runppu();
					break;
				case 1:
					if (PPUSTATUS_sl == 0 && PPUSTATUS_cycle == 305 && PPUON())
					{
						PPU_Regs_install_latches();

						read_value = t_oam[s].oam_ind;
						runppu();

					}
					else if ((PPUSTATUS_sl != 0) && PPUSTATUS_cycle == 257 && PPUON())
					{

						if (target <= 61441 && target > 0 && s == 0)
						{
							pipeline(0, 256);   //  last pipeline call option 1 of 2
							target++;
						}

						//at 257: 3d world runner is ugly if we do this at 256
						if (PPUON()/* && !race_2006_2*/) { PPU_Regs_install_h_latches(); }
						race_2006_2 = false;
						read_value = t_oam[s].oam_ind;
						runppu();

						/*
						if (target <= 61441 && target > 0 && s == 0)
						{
							//pipeline(0, 257);
						}
						*/
					}
					else
					{
						if (target <= 61441 && target > 0 && s == 0)
						{
							pipeline(0, 256);  //  last pipeline call option 2 of 2
							target++;
						}

						read_value = t_oam[s].oam_ind;
						runppu();

						/*
						if (target <= 61441 && target > 0 && s == 0)
						{
							//pipeline(0, 257);
						}
						*/
					}
					break;

				case 2:
					ppubus_read(PPU_Regs_get_atread(), true, true); //at or nt?
					read_value = t_oam[s].oam_attr;
					runppu();
					break;

				case 3:
					read_value = t_oam[s].oam_x;
					runppu();
					break;

				case 4:
					// if the PPU is off, we don't put anything on the bus
					if (junksprite)
					{
						ppubus_read(patternAddress, true, false);
						runppu();
					}
					else
					{
						temp_addr = patternAddress;
						t_oam[s].patterns_0 = ppubus_read(temp_addr, true, true);
						read_value = t_oam[s].oam_x;
						runppu();
					}
					break;
				case 5:
					runppu();
					break;
				case 6:
					// if the PPU is off, we don't put anything on the bus
					if (junksprite)
					{
						ppubus_read(patternAddress, true, false);
						runppu();
					}
					else
					{
						temp_addr += 8;
						t_oam[s].patterns_1 = ppubus_read(temp_addr, true, true);
						read_value = t_oam[s].oam_x;
						runppu();
					}
					break;
				case 7:
					// if the PPU is off, we don't put anything on the bus
					if (junksprite)
					{
						runppu();
					}
					else
					{
						runppu();

						// hflip
						if ((t_oam[s].oam_attr & 0x40) == 0)
						{
							t_oam[s].patterns_0 = BitReverse(t_oam[s].patterns_0);
							t_oam[s].patterns_1 = BitReverse(t_oam[s].patterns_1);
						}

						// if the sprites attribute is 0xFF, then this indicates a non-existent sprite
						// I think the logic here is that bits 2-4 in OAM are disabled, but soam is initialized with 0xFF
						// so the only way a sprite could have an 0xFF attribute is if it is not in the scope of the scanline
						if (t_oam[s].oam_attr == 0xFF)
						{
							t_oam[s].patterns_0 = 0;
							t_oam[s].patterns_1 = 0;
						}

					}
					break;
				}

				ppu_aux_index++;
				if (ppu_aux_index == 8)
				{
					// now that we have a sprite, we can fill in the next scnaline's sprite pixels with it
					// this saves quite a bit of processing compared to checking each pixel

					if (s < soam_index_prev && (PPUSTATUS_sl != 0) && (PPUSTATUS_sl != 240))
					{
						int temp_x = t_oam[s].oam_x;
						for (int i = 0; (temp_x + i) < 256 && i < 8; i++)
						{
							if (sl_sprites[256 + temp_x + i] == 0)
							{
								if ((t_oam[s].patterns_0 & (1 << i)) || (t_oam[s].patterns_1 & (1 << i)))
								{
									int spixel = (t_oam[s].patterns_0 & (1 << i)) ? 1 : 0;
									spixel |= (t_oam[s].patterns_1 & (1 << i)) ? 2 : 0;

									sl_sprites[temp_x + i] = (uint8_t)s;
									sl_sprites[256 + temp_x + i] = (uint8_t)spixel;
									sl_sprites[512 + temp_x + i] = t_oam[s].oam_attr;
								}
							}
						}
					}

					ppu_aux_index = 0;
					s++;

					if (s < 8)
					{
						junksprite = !PPUON();

						t_oam[s].oam_y = soam[s * 4];
						t_oam[s].oam_ind = soam[s * 4 + 1];
						t_oam[s].oam_attr = soam[s * 4 + 2];
						t_oam[s].oam_x = soam[s * 4 + 3];

						line = yp - t_oam[s].oam_y;
						if ((t_oam[s].oam_attr & 0x80) != 0) //vflip
							line = spriteHeight - line - 1;

						patternNumber = t_oam[s].oam_ind;
					}
					else
					{
						// repeat all the above steps for more then 8 sprites but don't run any cycles
						if (soam_index_aux > 8)
						{
							for (int s = 8; s < soam_index_aux; s++)
							{
								t_oam[s].oam_y = soam[s * 4];
								t_oam[s].oam_ind = soam[s * 4 + 1];
								t_oam[s].oam_attr = soam[s * 4 + 2];
								t_oam[s].oam_x = soam[s * 4 + 3];

								int line = yp - t_oam[s].oam_y;
								if ((t_oam[s].oam_attr & 0x80) != 0) //vflip
									line = spriteHeight - line - 1;

								int patternNumber = t_oam[s].oam_ind;
								int patternAddress;

								//8x16 sprite handling:
								if (Reg_2000_obj_size_16)
								{
									int bank = (patternNumber & 1) << 12;
									patternNumber = patternNumber & ~1;
									patternNumber |= (line >> 3) & 1;
									patternAddress = (patternNumber << 4) | bank;
								}
								else
									patternAddress = (patternNumber << 4) | (Reg_2000_obj_pattern_hi << 12);

								//offset into the pattern for the current line.
								//tricky: tall sprites have already had lines>8 taken care of by getting a new pattern number above.
								//so we just need the line offset for the second pattern
								patternAddress += line & 7;

								ppubus_read(PPU_Regs_get_ntread(), true, false);

								ppubus_read(PPU_Regs_get_atread(), true, false); //at or nt?

								int addr = patternAddress;
								t_oam[s].patterns_0 = ppubus_read(addr, true, false);

								addr += 8;
								t_oam[s].patterns_1 = ppubus_read(addr, true, false);

								// hflip
								if ((t_oam[s].oam_attr & 0x40) == 0)
								{
									t_oam[s].patterns_0 = BitReverse(t_oam[s].patterns_0);
									t_oam[s].patterns_1 = BitReverse(t_oam[s].patterns_1);
								}

								// if the sprites attribute is 0xFF, then this indicates a non-existent sprite
								// I think the logic here is that bits 2-4 in OAM are disabled, but soam is initialized with 0xFF
								// so the only way a sprite could have an 0xFF attribute is if it is not in the scope of the scanline
								if (t_oam[s].oam_attr == 0xFF)
								{
									t_oam[s].patterns_0 = 0;
									t_oam[s].patterns_1 = 0;
								}

								int temp_x = t_oam[s].oam_x;
								if ((PPUSTATUS_sl != 0) && (PPUSTATUS_sl != 240))
								{
									for (int i = 0; (temp_x + i) < 256 && i < 8; i++)
									{
										if (sl_sprites[256 + temp_x + i] == 0)
										{
											if ((t_oam[s].patterns_0 & (1 << i)) || (t_oam[s].patterns_1 & (1 << i)))
											{
												int spixel = (t_oam[s].patterns_0 & (1 << i)) ? 1 : 0;
												spixel |= (t_oam[s].patterns_1 & (1 << i)) ? 2 : 0;

												sl_sprites[temp_x + i] = (uint8_t)s;
												sl_sprites[256 + temp_x + i] = (uint8_t)spixel;
												sl_sprites[512 + temp_x + i] = t_oam[s].oam_attr;
											}
										}
									}
								}
							}
						}
					}
				}
			}
			else
			{
				if (PPUSTATUS_cycle < 336)
				{
					if (PPUSTATUS_cycle == 320)
					{
						ppuphase = PPU_PHASE_BG;
						xt = 0;
						xp = 0;
					}

					// if scanline is the pre-render line, we just read BG data
					Read_bgdata(xp, xt);

					runppu();

					if (xp == 6 && PPUON())
					{
						ppu_was_on = true;
					}

					if (xp == 7 && PPUON())
					{
						if (!race_2006)
							PPU_Regs_increment_hsc();

						if (PPUSTATUS_cycle == 256 && !race_2006)
							PPU_Regs_increment_vs();

						ppu_was_on = false;
					}

					xp++;

					if (xp == 8)
					{
						xp = 0;
						xt++;
					}
				}
				else if (PPUSTATUS_cycle < 340)
				{
					if (PPUSTATUS_cycle == 339)
					{
						evenOddDestiny = PPUON();
					}

					runppu();
				}
				else
				{
					// After memory access 170, the PPU simply rests for 4 cycles (or the
					// equivelant of half a memory access cycle) before repeating the whole
					// pixel/scanline rendering process. If the scanline being rendered is the very
					// first one on every second frame, then this delay simply doesn't exist.
					if (PPUSTATUS_sl == 0 && idleSynch && evenOddDestiny && chopdot)
					{
						PPUSTATUS_cycle++;
					} // increment cycle without running ppu
					else
					{
						runppu();
					}

					PPUSTATUS_cycle = 0;
					PPUSTATUS_sl++;

					if (PPUSTATUS_sl == 241)
					{
						do_active_sl = false;
						do_pre_vbl = true;
					}
				}
			}
		}

		void TickPPU_preVBL()
		{
			if ((PPUSTATUS_cycle == 340) && (PPUSTATUS_sl == 241 + preNMIlines - 1))
			{
				Reg2002_vblank_active_pending = true;
			}

			runppu();

			if (PPUSTATUS_cycle == 341)
			{
				PPUSTATUS_cycle = 0;
				PPUSTATUS_sl++;

				if (PPUSTATUS_sl == 241 + preNMIlines)
				{
					if (Reg2002_vblank_active_pending)
					{
						Reg2002_vblank_active = 1;
						Reg2002_vblank_active_pending = false;
					}

					do_pre_vbl = false;
					do_vbl = true;

					ppu_init_frame();
					frame_is_done = true;
				}
			}
		}

		//not quite emulating all the NES power up behavior
		//since it is known that the NES ignores writes to some
		//register before around a full frame, but no games
		//should write to those regs during that time, it needs
		//to wait for vblank
		void NewDeadPPU()
		{
			if (PPUSTATUS_cycle == 241 * 341 - start_up_offset - 1)
			{
				Reg2002_vblank_active_pending = true;
			}

			runppu();

			if (PPUSTATUS_cycle == 241 * 341 - start_up_offset)
			{
				if (Reg2002_vblank_active_pending)
				{
					Reg2002_vblank_active = 1;
					Reg2002_vblank_active_pending = false;
				}

				ppudead--;

				ppu_init_frame();

				do_vbl = true;

				frame_is_done = true;
			}
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

			saver = bool_saver(Settings_DispBackground, saver);
			saver = bool_saver(Settings_DispSprites, saver);
			saver = bool_saver(Settings_AllowMoreThanEightSprites, saver);

			saver = bool_saver(chopdot, saver);
			saver = bool_saver(vtoggle, saver);
			saver = bool_saver(idleSynch, saver);
			saver = bool_saver(HasClockPPU, saver);

			saver = bool_saver(Reg_2000_vram_incr32, saver);
			saver = bool_saver(Reg_2000_obj_pattern_hi, saver);
			saver = bool_saver(Reg_2000_bg_pattern_hi, saver);
			saver = bool_saver(Reg_2000_obj_size_16, saver);
			saver = bool_saver(Reg_2000_ppu_layer, saver);
			saver = bool_saver(Reg_2000_vblank_nmi_gen, saver);

			saver = bool_saver(Reg_2001_color_disable, saver);
			saver = bool_saver(Reg_2001_show_bg_leftmost, saver);
			saver = bool_saver(Reg_2001_show_obj_leftmost, saver);
			saver = bool_saver(Reg_2001_show_bg, saver);
			saver = bool_saver(Reg_2001_show_obj, saver);
			saver = bool_saver(Reg_2001_intense_green, saver);
			saver = bool_saver(Reg_2001_intense_blue, saver);
			saver = bool_saver(Reg_2001_intense_red, saver);

			saver = bool_saver(Reg2002_objoverflow, saver);
			saver = bool_saver(Reg2002_objhit, saver);
			saver = bool_saver(Reg2002_vblank_active, saver);
			saver = bool_saver(Reg2002_vblank_active_pending, saver);
			saver = bool_saver(Reg2002_vblank_clear_pending, saver);

			saver = bool_saver(show_bg_new, saver);
			saver = bool_saver(show_obj_new, saver);
			saver = bool_saver(race_2006, saver);
			saver = bool_saver(race_2006_2, saver);
			saver = bool_saver(sprite_eval_write, saver);
			saver = bool_saver(is_even_cycle, saver);
			saver = bool_saver(sprite_zero_in_range, saver);
			saver = bool_saver(sprite_zero_go, saver);
			saver = bool_saver(ppu_was_on, saver);
			saver = bool_saver(do_vbl, saver);
			saver = bool_saver(do_active_sl, saver);
			saver = bool_saver(do_pre_vbl, saver);
			saver = bool_saver(nmi_destiny, saver);
			saver = bool_saver(evenOddDestiny, saver);
			saver = bool_saver(renderspritenow, saver);
			saver = bool_saver(junksprite, saver);

			saver = byte_saver(_isVS2c05, saver);
			saver = byte_saver(VRAMBuffer, saver);
			saver = byte_saver(PPUGenLatch, saver);
			saver = byte_saver(reg_2003, saver);
			saver = byte_saver(reg_2006_2, saver);
			saver = byte_saver(read_value, saver);
			saver = byte_saver(ppu_open_bus, saver);

			saver = int_saver(cpu_step, saver);
			saver = int_saver(cpu_stepcounter, saver);
			saver = int_saver(ppudead, saver);
			saver = int_saver(NMI_PendingInstructions, saver);
			saver = int_saver(preNMIlines, saver);
			saver = int_saver(postNMIlines, saver);
			saver = int_saver(ppuphase, saver);
			saver = int_saver(Reg_2001_intensity_lsl_6, saver);
			saver = int_saver(PPUSTATUS_sl, saver);
			saver = int_saver(PPUSTATUS_cycle, saver);
			saver = int_saver(PPU_Regs_fv, saver);
			saver = int_saver(PPU_Regs_v, saver);
			saver = int_saver(PPU_Regs_h, saver);
			saver = int_saver(PPU_Regs_vt, saver);
			saver = int_saver(PPU_Regs_ht, saver);
			saver = int_saver(PPU_Regs_int_fv, saver);
			saver = int_saver(PPU_Regs_int_vt, saver);
			saver = int_saver(PPU_Regs_int_v, saver);
			saver = int_saver(PPU_Regs_int_h, saver);
			saver = int_saver(PPU_Regs_int_ht, saver);
			saver = int_saver(PPU_Regs_fh, saver);
			saver = int_saver(ppu_addr_temp, saver);
			saver = int_saver(pixelcolor_latch_1, saver);
			saver = int_saver(soam_index, saver);
			saver = int_saver(soam_index_prev, saver);
			saver = int_saver(soam_m_index, saver);
			saver = int_saver(oam_index, saver);
			saver = int_saver(oam_index_aux, saver);
			saver = int_saver(soam_index_aux, saver);
			saver = int_saver(spr_true_count, saver);
			saver = int_saver(yp, saver);
			saver = int_saver(target, saver);
			saver = int_saver(spriteHeight, saver);
			saver = int_saver(install_2006, saver);
			saver = int_saver(install_2001, saver);
			saver = int_saver(NMI_offset, saver);
			saver = int_saver(yp_shift, saver);
			saver = int_saver(sprite_eval_cycle, saver);
			saver = int_saver(xt, saver);
			saver = int_saver(xp, saver);
			saver = int_saver(xstart, saver);
			saver = int_saver(rasterpos, saver);
			saver = int_saver(s, saver);
			saver = int_saver(ppu_aux_index, saver);
			saver = int_saver(line, saver);
			saver = int_saver(patternNumber, saver);
			saver = int_saver(patternAddress, saver);
			saver = int_saver(temp_addr, saver);

			*saver = (uint8_t)(_totalCycles & 0xFF); saver++; *saver = (uint8_t)((_totalCycles >> 8) & 0xFF); saver++;
			*saver = (uint8_t)((_totalCycles >> 16) & 0xFF); saver++; *saver = (uint8_t)((_totalCycles >> 24) & 0xFF); saver++;
			*saver = (uint8_t)((_totalCycles >> 32) & 0xFF); saver++; *saver = (uint8_t)((_totalCycles >> 40) & 0xFF); saver++;
			*saver = (uint8_t)((_totalCycles >> 48) & 0xFF); saver++; *saver = (uint8_t)((_totalCycles >> 56) & 0xFF); saver++;

			*saver = (uint8_t)(double_2007_read & 0xFF); saver++; *saver = (uint8_t)((double_2007_read >> 8) & 0xFF); saver++;
			*saver = (uint8_t)((double_2007_read >> 16) & 0xFF); saver++; *saver = (uint8_t)((double_2007_read >> 24) & 0xFF); saver++;
			*saver = (uint8_t)((double_2007_read >> 32) & 0xFF); saver++; *saver = (uint8_t)((double_2007_read >> 40) & 0xFF); saver++;
			*saver = (uint8_t)((double_2007_read >> 48) & 0xFF); saver++; *saver = (uint8_t)((double_2007_read >> 56) & 0xFF); saver++;

			saver = byte_array_saver(CIRAM, saver, 0x800);
			saver = byte_array_saver(glitchy_reads_2003, saver, 8);
			saver = byte_array_saver(PALRAM, saver, 0x20);
			saver = byte_array_saver(OAM, saver, 0x100);
			saver = byte_array_saver(soam, saver, 256);
			saver = byte_array_saver(sl_sprites, saver, 3 * 256);

			saver = int_array_saver(ppu_open_bus_decay_timer, saver, 8);
			saver = int_array_saver(_currentLuma, saver, 64);

			for (int i = 0; i < 34; i++) 
			{
				saver = byte_saver(bgdata[i].nt, saver);
				saver = byte_saver(bgdata[i].at, saver);
				saver = byte_saver(bgdata[i].pt_0, saver);
				saver = byte_saver(bgdata[i].pt_1, saver);
			}

			for (int i = 0; i < 64; i++)
			{
				saver = byte_saver(t_oam[i].oam_y, saver);
				saver = byte_saver(t_oam[i].oam_ind, saver);
				saver = byte_saver(t_oam[i].oam_attr, saver);
				saver = byte_saver(t_oam[i].oam_x, saver);
				saver = byte_saver(t_oam[i].patterns_0, saver);
				saver = byte_saver(t_oam[i].patterns_1, saver);
			}

			return saver;
		}

		uint8_t* LoadState(uint8_t* loader)
		{
			loader = bool_loader(&frame_is_done, loader);
			loader = bool_loader(&do_the_reread, loader);

			loader = bool_loader(&Settings_DispBackground, loader);
			loader = bool_loader(&Settings_DispSprites, loader);
			loader = bool_loader(&Settings_AllowMoreThanEightSprites, loader);

			loader = bool_loader(&chopdot, loader);
			loader = bool_loader(&vtoggle, loader);
			loader = bool_loader(&idleSynch, loader);
			loader = bool_loader(&HasClockPPU, loader);

			loader = bool_loader(&Reg_2000_vram_incr32, loader);
			loader = bool_loader(&Reg_2000_obj_pattern_hi, loader);
			loader = bool_loader(&Reg_2000_bg_pattern_hi, loader);
			loader = bool_loader(&Reg_2000_obj_size_16, loader);
			loader = bool_loader(&Reg_2000_ppu_layer, loader);
			loader = bool_loader(&Reg_2000_vblank_nmi_gen, loader);

			loader = bool_loader(&Reg_2001_color_disable, loader);
			loader = bool_loader(&Reg_2001_show_bg_leftmost, loader);
			loader = bool_loader(&Reg_2001_show_obj_leftmost, loader);
			loader = bool_loader(&Reg_2001_show_bg, loader);
			loader = bool_loader(&Reg_2001_show_obj, loader);
			loader = bool_loader(&Reg_2001_intense_green, loader);
			loader = bool_loader(&Reg_2001_intense_blue, loader);
			loader = bool_loader(&Reg_2001_intense_red, loader);

			loader = bool_loader(&Reg2002_objoverflow, loader);
			loader = bool_loader(&Reg2002_objhit, loader);
			loader = bool_loader(&Reg2002_vblank_active, loader);
			loader = bool_loader(&Reg2002_vblank_active_pending, loader);
			loader = bool_loader(&Reg2002_vblank_clear_pending, loader);

			loader = bool_loader(&show_bg_new, loader);
			loader = bool_loader(&show_obj_new, loader);
			loader = bool_loader(&race_2006, loader);
			loader = bool_loader(&race_2006_2, loader);
			loader = bool_loader(&sprite_eval_write, loader);
			loader = bool_loader(&is_even_cycle, loader);
			loader = bool_loader(&sprite_zero_in_range, loader);
			loader = bool_loader(&sprite_zero_go, loader);
			loader = bool_loader(&ppu_was_on, loader);
			loader = bool_loader(&do_vbl, loader);
			loader = bool_loader(&do_active_sl, loader);
			loader = bool_loader(&do_pre_vbl, loader);
			loader = bool_loader(&nmi_destiny, loader);
			loader = bool_loader(&evenOddDestiny, loader);
			loader = bool_loader(&renderspritenow, loader);
			loader = bool_loader(&junksprite, loader);

			loader = byte_loader(&_isVS2c05, loader);
			loader = byte_loader(&VRAMBuffer, loader);
			loader = byte_loader(&PPUGenLatch, loader);
			loader = byte_loader(&reg_2003, loader);
			loader = byte_loader(&reg_2006_2, loader);
			loader = byte_loader(&read_value, loader);
			loader = byte_loader(&ppu_open_bus, loader);

			loader = sint_loader(&cpu_step, loader);
			loader = sint_loader(&cpu_stepcounter, loader);
			loader = sint_loader(&ppudead, loader);
			loader = sint_loader(&NMI_PendingInstructions, loader);
			loader = sint_loader(&preNMIlines, loader);
			loader = sint_loader(&postNMIlines, loader);
			loader = sint_loader(&ppuphase, loader);
			loader = sint_loader(&Reg_2001_intensity_lsl_6, loader);
			loader = sint_loader(&PPUSTATUS_sl, loader);
			loader = sint_loader(&PPUSTATUS_cycle, loader);
			loader = sint_loader(&PPU_Regs_fv, loader);
			loader = sint_loader(&PPU_Regs_v, loader);
			loader = sint_loader(&PPU_Regs_h, loader);
			loader = sint_loader(&PPU_Regs_vt, loader);
			loader = sint_loader(&PPU_Regs_ht, loader);
			loader = sint_loader(&PPU_Regs_int_fv, loader);
			loader = sint_loader(&PPU_Regs_int_vt, loader);
			loader = sint_loader(&PPU_Regs_int_v, loader);
			loader = sint_loader(&PPU_Regs_int_h, loader);
			loader = sint_loader(&PPU_Regs_int_ht, loader);
			loader = sint_loader(&PPU_Regs_fh, loader);
			loader = sint_loader(&ppu_addr_temp, loader);
			loader = sint_loader(&pixelcolor_latch_1, loader);
			loader = sint_loader(&soam_index, loader);
			loader = sint_loader(&soam_index_prev, loader);
			loader = sint_loader(&soam_m_index, loader);
			loader = sint_loader(&oam_index, loader);
			loader = sint_loader(&oam_index_aux, loader);
			loader = sint_loader(&soam_index_aux, loader);
			loader = sint_loader(&spr_true_count, loader);
			loader = sint_loader(&yp, loader);
			loader = sint_loader(&target, loader);
			loader = sint_loader(&spriteHeight, loader);
			loader = sint_loader(&install_2006, loader);
			loader = sint_loader(&install_2001, loader);
			loader = sint_loader(&NMI_offset, loader);
			loader = sint_loader(&yp_shift, loader);
			loader = sint_loader(&sprite_eval_cycle, loader);
			loader = sint_loader(&xt, loader);
			loader = sint_loader(&xp, loader);
			loader = sint_loader(&xstart, loader);
			loader = sint_loader(&rasterpos, loader);
			loader = sint_loader(&s, loader);
			loader = sint_loader(&ppu_aux_index, loader);
			loader = sint_loader(&line, loader);
			loader = sint_loader(&patternNumber, loader);
			loader = sint_loader(&patternAddress, loader);
			loader = sint_loader(&temp_addr, loader);

			double_2007_read = *loader; loader++; double_2007_read |= ((uint64_t)*loader << 8); loader++;
			double_2007_read |= ((uint64_t)*loader << 16); loader++; double_2007_read |= ((uint64_t)*loader << 24); loader++;
			double_2007_read |= ((uint64_t)*loader << 32); loader++; double_2007_read |= ((uint64_t)*loader << 40); loader++;
			double_2007_read |= ((uint64_t)*loader << 48); loader++; double_2007_read |= ((uint64_t)*loader << 56); loader++;

			_totalCycles = *loader; loader++; _totalCycles |= ((uint64_t)*loader << 8); loader++;
			_totalCycles |= ((uint64_t)*loader << 16); loader++; _totalCycles |= ((uint64_t)*loader << 24); loader++;
			_totalCycles |= ((uint64_t)*loader << 32); loader++; _totalCycles |= ((uint64_t)*loader << 40); loader++;
			_totalCycles |= ((uint64_t)*loader << 48); loader++; _totalCycles |= ((uint64_t)*loader << 56); loader++;

			loader = byte_array_loader(CIRAM, loader, 0x800);
			loader = byte_array_loader(glitchy_reads_2003, loader, 8);
			loader = byte_array_loader(PALRAM, loader, 0x20);
			loader = byte_array_loader(OAM, loader, 0x100);
			loader = byte_array_loader(soam, loader, 256);
			loader = byte_array_loader(sl_sprites, loader, 3 * 256);

			loader = int_array_loader(ppu_open_bus_decay_timer, loader, 8);
			loader = int_array_loader(_currentLuma, loader, 64);

			for (int i = 0; i < 34; i++)
			{
				loader = byte_loader(&bgdata[i].nt, loader);
				loader = byte_loader(&bgdata[i].at, loader);
				loader = byte_loader(&bgdata[i].pt_0, loader);
				loader = byte_loader(&bgdata[i].pt_1, loader);
			}

			for (int i = 0; i < 64; i++)
			{
				loader = byte_loader(&t_oam[i].oam_y, loader);
				loader = byte_loader(&t_oam[i].oam_ind, loader);
				loader = byte_loader(&t_oam[i].oam_attr, loader);
				loader = byte_loader(&t_oam[i].oam_x, loader);
				loader = byte_loader(&t_oam[i].patterns_0, loader);
				loader = byte_loader(&t_oam[i].patterns_1, loader);
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
