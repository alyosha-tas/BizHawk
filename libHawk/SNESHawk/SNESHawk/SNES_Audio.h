#include <iostream>
#include <cstdint>
#include <iomanip>
#include <string>

using namespace std;

namespace SNESHawk
{
	class MemoryManager;
	
	class SNES_Audio
	{
	public:
		
		SNES_Audio()
		{
			// change tables to PAL tables if needed
		}

		MemoryManager* mem_ctrl;

		uint8_t ReadMemory(uint32_t);

		// pointers not stated
		bool* _irq_apu = nullptr;

		#pragma region Variables

		bool call_from_write;
		bool recalculate = false;
		bool irq_pending;
		bool dmc_irq;
		bool doing_tick_quarter = false;
		bool len_clock_active;
		bool sequencer_irq, sequence_reset_pending, sequencer_irq_clear_pending, sequencer_irq_flag;
		
		uint8_t pending_val = 0;
		uint8_t seq_val;

		int32_t seq_tick;
		int32_t m_vol = 1;
		int32_t dmc_dma_countdown = -1;
		int32_t sequencer_counter, sequencer_step, sequencer_mode, sequencer_irq_inhibit, sequencer_irq_assert;
		int32_t pending_reg = -1;
		int32_t pending_length_change;

		int32_t oldmix = 0;
		int32_t cart_sound = 0;
		int32_t old_cart_sound = 0;

		uint64_t sampleclock = 0;

		////////////////////////////////
		// Pulse Units
		////////////////////////////////
		
		int32_t unit_0, unit_1;

		// reg0
		bool len_halt_0, len_halt_1;
		int32_t duty_cnt_0, duty_cnt_1;
		int32_t env_loop_0, env_loop_1; 
		int32_t env_constant_0, env_constant_1;
		int32_t env_cnt_value_0, env_cnt_value_1;
		
		// reg1
		bool sweep_reload_0, sweep_reload_1;
		int32_t sweep_en_0, sweep_en_1;
		int32_t sweep_divider_cnt_0, sweep_divider_cnt_1;
		int32_t sweep_negate_0, sweep_negate_1;
		int32_t sweep_shiftcount_0, sweep_shiftcount_1;
	
		// reg2/3
		int32_t len_cnt_0, len_cnt_1;
		int32_t timer_raw_reload_value_0, timer_raw_reload_value_1;
		int32_t timer_reload_value_0, timer_reload_value_1;

		// misc..
		int32_t lenctr_en_0, lenctr_en_1;

		// state
		bool swp_silence_0, swp_silence_1;
		bool duty_value_0, duty_value_1;
		int32_t swp_divider_counter_0, swp_divider_counter_1;		
		int32_t duty_step_0, duty_step_1;
		int32_t timer_counter_0, timer_counter_1;
		int32_t sample_0, sample_1;
		int32_t env_start_flag_0, env_start_flag_1;
		int32_t env_divider_0, env_divider_1;
		int32_t env_counter_0, env_counter_1;
		int32_t env_output_0, env_output_1;

		////////////////////////////////
		// Noise Unit
		////////////////////////////////

		// reg0 (sweep)
		bool len_halt_N;
		int32_t env_cnt_value_N, env_loop_N, env_constant_N;	

		// reg2 (mode and period)
		int32_t mode_cnt_N, period_cnt_N;

		// reg3 (length counter and envelop trigger)
		int32_t len_cnt_N;

		// set from apu:
		int32_t lenctr_en_N;

		// state
		bool noise_bit_N = true;
		int32_t shift_register_N = 1;
		int32_t timer_counter_N;
		int32_t sample_N;
		int32_t env_output_N, env_start_flag_N, env_divider_N, env_counter_N;
		
		////////////////////////////////
		// Triangle Unit
		////////////////////////////////

		// reg0
		int32_t linear_counter_reload_T, control_flag_T;

		// reg1 (n/a)
		// reg2/3
		bool halt_2_T;
		int32_t timer_cnt_T, reload_flag_T, len_cnt_T;
		
		// misc..
		int32_t lenctr_en_T;
		int32_t linear_counter_T, timer_T, timer_cnt_reload_T;
		int32_t seq_T = 0;
		int32_t sample_T;

		////////////////////////////////
		// DMC Unit
		////////////////////////////////

		bool irq_enabled_D;
		bool loop_flag_D;
		int32_t timer_reload_D;

		// dmc delay per visual 2a03
		int32_t delay_D;

		bool sample_buffer_filled_D;
		bool out_silence_D;
		// this timer never stops, ever, so it is convenient to use for even/odd timing used elsewhere
		int32_t timer_D;
		int32_t user_address_D;
		int32_t user_length_D, sample_length_D;
		int32_t sample_address_D, sample_buffer_D;
		int32_t out_shift_D, out_bits_remaining_D, out_deltacounter_D;

		int32_t sample_D() { return out_deltacounter_D /* - 64*/; }

		#pragma endregion

		#pragma region Constants

		const int32_t LENGTH_TABLE[32] = { 10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14, 12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

		const uint8_t PULSE_DUTY[4][8] = {
			{0,1,0,0,0,0,0,0}, // (12.5%)
			{0,1,1,0,0,0,0,0}, // (25%)
			{0,1,1,1,1,0,0,0}, // (50%)
			{1,0,0,1,1,1,1,1}, // (25% negated (75%))
		};

		const uint8_t TRIANGLE_TABLE[32] =
		{
			15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
			0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
		};

		#pragma endregion

		#pragma region Audio Functions

		void RunDMCFetch()
		{
			Fetch_D();
		}

		int32_t DMC_RATE[16] = { 428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54 };
		int32_t DMC_RATE_PAL[16] = { 398, 354, 316, 298, 276, 236, 210, 198, 176, 148, 132, 118, 98, 78, 66, 50 };

		int32_t NOISE_TABLE[16] = { 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068 };
		int32_t NOISE_TABLE_PAL[16] = { 4, 7, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778 };

		int32_t sequencer_lut[2][5] = {
			{7457,14913,22371,29830, 0},
			{7457,14913,22371,29830,37282}
		};

		int32_t sequencer_lut_pal[2][5] = {
			{8313,16627,24939,33254, 0},
			{8313,16627,24939,33254,41566}
		};

		void sequencer_write_tick(uint8_t val)
		{
			if (seq_tick > 0)
			{
				seq_tick--;

				if (seq_tick == 0)
				{
					sequencer_mode = (val >> 7) & 1;

					// Console.WriteLine("apu 4017 = {0:X2}", val);
					// check if we will be doing the extra frame ticks or not
					if (sequencer_mode == 1)
					{
						if (!doing_tick_quarter)
						{
							QuarterFrame();
							HalfFrame();
						}
					}

					sequencer_irq_inhibit = (val >> 6) & 1;
					if (sequencer_irq_inhibit == 1)
					{
						sequencer_irq_flag = false;
					}

					sequencer_counter = 0;
					sequencer_step = 0;
				}
			}
		}

		void sequencer_tick()
		{
			sequencer_counter++;
			if (sequencer_mode == 0 && sequencer_counter == sequencer_lut[0][3] - 1)
			{
				if (sequencer_irq_inhibit == 0)
				{
					sequencer_irq_assert = 2;
					sequencer_irq_flag = true;
				}

				HalfFrame();
			}
			if (sequencer_mode == 0 && sequencer_counter == sequencer_lut[0][3] - 2 && sequencer_irq_inhibit == 0)
			{
				//sequencer_irq_assert = 2;
				sequencer_irq_flag = true;
			}
			if (sequencer_mode == 1 && sequencer_counter == sequencer_lut[1][4] - 1)
			{
				HalfFrame();
			}
			if (sequencer_lut[sequencer_mode][sequencer_step] != sequencer_counter)
				return;
			sequencer_check();
		}

		void SyncIRQ()
		{
			irq_pending = sequencer_irq | dmc_irq;
		}

		void sequencer_check()
		{
			bool quarter = false;
			bool half = false;
			bool reset = false;

			switch (sequencer_mode)
			{
				case 0: // 4-step
					quarter = true;
					half = sequencer_step == 1;
					reset = sequencer_step == 3;
					if (reset && sequencer_irq_inhibit == 0)
					{
						// sequencer_irq_assert = 2;
						sequencer_irq_flag = true;
					}
					break;

				case 1: // 5-step
					quarter = sequencer_step != 3;
					half = sequencer_step == 1;
					reset = sequencer_step == 4;
					break;

			default:
				//throw new InvalidOperationException();
				break;
			}

			if (reset)
			{
				sequencer_counter = 0;
				sequencer_step = 0;
			}
			else sequencer_step++;

			if (quarter) QuarterFrame();
			if (half) HalfFrame();
		}

		void HalfFrame()
		{
			doing_tick_quarter = true;
			clock_length_and_sweep_0();
			clock_length_and_sweep_1();
			clock_length_and_sweep_T();
			clock_length_and_sweep_N();
		}

		void QuarterFrame()
		{
			doing_tick_quarter = true;
			clock_env_0();
			clock_env_1();
			clock_linear_counter_T();
			clock_env_N();
		}

		void NESSoftReset()
		{
			// need to study what happens to apu and stuff..
			sequencer_irq = false;
			sequencer_irq_flag = false;
			_WriteReg(0x4015, 0);

			// for 4017, its as if the last value written gets rewritten
			sequencer_mode = (seq_val >> 7) & 1;
			sequencer_irq_inhibit = (seq_val >> 6) & 1;
			if (sequencer_irq_inhibit == 1)
			{
				sequencer_irq_flag = false;
			}
			sequencer_counter = 0;
			sequencer_step = 0;
		}

		void NESHardReset()
		{
			// "at power on it is as if $00 was written to $4017 9-12 cycles before the reset vector"
			// that translates to a starting value for the counter of -3
			sequencer_counter = -1;
		}

		void WriteReg(int32_t addr, uint8_t val)
		{
			pending_reg = addr;
			pending_val = val;
		}

		void _WriteReg(int32_t addr, uint8_t val)
		{
			int32_t index = addr - 0x4000;
			int32_t reg = index & 3;
			int32_t channel = index >> 2;
			switch (channel)
			{
			case 0:
				WriteReg_0(reg, val);
				break;
			case 1:
				WriteReg_1(reg, val);
				break;
			case 2:
				WriteReg_T(reg, val);
				break;
			case 3:
				WriteReg_N(reg, val);
				break;
			case 4:
				WriteReg_D(reg, val);
				break;
			case 5:
				if (addr == 0x4015)
				{
					set_lenctr_en_0(val & 1);
					set_lenctr_en_1((val >> 1) & 1);
					set_lenctr_en_T((val >> 2) & 1);
					set_lenctr_en_N((val >> 3) & 1);
					set_lenctr_en_D((val & 0x10) == 0x10);

				}
				else if (addr == 0x4017)
				{
					if (timer_D % 2 == 0)
					{
						seq_tick = 3;

					}
					else
					{
						seq_tick = 4;
					}

					seq_val = val;
				}
				break;
			}
		}

		uint8_t PeekReg(int32_t addr)
		{
			switch (addr)
			{
			case 0x4015:
			{
				//notice a missing bit here. should properly emulate with empty / Data bus
				//if an interrupt flag was set at the same moment of the read, it will read back as 1 but it will not be cleared. 
				int32_t dmc_nonzero = IsLenCntNonZero_D() ? 1 : 0;
				int32_t noise_nonzero = IsLenCntNonZero_N() ? 1 : 0;
				int32_t tri_nonzero = IsLenCntNonZero_T() ? 1 : 0;
				int32_t pulse1_nonzero = IsLenCntNonZero_1() ? 1 : 0;
				int32_t pulse0_nonzero = IsLenCntNonZero_0() ? 1 : 0;
				int32_t ret = ((dmc_irq ? 1 : 0) << 7) | ((sequencer_irq_flag ? 1 : 0) << 6) | (dmc_nonzero << 4) | (noise_nonzero << 3) | (tri_nonzero << 2) | (pulse1_nonzero << 1) | (pulse0_nonzero);
				return (uint8_t)ret;
			}
			default:
				// don't return 0xFF here or SMB will break
				return 0x00;
			}
		}

		uint8_t ReadReg(int32_t addr)
		{
			switch (addr)
			{
			case 0x4015:
			{
				uint8_t ret = PeekReg(0x4015);
				// Console.WriteLine("{0} {1,5} $4015 clear irq, was at {2}", nes.Frame, sequencer_counter, sequencer_irq);
				sequencer_irq_flag = false;
				SyncIRQ();
				return ret;
			}
			default:
				// don't return 0xFF here or SMB will break
				return 0x00;
			}
		}



		void RunOneFirst()
		{
			Run_0();
			Run_1();
			Run_T();
			Run_N();
			Run_D();

			len_halt_0 = false;
			len_halt_1 = false;
			len_halt_N = false;
		}

		void RunOneLast()
		{
			if (pending_length_change > 0)
			{
				pending_length_change--;
				if (pending_length_change == 0)
				{
					sample_length_D--;
				}
			}

			// we need to predict if there will be a length clock here, because the sequencer ticks last, but the 
			// timer reload shouldn't happen if length clock and write happen simultaneously
			// I'm not sure if we can avoid this by simply processing the sequencer first
			// but at the moment that would break everything, so this is good enough for now
			if (sequencer_counter == (sequencer_lut[0][1] - 1) ||
				(sequencer_counter == sequencer_lut[0][3] - 2 && sequencer_mode == 0) ||
				(sequencer_counter == sequencer_lut[1][4] - 2 && sequencer_mode == 1))
			{
				len_clock_active = true;
			}

			// handle writes
			// notes: this set up is a bit convoluded at the moment, mainly because APU behaviour is not entirely understood
			// in partiuclar, there are several clock pulses affecting the APU, and when new written are latched is not known in detail
			// the current code simply matches known behaviour			
			if (pending_reg != -1)
			{
				if (pending_reg == 0x4015 || pending_reg == 0x4003 || pending_reg == 0x4007)
				{
					_WriteReg(pending_reg, pending_val);
					pending_reg = -1;
				}
				else if (timer_D % 2 == 0)
				{
					_WriteReg(pending_reg, pending_val);
					pending_reg = -1;
				}
			}

			len_clock_active = false;

			sequencer_tick();
			sequencer_write_tick(seq_val);
			doing_tick_quarter = false;

			if (sequencer_irq_assert > 0)
			{
				sequencer_irq_assert--;
				if (sequencer_irq_assert == 0)
				{
					sequencer_irq = true;
				}
			}

			SyncIRQ();
			_irq_apu[0] = irq_pending;

			// since the units run concurrently, the APU frame sequencer is ran last because
			// it can change the output values of the pulse/triangle channels
			// we want the changes to affect it on the *next* cycle.

			if (sequencer_irq_flag == false)
				sequencer_irq = false;
		}

		/// <summary>only call in board.ClockCPU()</summary>
		void ExternalQueue(int32_t value)
		{
			cart_sound = value + old_cart_sound;

			if (cart_sound != old_cart_sound)
			{
				recalculate = true;
				old_cart_sound = cart_sound;
			}
		}

		int32_t EmitSample()
		{
			if (recalculate)
			{
				recalculate = false;

				int32_t s_pulse0 = sample_0;
				int32_t s_pulse1 = sample_1;
				int32_t s_tri = sample_T;
				int32_t s_noise = sample_N;
				int32_t s_dmc = sample_D();

				// more properly correct
				float pulse_out = s_pulse0 == 0 && s_pulse1 == 0
					? 0
					: 95.88f / ((8128.0f / (s_pulse0 + s_pulse1)) + 100.0f);

				float tnd_out = s_tri == 0 && s_noise == 0 && s_dmc == 0
					? 0
					: 159.79f / (1 / ((s_tri / 8227.0f) + (s_noise / 12241.0f /* * NOISEADJUST*/) + (s_dmc / 22638.0f)) + 100);


				float output = pulse_out + tnd_out;

				// output = output * 2 - 1;
				// this needs to leave enough headroom for straying DC bias due to the DMC unit getting stuck outputs. smb3 is bad about that. 
				oldmix = (int)(20000 * output * (1 + m_vol / 5)) + cart_sound;
			}

			return oldmix;
		}

		////////////////////////////////
		// Pulse Units
		////////////////////////////////

		void WriteReg_0(int32_t addr, uint8_t val)
		{
			// Console.WriteLine("write pulse {0:X} {1:X}", addr, val);
			switch (addr)
			{
			case 0:
				env_cnt_value_0 = val & 0xF;
				env_constant_0 = (val >> 4) & 1;
				env_loop_0 = (val >> 5) & 1;
				duty_cnt_0 = (val >> 6) & 3;
				break;
			case 1:
				sweep_shiftcount_0 = val & 7;
				sweep_negate_0 = (val >> 3) & 1;
				sweep_divider_cnt_0 = (val >> 4) & 7;
				sweep_en_0 = (val >> 7) & 1;
				sweep_reload_0 = true;
				break;
			case 2:
				timer_reload_value_0 = (timer_reload_value_0 & 0x700) | val;
				timer_raw_reload_value_0 = timer_reload_value_0 * 2 + 2;
				// if (unit == 1) Console.WriteLine("{0} timer_reload_value: {1}", unit, timer_reload_value);
				break;
			case 3:
				if (len_clock_active)
				{
					if (len_cnt_0 == 0)
					{
						len_cnt_0 = LENGTH_TABLE[(val >> 3) & 0x1F] + 1;
					}
				}
				else
				{
					len_cnt_0 = LENGTH_TABLE[(val >> 3) & 0x1F];
				}

				timer_reload_value_0 = (timer_reload_value_0 & 0xFF) | ((val & 0x07) << 8);
				timer_raw_reload_value_0 = timer_reload_value_0 * 2 + 2;
				duty_step_0 = 0;
				env_start_flag_0 = 1;

				// allow the lenctr_en to kill the len_cnt
				set_lenctr_en_0(lenctr_en_0);

				// serves as a useful note-on diagnostic
				// if(unit==1) Console.WriteLine("{0} timer_reload_value: {1}", unit, timer_reload_value);
				break;
			}
		}

		void WriteReg_1(int32_t addr, uint8_t val)
		{
			// Console.WriteLine("write pulse {0:X} {1:X}", addr, val);
			switch (addr)
			{
			case 0:
				env_cnt_value_1 = val & 0xF;
				env_constant_1 = (val >> 4) & 1;
				env_loop_1 = (val >> 5) & 1;
				duty_cnt_1 = (val >> 6) & 3;
				break;
			case 1:
				sweep_shiftcount_1 = val & 7;
				sweep_negate_1 = (val >> 3) & 1;
				sweep_divider_cnt_1 = (val >> 4) & 7;
				sweep_en_1 = (val >> 7) & 1;
				sweep_reload_1 = true;
				break;
			case 2:
				timer_reload_value_1 = (timer_reload_value_1 & 0x700) | val;
				timer_raw_reload_value_1 = timer_reload_value_1 * 2 + 2;
				// if (unit == 1) Console.WriteLine("{0} timer_reload_value: {1}", unit, timer_reload_value);
				break;
			case 3:
				if (len_clock_active)
				{
					if (len_cnt_1 == 0)
					{
						len_cnt_1 = LENGTH_TABLE[(val >> 3) & 0x1F] + 1;
					}
				}
				else
				{
					len_cnt_1 = LENGTH_TABLE[(val >> 3) & 0x1F];
				}

				timer_reload_value_1 = (timer_reload_value_1 & 0xFF) | ((val & 0x07) << 8);
				timer_raw_reload_value_1 = timer_reload_value_1 * 2 + 2;
				duty_step_1 = 0;
				env_start_flag_1 = 1;

				// allow the lenctr_en to kill the len_cnt
				set_lenctr_en_1(lenctr_en_1);

				// serves as a useful note-on diagnostic
				// if(unit==1) Console.WriteLine("{0} timer_reload_value: {1}", unit, timer_reload_value);
				break;
			}
		}

		bool IsLenCntNonZero_0() { return len_cnt_0 > 0; }
		bool IsLenCntNonZero_1() { return len_cnt_1 > 0; }

		void set_lenctr_en_0(int32_t value)
		{
			lenctr_en_0 = value;
			// if the length counter is not enabled, then we must disable the length system in this way
			if (lenctr_en_0 == 0) len_cnt_0 = 0;
		}
		void set_lenctr_en_1(int32_t value)
		{
			lenctr_en_1 = value;
			// if the length counter is not enabled, then we must disable the length system in this way
			if (lenctr_en_1 == 0) len_cnt_1 = 0;
		}

		void clock_length_and_sweep_0()
		{
			// this should be optimized to update only when `timer_reload_value` changes
			int32_t sweep_shifter = timer_reload_value_0 >> sweep_shiftcount_0;
			if (sweep_negate_0 == 1)
				sweep_shifter = -sweep_shifter - unit_0;
			sweep_shifter += timer_reload_value_0;

			// this sweep logic is always enabled:
			swp_silence_0 = (timer_reload_value_0 < 8 || (sweep_shifter > 0x7FF)); // && sweep_negate == 0));

			// does enable only block the pitch bend? does the clocking proceed?
			if (sweep_en_0 == 1)
			{
				// clock divider
				if (swp_divider_counter_0 != 0) swp_divider_counter_0--;
				if (swp_divider_counter_0 == 0)
				{
					swp_divider_counter_0 = sweep_divider_cnt_0 + 1;

					// divider was clocked: process sweep pitch bend
					if (sweep_shiftcount_0 != 0 && !swp_silence_0)
					{
						timer_reload_value_0 = sweep_shifter;
						timer_raw_reload_value_0 = (timer_reload_value_0 << 1) + 2;
					}
					// TODO - does this change the user's reload value or the latched reload value?
				}

				// handle divider reload, after clocking happens
				if (sweep_reload_0)
				{
					swp_divider_counter_0 = sweep_divider_cnt_0 + 1;
					sweep_reload_0 = false;
				}
			}

			// env_loop doubles as "halt length counter"
			if ((env_loop_0 == 0 || len_halt_0) && len_cnt_0 > 0)
				len_cnt_0--;
		}

		void clock_length_and_sweep_1()
		{
			// this should be optimized to update only when `timer_reload_value` changes
			int32_t sweep_shifter = timer_reload_value_1 >> sweep_shiftcount_1;
			if (sweep_negate_1 == 1)
				sweep_shifter = -sweep_shifter - unit_1;
			sweep_shifter += timer_reload_value_1;

			// this sweep logic is always enabled:
			swp_silence_1 = (timer_reload_value_1 < 8 || (sweep_shifter > 0x7FF)); // && sweep_negate == 0));

			// does enable only block the pitch bend? does the clocking proceed?
			if (sweep_en_1 == 1)
			{
				// clock divider
				if (swp_divider_counter_1 != 0) swp_divider_counter_1--;
				if (swp_divider_counter_1 == 0)
				{
					swp_divider_counter_1 = sweep_divider_cnt_1 + 1;

					// divider was clocked: process sweep pitch bend
					if (sweep_shiftcount_1 != 0 && !swp_silence_1)
					{
						timer_reload_value_1 = sweep_shifter;
						timer_raw_reload_value_1 = (timer_reload_value_1 << 1) + 2;
					}
					// TODO - does this change the user's reload value or the latched reload value?
				}

				// handle divider reload, after clocking happens
				if (sweep_reload_1)
				{
					swp_divider_counter_1 = sweep_divider_cnt_1 + 1;
					sweep_reload_1 = false;
				}
			}

			// env_loop doubles as "halt length counter"
			if ((env_loop_1 == 0 || len_halt_1) && len_cnt_1 > 0)
				len_cnt_1--;
		}

		void clock_env_0()
		{
			if (env_start_flag_0 == 1)
			{
				env_start_flag_0 = 0;
				env_divider_0 = env_cnt_value_0;
				env_counter_0 = 15;
			}
			else
			{
				if (env_divider_0 != 0)
				{
					env_divider_0--;
				}
				else if (env_divider_0 == 0)
				{
					env_divider_0 = env_cnt_value_0;
					if (env_counter_0 == 0)
					{
						if (env_loop_0 == 1)
						{
							env_counter_0 = 15;
						}
					}
					else env_counter_0--;
				}
			}
		}

		void clock_env_1()
		{
			if (env_start_flag_1 == 1)
			{
				env_start_flag_1 = 0;
				env_divider_1 = env_cnt_value_1;
				env_counter_1 = 15;
			}
			else
			{
				if (env_divider_1 != 0)
				{
					env_divider_1--;
				}
				else if (env_divider_1 == 0)
				{
					env_divider_1 = env_cnt_value_1;
					if (env_counter_1 == 0)
					{
						if (env_loop_1 == 1)
						{
							env_counter_1 = 15;
						}
					}
					else env_counter_1--;
				}
			}
		}

		void Run_0()
		{
			if (env_constant_0 == 1)
				env_output_0 = env_cnt_value_0;
			else env_output_0 = env_counter_0;

			if (timer_counter_0 > 0) timer_counter_0--;
			if (timer_counter_0 == 0 && timer_raw_reload_value_0 != 0)
			{
				if (duty_step_0 == 7)
				{
					duty_step_0 = 0;
				}
				else
				{
					duty_step_0++;
				}
				duty_value_0 = PULSE_DUTY[duty_cnt_0][duty_step_0] == 1;
				// reload timer
				timer_counter_0 = timer_raw_reload_value_0;
			}

			int32_t newsample;

			if (duty_value_0) // high state of duty cycle
			{
				newsample = env_output_0;
				if (swp_silence_0 || len_cnt_0 == 0)
					newsample = 0; // silenced
			}
			else
				newsample = 0; // duty cycle is 0, silenced.

			// newsample -= env_output >> 1; //unbias
			if (newsample != sample_0)
			{
				recalculate = true;
				sample_0 = newsample;
			}
		}

		void Run_1()
		{
			if (env_constant_1 == 1)
				env_output_1 = env_cnt_value_1;
			else env_output_1 = env_counter_1;

			if (timer_counter_1 > 0) timer_counter_1--;
			if (timer_counter_1 == 0 && timer_raw_reload_value_1 != 0)
			{
				if (duty_step_1 == 7)
				{
					duty_step_1 = 0;
				}
				else
				{
					duty_step_1++;
				}
				duty_value_1 = PULSE_DUTY[duty_cnt_1][duty_step_1] == 1;
				// reload timer
				timer_counter_1 = timer_raw_reload_value_1;
			}

			int32_t newsample;

			if (duty_value_1) // high state of duty cycle
			{
				newsample = env_output_1;
				if (swp_silence_1 || len_cnt_1 == 0)
					newsample = 0; // silenced
			}
			else
				newsample = 0; // duty cycle is 0, silenced.

			// newsample -= env_output >> 1; //unbias
			if (newsample != sample_1)
			{
				recalculate = true;
				sample_1 = newsample;
			}
		}

		////////////////////////////////
		// Noise Unit
		////////////////////////////////

		void WriteReg_N(int32_t addr, uint8_t val)
		{
			switch (addr)
			{
			case 0:
				env_cnt_value_N = val & 0xF;
				env_constant_N = (val >> 4) & 1;
				// we want to delay a halt until after a length clock if they happen on the same cycle
				if (env_loop_N == 0 && ((val >> 5) & 1) == 1)
				{
					len_halt_N = true;
				}
				env_loop_N = (val >> 5) & 1;
				break;
			case 1:
				break;
			case 2:
				period_cnt_N = NOISE_TABLE[val & 0xF];
				mode_cnt_N = (val >> 7) & 1;
				break;
			case 3:
				if (len_clock_active)
				{
					if (len_cnt_N == 0)
					{
						len_cnt_N = LENGTH_TABLE[(val >> 3) & 0x1F] + 1;
					}
				}
				else
				{
					len_cnt_N = LENGTH_TABLE[(val >> 3) & 0x1F];
				}

				set_lenctr_en_N(lenctr_en_N);
				env_start_flag_N = 1;
				break;
			}
		}

		bool IsLenCntNonZero_N() { return len_cnt_N > 0; }

		void set_lenctr_en_N(int32_t value)
		{
			lenctr_en_N = value;
			// if the length counter is not enabled, then we must disable the length system in this way
			if (lenctr_en_N == 0) len_cnt_N = 0;
		}

		void clock_env_N()
		{
			if (env_start_flag_N == 1)
			{
				env_start_flag_N = 0;
				env_divider_N = (env_cnt_value_N + 1);
				env_counter_N = 15;
			}
			else
			{
				if (env_divider_N != 0) env_divider_N--;
				if (env_divider_N == 0)
				{
					env_divider_N = (env_cnt_value_N + 1);
					if (env_counter_N == 0)
					{
						if (env_loop_N == 1)
						{
							env_counter_N = 15;
						}
					}
					else env_counter_N--;
				}
			}
		}

		void clock_length_and_sweep_N()
		{

			if (len_cnt_N > 0 && (env_loop_N == 0 || len_halt_N))
				len_cnt_N--;
		}

		void Run_N()
		{
			if (env_constant_N == 1) 
			{
				env_output_N = env_cnt_value_N;
			}			
			else
			{
				env_output_N = env_counter_N;
			}

			if (timer_counter_N > 0) { timer_counter_N--; }
			if (timer_counter_N == 0 && period_cnt_N != 0)
			{
				// reload timer
				timer_counter_N = period_cnt_N;
				int32_t feedback_bit;
				if (mode_cnt_N == 1) feedback_bit = (shift_register_N >> 6) & 1;
				else feedback_bit = (shift_register_N >> 1) & 1;
				int32_t feedback = feedback_bit ^ (shift_register_N & 1);
				shift_register_N >>= 1;
				shift_register_N &= ~(1 << 14);
				shift_register_N |= (feedback << 14);
				noise_bit_N = (shift_register_N & 1) != 0;
			}

			int32_t newsample;
			if (len_cnt_N == 0) newsample = 0;
			else if (noise_bit_N) newsample = env_output_N; // switched, was 0?
			else newsample = 0;
			if (newsample != sample_N)
			{
				recalculate = true;
				sample_N = newsample;
			}
		}

		////////////////////////////////
		// Triangle Unit
		////////////////////////////////

		void WriteReg_T(int32_t addr, uint8_t val)
		{
			switch (addr)
			{
			case 0:
				linear_counter_reload_T = (val & 0x7F);
				control_flag_T = (val >> 7) & 1;
				break;
			case 1: break;
			case 2:
				timer_cnt_T = (timer_cnt_T & ~0xFF) | val;
				timer_cnt_reload_T = timer_cnt_T + 1;
				break;
			case 3:
				timer_cnt_T = (timer_cnt_T & 0xFF) | ((val & 0x7) << 8);
				timer_cnt_reload_T = timer_cnt_T + 1;
				if (len_clock_active)
				{
					if (len_cnt_T == 0)
					{
						len_cnt_T = LENGTH_TABLE[(val >> 3) & 0x1F] + 1;
					}
				}
				else
				{
					len_cnt_T = LENGTH_TABLE[(val >> 3) & 0x1F];
				}
				reload_flag_T = 1;

				// allow the lenctr_en to kill the len_cnt
				set_lenctr_en_T(lenctr_en_T);
				break;
			}
		}

		bool IsLenCntNonZero_T() { return len_cnt_T > 0; }

		void set_lenctr_en_T(int32_t value)
		{
			lenctr_en_T = value;
			// if the length counter is not enabled, then we must disable the length system in this way
			if (lenctr_en_T == 0) len_cnt_T = 0;
		}

		void Run_T()
		{
			// when clocked by timer, seq steps forward
			// except when linear counter or length counter is 0 
			bool en = len_cnt_T != 0 && linear_counter_T != 0;

			bool do_clock = false;
			if (timer_T > 0) timer_T--;
			if (timer_T == 0)
			{
				do_clock = true;
				timer_T = timer_cnt_reload_T;
			}

			if (en && do_clock)
			{
				int32_t newsample;

				seq_T = (seq_T + 1) & 0x1F;

				newsample = TRIANGLE_TABLE[seq_T];

				// special hack: frequently, games will use the maximum frequency triangle in order to mute it
				// apparently this results in the DAC for the triangle wave outputting a steady level at about 7.5
				// so we'll emulate it at the digital level
				if (timer_cnt_reload_T == 1) newsample = 8;

				if (newsample != sample_T)
				{
					recalculate = true;
					sample_T = newsample;
				}
			}
		}

		void clock_length_and_sweep_T()
		{
			// env_loopdoubles as "halt length counter"
			if (len_cnt_T > 0 && control_flag_T == 0)
				len_cnt_T--;
		}

		void clock_linear_counter_T()
		{
			if (reload_flag_T == 1)
			{
				linear_counter_T = linear_counter_reload_T;
			}
			else if (linear_counter_T != 0)
			{
				linear_counter_T--;
			}

			if (control_flag_T == 0) { reload_flag_T = 0; }
		}

		////////////////////////////////
		// DMC Unit
		////////////////////////////////

		void WriteReg_D(int32_t addr, uint8_t val)
		{
			switch (addr)
			{
			case 0:
				irq_enabled_D = (val & 0x80) == 0x80;
				loop_flag_D = (val & 0x40) == 0x40;
				timer_reload_D = DMC_RATE[val & 0xF];
				if (!irq_enabled_D) { dmc_irq = false; }
				// apu.dmc_irq = false;
				SyncIRQ();
				break;
			case 1:
				out_deltacounter_D = val & 0x7F;
				recalculate = true;
				break;
			case 2:
				user_address_D = 0xC000 | (val << 6);
				break;
			case 3:
				user_length_D = ((uint16_t)val << 4) + 1;
				break;
			}
		}

		void Run_D()
		{
			if (timer_D > 0) timer_D--;
			if (timer_D == 0)
			{
				timer_D = timer_reload_D;
				Clock_D();
			}

			// Any time the sample buffer is in an empty state and bytes remaining is not zero, the following occur: 
			// also note that the halt for DMC DMA occurs on APU cycles only (hence the timer check)
			if (!sample_buffer_filled_D && sample_length_D > 0 && dmc_dma_countdown == -1 && delay_D == 0)
			{
				// calls from write take one less cycle, but start on a write instead of a read
				if (!call_from_write)
				{
					if (timer_D % 2 == 1)
					{
						delay_D = 3;
					}
					else
					{
						delay_D = 2;
					}
				}
				else
				{
					if (timer_D % 2 == 1)
					{
						delay_D = 2;
					}
					else
					{
						delay_D = 3;
					}
				}
			}

			// I did some tests in Visual 2A03 and there seems to be some delay betwen when a DMC is first needed and when the 
			// process to execute the DMA starts. The details are not currently known, but it seems to be a 2 cycle delay
			if (delay_D != 0)
			{
				delay_D--;
				if (delay_D == 0)
				{
					if (!call_from_write)
					{
						dmc_dma_countdown = 4;
					}
					else
					{

						dmc_dma_countdown = 3;
						call_from_write = false;
					}
				}
			}
		}

		void Clock_D()
		{
			// If the silence flag is clear, bit 0 of the shift register is applied to the counter as follows: 
			// if bit 0 is clear and the delta-counter is greater than 1, the counter is decremented by 2; 
			// otherwise, if bit 0 is set and the delta-counter is less than 126, the counter is incremented by 2
			if (!out_silence_D)
			{
				// apply current sample bit to delta counter
				if ((out_shift_D & 1) == 1)
				{
					if (out_deltacounter_D < 126)
						out_deltacounter_D += 2;
				}
				else
				{
					if (out_deltacounter_D > 1)
						out_deltacounter_D -= 2;
				}

				recalculate = true;
			}

			// The right shift register is clocked. 
			out_shift_D >>= 1;

			// The bits-remaining counter is decremented. If it becomes zero, a new cycle is started. 
			if (out_bits_remaining_D == 0)
			{
				// The bits-remaining counter is loaded with 8. 
				out_bits_remaining_D = 7;
				// If the sample buffer is empty then the silence flag is set
				if (!sample_buffer_filled_D)
				{
					out_silence_D = true;
				}
				else
					// otherwise, the silence flag is cleared and the sample buffer is emptied into the shift register. 
				{
					out_silence_D = false;
					out_shift_D = sample_buffer_D;
					sample_buffer_filled_D = false;
				}
			}
			else out_bits_remaining_D--;
		}

		void set_lenctr_en_D(bool en)
		{
			if (!en)
			{
				// If the DMC bit is clear, the DMC bytes remaining will be set to 0 
				// and the DMC will silence when it empties.
				sample_length_D = 0;
			}
			else
			{
				// only start playback if playback is stopped
				if (sample_length_D == 0)
				{
					sample_address_D = user_address_D;
					sample_length_D = user_length_D;

				}
				if (!sample_buffer_filled_D)
				{
					// apparently the dmc is different if called from a cpu write, let's try
					call_from_write = true;
				}
			}

			// irq is acknowledged or sure to be clear, in either case
			dmc_irq = false;
			SyncIRQ();
		}

		bool IsLenCntNonZero_D()
		{
			return sample_length_D != 0;
		}

		void Fetch_D()
		{
			if (sample_length_D != 0)
			{
				sample_buffer_D = ReadMemory((uint16_t)sample_address_D);
				sample_buffer_filled_D = true;
				sample_address_D = (uint16_t)(sample_address_D + 1);
				sample_length_D--;
				// apu.pending_length_change = 1;
			}
			if (sample_length_D == 0)
			{
				if (loop_flag_D)
				{
					sample_address_D = user_address_D;
					sample_length_D = user_length_D;
				}
				else if (irq_enabled_D) dmc_irq = true;
			}
		}

		#pragma endregion


		#pragma region State Save / Load

		uint8_t* SaveState(uint8_t* saver)
		{
			saver = bool_saver(call_from_write, saver);
			saver = bool_saver(recalculate, saver);
			saver = bool_saver(irq_pending, saver);
			saver = bool_saver(dmc_irq, saver);
			saver = bool_saver(doing_tick_quarter, saver);
			saver = bool_saver(len_clock_active, saver);
			saver = bool_saver(sequencer_irq, saver);
			saver = bool_saver(sequence_reset_pending, saver);
			saver = bool_saver(sequencer_irq_clear_pending, saver);
			saver = bool_saver(sequencer_irq_flag, saver);

			saver = byte_saver(pending_val, saver);
			saver = byte_saver(seq_val, saver);

			saver = int_saver(seq_tick, saver);
			saver = int_saver(m_vol, saver);
			saver = int_saver(dmc_dma_countdown, saver);
			saver = int_saver(sequencer_counter, saver);
			saver = int_saver(sequencer_step, saver);
			saver = int_saver(sequencer_mode, saver);
			saver = int_saver(sequencer_irq_inhibit, saver);
			saver = int_saver(sequencer_irq_assert, saver);
			saver = int_saver(pending_reg, saver);
			saver = int_saver(pending_length_change, saver);

			saver = int_saver(oldmix, saver);
			saver = int_saver(cart_sound, saver);
			saver = int_saver(old_cart_sound, saver);

			*saver = (uint8_t)(sampleclock & 0xFF); saver++; *saver = (uint8_t)((sampleclock >> 8) & 0xFF); saver++;
			*saver = (uint8_t)((sampleclock >> 16) & 0xFF); saver++; *saver = (uint8_t)((sampleclock >> 24) & 0xFF); saver++;
			*saver = (uint8_t)((sampleclock >> 32) & 0xFF); saver++; *saver = (uint8_t)((sampleclock >> 40) & 0xFF); saver++;
			*saver = (uint8_t)((sampleclock >> 48) & 0xFF); saver++; *saver = (uint8_t)((sampleclock >> 56) & 0xFF); saver++;

			////////////////////////////////
			// Pulse Units
			////////////////////////////////

			saver = int_saver(unit_0, saver);
			saver = int_saver(unit_1, saver);

			// reg0
			saver = bool_saver(len_halt_0, saver);
			saver = bool_saver(len_halt_1, saver);
			saver = int_saver(duty_cnt_0, saver);
			saver = int_saver(duty_cnt_1, saver);
			saver = int_saver(env_loop_0, saver);
			saver = int_saver(env_loop_1, saver);
			saver = int_saver(env_constant_0, saver);
			saver = int_saver(env_constant_1, saver);
			saver = int_saver(env_cnt_value_0, saver);
			saver = int_saver(env_cnt_value_1, saver);

			// reg1
			saver = bool_saver(sweep_reload_0, saver);
			saver = bool_saver(sweep_reload_1, saver);
			saver = int_saver(sweep_en_0, saver);
			saver = int_saver(sweep_en_1, saver);
			saver = int_saver(sweep_divider_cnt_0, saver);
			saver = int_saver(sweep_divider_cnt_1, saver);
			saver = int_saver(sweep_negate_0, saver);
			saver = int_saver(sweep_negate_1, saver);
			saver = int_saver(sweep_shiftcount_0, saver);
			saver = int_saver(sweep_shiftcount_1, saver);

			// reg2/3
			saver = int_saver(len_cnt_0, saver);
			saver = int_saver(len_cnt_1, saver);
			saver = int_saver(timer_raw_reload_value_0, saver);
			saver = int_saver(timer_raw_reload_value_1, saver);
			saver = int_saver(timer_reload_value_0, saver);
			saver = int_saver(timer_reload_value_1, saver);

			// misc..
			saver = int_saver(lenctr_en_0, saver);
			saver = int_saver(lenctr_en_1, saver);

			// state
			saver = bool_saver(swp_silence_0, saver);
			saver = bool_saver(swp_silence_1, saver);
			saver = bool_saver(duty_value_0, saver);
			saver = bool_saver(duty_value_1, saver);
			saver = int_saver(swp_divider_counter_0, saver);
			saver = int_saver(swp_divider_counter_1, saver);
			saver = int_saver(duty_step_0, saver);
			saver = int_saver(duty_step_1, saver);
			saver = int_saver(timer_counter_0, saver);
			saver = int_saver(timer_counter_1, saver);
			saver = int_saver(sample_0, saver);
			saver = int_saver(sample_1, saver);
			saver = int_saver(env_start_flag_0, saver);
			saver = int_saver(env_start_flag_1, saver);
			saver = int_saver(env_divider_0, saver);
			saver = int_saver(env_divider_1, saver);
			saver = int_saver(env_counter_0, saver);
			saver = int_saver(env_counter_1, saver);
			saver = int_saver(env_output_0, saver);
			saver = int_saver(env_output_1, saver);

			////////////////////////////////
			// Noise Unit
			////////////////////////////////

			// reg0 (sweep)
			saver = bool_saver(len_halt_N, saver);
			saver = int_saver(env_cnt_value_N, saver);
			saver = int_saver(env_loop_N, saver);
			saver = int_saver(env_constant_N, saver);

			// reg2 (mode and period)
			saver = int_saver(mode_cnt_N, saver);
			saver = int_saver(period_cnt_N, saver);

			// reg3 (length counter and envelop trigger)
			saver = int_saver(len_cnt_N, saver);

			// set from apu:
			saver = int_saver(lenctr_en_N, saver);

			// state
			saver = bool_saver(noise_bit_N, saver);
			saver = int_saver(shift_register_N, saver);
			saver = int_saver(timer_counter_N, saver);
			saver = int_saver(sample_N, saver);
			saver = int_saver(env_output_N, saver);
			saver = int_saver(env_start_flag_N, saver);
			saver = int_saver(env_divider_N, saver);
			saver = int_saver(env_counter_N, saver);

			////////////////////////////////
			// Triangle Unit
			////////////////////////////////

			// reg0
			saver = int_saver(linear_counter_reload_T, saver);
			saver = int_saver(control_flag_T, saver);

			// reg1 (n/a)
			// reg2/3
			saver = bool_saver(halt_2_T, saver);
			saver = int_saver(timer_cnt_T, saver);
			saver = int_saver(reload_flag_T, saver);
			saver = int_saver(len_cnt_T, saver);

			// misc..
			saver = int_saver(lenctr_en_T, saver);
			saver = int_saver(linear_counter_T, saver);
			saver = int_saver(timer_T, saver);
			saver = int_saver(timer_cnt_reload_T, saver);
			saver = int_saver(seq_T, saver);
			saver = int_saver(sample_T, saver);

			////////////////////////////////
			// DMC Unit
			////////////////////////////////

			saver = bool_saver(irq_enabled_D, saver);
			saver = bool_saver(loop_flag_D, saver);
			saver = int_saver(timer_reload_D, saver);

			saver = int_saver(delay_D, saver);

			saver = bool_saver(sample_buffer_filled_D, saver);
			saver = bool_saver(out_silence_D, saver);
			saver = int_saver(timer_D, saver);
			saver = int_saver(user_address_D, saver);
			saver = int_saver(user_length_D, saver);
			saver = int_saver(sample_length_D, saver);
			saver = int_saver(sample_address_D, saver);
			saver = int_saver(sample_buffer_D, saver);
			saver = int_saver(out_shift_D, saver);
			saver = int_saver(out_bits_remaining_D, saver);
			saver = int_saver(out_deltacounter_D, saver);
					
			return saver;
		}

		uint8_t* LoadState(uint8_t* loader)
		{
			loader = bool_loader(&call_from_write, loader);
			loader = bool_loader(&recalculate, loader);
			loader = bool_loader(&irq_pending, loader);
			loader = bool_loader(&dmc_irq, loader);
			loader = bool_loader(&doing_tick_quarter, loader);
			loader = bool_loader(&len_clock_active, loader);
			loader = bool_loader(&sequencer_irq, loader);
			loader = bool_loader(&sequence_reset_pending, loader);
			loader = bool_loader(&sequencer_irq_clear_pending, loader);
			loader = bool_loader(&sequencer_irq_flag, loader);

			loader = byte_loader(&pending_val, loader);
			loader = byte_loader(&seq_val, loader);

			loader = sint_loader(&seq_tick, loader);
			loader = sint_loader(&m_vol, loader);
			loader = sint_loader(&dmc_dma_countdown, loader);
			loader = sint_loader(&sequencer_counter, loader);
			loader = sint_loader(&sequencer_step, loader);
			loader = sint_loader(&sequencer_mode, loader);
			loader = sint_loader(&sequencer_irq_inhibit, loader);
			loader = sint_loader(&sequencer_irq_assert, loader);
			loader = sint_loader(&pending_reg, loader);
			loader = sint_loader(&pending_length_change, loader);

			loader = sint_loader(&oldmix, loader);
			loader = sint_loader(&cart_sound, loader);
			loader = sint_loader(&old_cart_sound, loader);

			sampleclock = *loader; loader++; sampleclock |= ((uint64_t)*loader << 8); loader++;
			sampleclock |= ((uint64_t)*loader << 16); loader++; sampleclock |= ((uint64_t)*loader << 24); loader++;
			sampleclock |= ((uint64_t)*loader << 32); loader++; sampleclock |= ((uint64_t)*loader << 40); loader++;
			sampleclock |= ((uint64_t)*loader << 48); loader++; sampleclock |= ((uint64_t)*loader << 56); loader++;

			////////////////////////////////
			// Pulse Units
			////////////////////////////////

			loader = sint_loader(&unit_0, loader);
			loader = sint_loader(&unit_1, loader);

			// reg0
			loader = bool_loader(&len_halt_0, loader);
			loader = bool_loader(&len_halt_1, loader);
			loader = sint_loader(&duty_cnt_0, loader);
			loader = sint_loader(&duty_cnt_1, loader);
			loader = sint_loader(&env_loop_0, loader);
			loader = sint_loader(&env_loop_1, loader);
			loader = sint_loader(&env_constant_0, loader);
			loader = sint_loader(&env_constant_1, loader);
			loader = sint_loader(&env_cnt_value_0, loader);
			loader = sint_loader(&env_cnt_value_1, loader);

			// reg1
			loader = bool_loader(&sweep_reload_0, loader);
			loader = bool_loader(&sweep_reload_1, loader);
			loader = sint_loader(&sweep_en_0, loader);
			loader = sint_loader(&sweep_en_1, loader);
			loader = sint_loader(&sweep_divider_cnt_0, loader);
			loader = sint_loader(&sweep_divider_cnt_1, loader);
			loader = sint_loader(&sweep_negate_0, loader);
			loader = sint_loader(&sweep_negate_1, loader);
			loader = sint_loader(&sweep_shiftcount_0, loader);
			loader = sint_loader(&sweep_shiftcount_1, loader);

			// reg2/3
			loader = sint_loader(&len_cnt_0, loader);
			loader = sint_loader(&len_cnt_1, loader);
			loader = sint_loader(&timer_raw_reload_value_0, loader);
			loader = sint_loader(&timer_raw_reload_value_1, loader);
			loader = sint_loader(&timer_reload_value_0, loader);
			loader = sint_loader(&timer_reload_value_1, loader);

			// misc..
			loader = sint_loader(&lenctr_en_0, loader);
			loader = sint_loader(&lenctr_en_1, loader);

			// state
			loader = bool_loader(&swp_silence_0, loader);
			loader = bool_loader(&swp_silence_1, loader);
			loader = bool_loader(&duty_value_0, loader);
			loader = bool_loader(&duty_value_1, loader);
			loader = sint_loader(&swp_divider_counter_0, loader);
			loader = sint_loader(&swp_divider_counter_1, loader);
			loader = sint_loader(&duty_step_0, loader);
			loader = sint_loader(&duty_step_1, loader);
			loader = sint_loader(&timer_counter_0, loader);
			loader = sint_loader(&timer_counter_1, loader);
			loader = sint_loader(&sample_0, loader);
			loader = sint_loader(&sample_1, loader);
			loader = sint_loader(&env_start_flag_0, loader);
			loader = sint_loader(&env_start_flag_1, loader);
			loader = sint_loader(&env_divider_0, loader);
			loader = sint_loader(&env_divider_1, loader);
			loader = sint_loader(&env_counter_0, loader);
			loader = sint_loader(&env_counter_1, loader);
			loader = sint_loader(&env_output_0, loader);
			loader = sint_loader(&env_output_1, loader);

			////////////////////////////////
			// Noise Unit
			////////////////////////////////

			// reg0 (sweep)
			loader = bool_loader(&len_halt_N, loader);
			loader = sint_loader(&env_cnt_value_N, loader);
			loader = sint_loader(&env_loop_N, loader);
			loader = sint_loader(&env_constant_N, loader);

			// reg2 (mode and period)
			loader = sint_loader(&mode_cnt_N, loader);
			loader = sint_loader(&period_cnt_N, loader);

			// reg3 (length counter and envelop trigger)
			loader = sint_loader(&len_cnt_N, loader);

			// set from apu:
			loader = sint_loader(&lenctr_en_N, loader);

			// state
			loader = bool_loader(&noise_bit_N, loader);
			loader = sint_loader(&shift_register_N, loader);
			loader = sint_loader(&timer_counter_N, loader);
			loader = sint_loader(&sample_N, loader);
			loader = sint_loader(&env_output_N, loader);
			loader = sint_loader(&env_start_flag_N, loader);
			loader = sint_loader(&env_divider_N, loader);
			loader = sint_loader(&env_counter_N, loader);

			////////////////////////////////
			// Triangle Unit
			////////////////////////////////

			// reg0
			loader = sint_loader(&linear_counter_reload_T, loader);
			loader = sint_loader(&control_flag_T, loader);

			// reg1 (n/a)
			// reg2/3
			loader = bool_loader(&halt_2_T, loader);
			loader = sint_loader(&timer_cnt_T, loader);
			loader = sint_loader(&reload_flag_T, loader);
			loader = sint_loader(&len_cnt_T, loader);

			// misc..
			loader = sint_loader(&lenctr_en_T, loader);
			loader = sint_loader(&linear_counter_T, loader);
			loader = sint_loader(&timer_T, loader);
			loader = sint_loader(&timer_cnt_reload_T, loader);
			loader = sint_loader(&seq_T, loader);
			loader = sint_loader(&sample_T, loader);

			////////////////////////////////
			// DMC Unit
			////////////////////////////////

			loader = bool_loader(&irq_enabled_D, loader);
			loader = bool_loader(&loop_flag_D, loader);
			loader = sint_loader(&timer_reload_D, loader);

			loader = sint_loader(&delay_D, loader);

			loader = bool_loader(&sample_buffer_filled_D, loader);
			loader = bool_loader(&out_silence_D, loader);
			loader = sint_loader(&timer_D, loader);
			loader = sint_loader(&user_address_D, loader);
			loader = sint_loader(&user_length_D, loader);
			loader = sint_loader(&sample_length_D, loader);
			loader = sint_loader(&sample_address_D, loader);
			loader = sint_loader(&sample_buffer_D, loader);
			loader = sint_loader(&out_shift_D, loader);
			loader = sint_loader(&out_bits_remaining_D, loader);
			loader = sint_loader(&out_deltacounter_D, loader);

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

		#pragma endregion
	};
}