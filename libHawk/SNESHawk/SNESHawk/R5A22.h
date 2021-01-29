#include <iostream>
#include <cstdint>
#include <iomanip>
#include <string>

using namespace std;

namespace SNESHawk
{
	class MemoryManager;

	// Method of Operation: It can take up to 12 master clock sycles for an instruction cycle to execute. So all instruction cycles are assumed to be 
	// 12 master cycles long, and then some are skipped based on the check done on the second master cycle of each instruction cycle.
	// No instruction cycles are run during memory refresh
	// DMA is also handled here
	
	class R5A22
	{
	public:
		#pragma region Variable Declarations

		// pointer to controlling memory manager
		MemoryManager* mem_ctrl;

		void WriteMemory(uint32_t, uint8_t);
		uint8_t ReadMemory(uint32_t);
		uint8_t DummyReadMemory(uint32_t);
		uint8_t PeekMemory(uint32_t);

		// variables for building the instruction table
		int op;
		int op_length = 12;
		int num_uops = 0;

		// State variables


		// variables for operations
		bool EM; // emulation mode
		bool branch_taken;
		bool my_iflag;
		bool booltemp;
		bool BCD_Enabled;
		uint8_t value8, temp8;
		uint16_t value16;
		int32_t tempint;
		int32_t lo, hi;

		// CPU
		uint8_t P;
		uint16_t S;
		uint16_t A;
		uint16_t X;
		uint16_t Y;
		uint16_t XY_mask;
		uint16_t PC;
		uint16_t D;
		uint32_t DBR;
		uint32_t PBR;

		bool RDY;
		bool NMI;
		bool IRQ;
		bool iflag_pending; //iflag must be stored after it is checked in some cases (CLI and SEI).
		bool rdy_freeze; //true if the CPU must be frozen
		//tracks whether an interrupt condition has popped up recently.
		//not sure if this is real or not but it helps with the branch_irq_hack
		bool interrupt_pending;
		bool branch_irq_hack; //see Uop.RelBranch_Stage3 for more details		
		uint8_t opcode2, opcode3, opcode4;
		int32_t opcode;
		int32_t ea, alu_temp; //cpu internal temp variables
		int32_t mi; //microcode index
		int32_t LY; // refernced from PPU to see what the scanline is
		uint64_t TotalExecutedCycles;

		#pragma endregion

		#pragma region Constant Declarations
		
		const uint16_t NMIVector = 0xFFFA;
		const uint16_t ResetVector = 0xFFFC;
		const uint16_t BRKVector = 0xFFFE;
		const uint16_t BRKVectorNative = 0xFFE6;
		const uint16_t COPVector = 0xFFFE;
		const uint16_t IRQVector = 0xFFFE;
		
		// operations that can take place in an instruction
		enum Uop
		{
			Fetch1, Fetch1_Real, Fetch2, Fetch3, Fetch4,
			//used by instructions with no second opcode byte (6502 fetches a byte anyway but won't increment PC for these)
			FetchDummy,

			NOP,

			// check 16 bit operations
			MMT, XYT,

			// check memory access time
			CHP, CHE, CHS, SKP,

			// basic operations
			End_LDA, End_LDX, End_LDY, End_ORA, End_CMP, End_CPX, End_CPY, End_ADC, End_AND, End_EOR, End_SBC, End_BIT,

			// basic read and write operations
			DBR_EA_READ, DBR_EA_READa,
			DBR_EA_STA, DBR_EA_STAa,
			DBR_EA_STY, DBR_EA_STYa,
			DBR_EA_STX, DBR_EA_STXa,
			DBR_EA_STZ, DBR_EA_STZa,

			JSR,
			IncPC, //from RTS

			//[absolute RMW]
			Abs_RMW_Stage4, Abs_RMW_Stage6,
			Abs_RMW_Stage5_INC, Abs_RMW_Stage5_DEC, Abs_RMW_Stage5_LSR, Abs_RMW_Stage5_ROL, Abs_RMW_Stage5_ASL, Abs_RMW_Stage5_ROR,

			//[absolute JUMP]
			JMP_abs, JSL,

			//[long absolute BR]
			BRL,

			//[zero page misc]
			ZpIdx_Stage3_X,
			ZpIdx_RMW_Stage4, ZpIdx_RMW_Stage6,

			//[zero page RMW]
			ZP_RMW_Stage3, ZP_RMW_Stage5,
			ZP_RMW_DEC, ZP_RMW_INC, ZP_RMW_ASL, ZP_RMW_LSR, ZP_RMW_ROR, ZP_RMW_ROL,

			// d
			dir_READ, dir_READa,
			dir_STA, dir_STAa,
			dir_STX, dir_STXa,
			dir_STY, dir_STYa,
			dir_STZ, dir_STZa,

			// (d,x)
			IdxInd_Stage3, IdxInd_Stage3a, IdxInd_Stage4, IdxInd_Stage5,
			IdxInd_Stage6_READ, IdxInd_Stage6_READa,
			IdxInd_Stage6_STA, IdxInd_Stage6_STAa,

			// (d),y
			IndIdx_Stage3, IndIdx_Stage3a,
			IndIdx_Stage4, IndIdx_Stage4a,
			IndIdx_Stage5, IndIdx_Stage5a,
			IndIdx_Stage5_STA, IndIdx_Stage5_STAa,

			// (d)
			Ind_Stage3, Ind_Stage3a,
			Ind_Stage4,
			Ind_Stage5, Ind_Stage5a,
			Ind_Stage5_STA, Ind_Stage5_STAa,

			// d,s and (d,s),y
			StackEA_1, StackEA_2,
			StkRelInd3_READ, StkRelInd3_READa,
			StkRelInd5_READ, StkRelInd5_READa,
			StkRelInd5_STA, StkRelInd5_STAa,

			// d,x snd d,y
			EA_DX, EA_DY,

			// imm #
			Imm_READ, Imm_READa,

			// a and a,x and a,y		
			Abs_Fetch_EA, Abs_Fetch_EA_Y, Abs_Fetch_EA_X,

			// al and al,x
			Absl_Fetch_EA, Abslx_Fetch_EA,
			Absl_READ, Absl_READa,
			Absl_STA, Absl_STAa,

			//[absolute indexed]
			AbsIdx_Stage3_X, AbsIdx_Stage4,
			//[absolute indexed RMW]
			AbsIdx_RMW_Stage5, AbsIdx_RMW_Stage7,
			AbsIdx_RMW_Stage6_ROR, AbsIdx_RMW_Stage6_DEC, AbsIdx_RMW_Stage6_INC, AbsIdx_RMW_Stage6_ASL, AbsIdx_RMW_Stage6_LSR, AbsIdx_RMW_Stage6_ROL,

			IncS, DecS,
			PushPCL, PushPCH, PushPBR, PushPBR_BRK, PushPERH, PushPERL, PushP, PullP, PullPCL, PullPCH_NoInc, PullA_NoInc, PullP_NoInc,
			PushA, PushB, PushK, PushDH, PushDL, PushYH, PushYL, PushXH, PushXL,
			PushP_BRK, PushP_COP, PushP_NMI, PushP_IRQ, PushP_Reset, PushDummy,
			FetchPCLVector, FetchPCHVector, //todo - may not need these ?? can reuse fetch2 and fetch3?

			//[implied] and [accumulator]
			Imp_ASL_A, Imp_ROL_A, Imp_ROR_A, Imp_LSR_A,
			Imp_SEC, Imp_CLI, Imp_SEI, Imp_CLD, Imp_CLC, Imp_CLV, Imp_SED,
			Imp_INY, Imp_DEY, Imp_INX, Imp_DEX,
			Imp_TSX, Imp_TXS, Imp_TAX, Imp_TAY, Imp_TYA, Imp_TXA,

			//sub-ops
			NZ_X, NZ_Y, NZ_A,
			RelBranch_Stage2_BNE, RelBranch_Stage2_BPL, RelBranch_Stage2_BCC, RelBranch_Stage2_BCS, RelBranch_Stage2_BEQ, RelBranch_Stage2_BMI, RelBranch_Stage2_BVC, RelBranch_Stage2_BVS, RelBranch_Stage2_A,
			RelBranch_Stage2, RelBranch_Stage3, RelBranch_Stage4,
			_Eor, _Bit, _Cpx, _Cpy, _Cmp, _Adc, _Sbc, _Ora, _And, _Anc, _Asr, _Arr, _Axs, //alu-related sub-ops

			//JMP (addr) 0x6C
			AbsInd_JMP_Stage4, AbsInd_JMP_Stage5,

			DirectEA_RD,

			REP, SEP, TSB, TRB,

			End,
			End_ISpecial, //same as end, but preserves the iflag set by the instruction
			End_BranchSpecial,
			End_NoInt,

			// NOTE: jump to 256 here
			VOP_Fetch1 = 256,
			VOP_RelativeStuff,
			VOP_RelativeStuff2,
			VOP_RelativeStuff3,
			VOP_NMI,
			VOP_IRQ,
			VOP_RESET,
			VOP_Fetch1_NoInterrupt,
			VOP_NUM
		};

		// instruction table
		Uop Microcode[(256 + 8) * 12 * 12];

		uint8_t TableNZ[256] =
		{
			0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
		};

		#pragma endregion

		#pragma region R5A22 functions

		inline bool FlagCget() { return (P & 0x01) != 0; };
		inline void FlagCset(bool value) { P = (uint8_t)((P & ~0x01) | (value ? 0x01 : 0x00)); }

		inline bool FlagZget() { return (P & 0x02) != 0; };
		inline void FlagZset(bool value) { P = (uint8_t)((P & ~0x02) | (value ? 0x02 : 0x00)); }

		inline bool FlagIget() { return (P & 0x04) != 0; };
		inline void FlagIset(bool value) { P = (uint8_t)((P & ~0x04) | (value ? 0x04 : 0x00)); }

		inline bool FlagDget() { return (P & 0x08) != 0; };
		inline void FlagDset(bool value) { P = (uint8_t)((P & ~0x08) | (value ? 0x08 : 0x00)); }

		inline bool FlagXget() { return (P & 0x10) != 0; };
		inline void FlagXset(bool value) { P = (uint8_t)((P & ~0x10) | (value ? 0x10 : 0x00)); }

		inline bool FlagMget() { return (P & 0x20) != 0; };
		inline void FlagMset(bool value) { P = (uint8_t)((P & ~0x20) | (value ? 0x20 : 0x00)); }

		inline bool FlagVget() { return (P & 0x40) != 0; };
		inline void FlagVset(bool value) { P = (uint8_t)((P & ~0x40) | (value ? 0x40 : 0x00)); }

		inline bool FlagNget() { return (P & 0x80) != 0; };
		inline void FlagNset(bool value) { P = (uint8_t)((P & ~0x80) | (value ? 0x80 : 0x00)); }

		inline uint32_t RegPCget() { return (uint32_t)PC; }
		inline void RegPCset(uint32_t value) { PC = (uint16_t)value; }

		inline bool Interrupted(void) { return (RDY && (NMI || (IRQ && !FlagIget()))); }

		// SO pin
		void SetOverflow()
		{
			FlagVset(true);
		}

		R5A22()
		{
			Reset();
		}

		void Reset()
		{
			A = 0;
			X = 0;
			Y = 0;
			P = 0x34;
			S = 0x100;
			PC = 0;
			D = 0;
			DBR = 0;
			PBR = 0;
			TotalExecutedCycles = 0;
			mi = 0;
			opcode = VOP_RESET;
			iflag_pending = true;
			RDY = true;
			EM = true;
		}

		void ExecuteOne()
		{
			// total cycles now increments every time a cycle is called to accurately count during RDY
			TotalExecutedCycles++;
			interrupt_pending |= Interrupted();
			rdy_freeze = false;

			//i tried making ExecuteOneRetry not re-entrant by having it set a flag instead, then exit from the call below, check the flag, and GOTO if it was flagged, but it wasnt faster
			ExecuteOneRetry();

			if (!rdy_freeze)
				mi++;
		}

		// Execute instructions
		void ExecuteOneRetry()
		{
			switch (Microcode[opcode * 12 + mi])
			{
			case Fetch1: Fetch1_F(); break;
			case Fetch1_Real: Fetch1_Real_F(); break;
			case Fetch2: Fetch2_F(); break;
			case Fetch3: Fetch3_F(); break;
			case Fetch4: Fetch4_F(); break;
			case FetchDummy: FetchDummy_F(); break;
			case PushPCH: PushPCH_F(); break;
			case PushPCL: PushPCL_F(); break;
			case PushPBR: PushPBR_F(); break;
			case PushPBR_BRK: PushPBR_BRK_F(); break;
			case PushPERH: PushPERH_F(); break;
			case PushPERL: PushPERL_F(); break;
			case PushP_BRK: PushP_BRK_F(); break;
			case PushP_COP: PushP_COP_F(); break;
			case PushP_IRQ: PushP_IRQ_F(); break;
			case PushP_NMI: PushP_NMI_F(); break;
			case PushP_Reset: PushP_Reset_F(); break;
			case PushDummy: PushDummy_F(); break;
			case FetchPCLVector: FetchPCLVector_F(); break;
			case FetchPCHVector: FetchPCHVector_F(); break;
			case Imp_INY: Imp_INY_F(); break;
			case Imp_DEY: Imp_DEY_F(); break;
			case Imp_INX: Imp_INX_F(); break;
			case Imp_DEX: Imp_DEX_F(); break;
			case NZ_A: NZ_A_F(); break;
			case NZ_X: NZ_X_F(); break;
			case NZ_Y: NZ_Y_F(); break;
			case Imp_TSX: Imp_TSX_F(); break;
			case Imp_TXS: Imp_TXS_F(); break;
			case Imp_TAX: Imp_TAX_F(); break;
			case Imp_TAY: Imp_TAY_F(); break;
			case Imp_TYA: Imp_TYA_F(); break;
			case Imp_TXA: Imp_TXA_F(); break;
			case Imp_SEI: Imp_SEI_F(); break;
			case Imp_CLI: Imp_CLI_F(); break;
			case Imp_SEC: Imp_SEC_F(); break;
			case Imp_CLC: Imp_CLC_F(); break;
			case Imp_SED: Imp_SED_F(); break;
			case Imp_CLD: Imp_CLD_F(); break;
			case Imp_CLV: Imp_CLV_F(); break;
			case IndIdx_Stage3: IndIdx_Stage3_F(); break;
			case IndIdx_Stage3a: IndIdx_Stage3a_F(); break;
			case IndIdx_Stage4: IndIdx_Stage4_F(); break;
			case IndIdx_Stage4a: IndIdx_Stage4a_F(); break;
			case IndIdx_Stage5: IndIdx_Stage5_F(); break;
			case IndIdx_Stage5a: IndIdx_Stage5a_F(); break;
			case IndIdx_Stage5_STA: IndIdx_Stage5_STA_F(); break;
			case IndIdx_Stage5_STAa: IndIdx_Stage5_STAa_F(); break;
			case Ind_Stage3: Ind_Stage3_F(); break;
			case Ind_Stage3a: Ind_Stage3a_F(); break;
			case Ind_Stage4: Ind_Stage4_F(); break;
			case Ind_Stage5: Ind_Stage5_F(); break;
			case Ind_Stage5a: Ind_Stage5a_F(); break;
			case Ind_Stage5_STA: Ind_Stage5_STA_F(); break;
			case Ind_Stage5_STAa: Ind_Stage5_STAa_F(); break;
			case RelBranch_Stage2_BVS: RelBranch_Stage2_BVS_F(); break;
			case RelBranch_Stage2_BVC: RelBranch_Stage2_BVC_F(); break;
			case RelBranch_Stage2_BMI: RelBranch_Stage2_BMI_F(); break;
			case RelBranch_Stage2_BPL: RelBranch_Stage2_BPL_F(); break;
			case RelBranch_Stage2_BCS: RelBranch_Stage2_BCS_F(); break;
			case RelBranch_Stage2_BCC: RelBranch_Stage2_BCC_F(); break;
			case RelBranch_Stage2_BEQ: RelBranch_Stage2_BEQ_F(); break;
			case RelBranch_Stage2_BNE: RelBranch_Stage2_BNE_F(); break;
			case RelBranch_Stage2_A: RelBranch_Stage2_A_F(); break;
			case RelBranch_Stage2: RelBranch_Stage2_F(); break;
			case RelBranch_Stage3: RelBranch_Stage3_F(); break;
			case RelBranch_Stage4: RelBranch_Stage4_F(); break;
			case NOP: NOP_F(); break;
			case MMT: MMT_F(); break;
			case XYT: XYT_F(); break;
			case CHP: CHP_F(); break;
			case CHE: CHE_F(); break;
			case CHS: CHS_F(); break;
			case SKP: SKP_F(); break;
			case DecS: DecS_F(); break;
			case IncS: IncS_F(); break;
			case JSR: JSR_F(); break;
			case PullP: PullP_F(); break;
			case PullPCL: PullPCL_F(); break;
			case PullPCH_NoInc: PullPCH_NoInc_F(); break;
			case ZpIdx_Stage3_X: ZpIdx_Stage3_X_F(); break;
			case ZpIdx_RMW_Stage4: ZpIdx_RMW_Stage4_F(); break;
			case ZpIdx_RMW_Stage6: ZpIdx_RMW_Stage6_F(); break;
			case dir_READ: dir_READ_F();
			case dir_READa: dir_READa_F();
			case dir_STA: dir_STA_F();
			case dir_STAa: dir_STAa_F();
			case dir_STX: dir_STX_F();
			case dir_STXa: dir_STXa_F();
			case dir_STY: dir_STY_F();
			case dir_STYa: dir_STYa_F();
			case dir_STZ: dir_STZ_F();
			case dir_STZa: dir_STZa_F();
			case _Cpx: _Cpx_F(); break;
			case _Cpy: _Cpy_F(); break;
			case _Cmp: _Cmp_F(); break;
			case _Eor: _Eor_F(); break;
			case _And: _And_F(); break;
			case _Ora: _Ora_F(); break;
			case _Anc: _Anc_F(); break;
			case _Asr: _Asr_F(); break;
			case _Axs: _Axs_F(); break;
			case _Arr: _Arr_F(); break;
			case _Sbc: _Sbc_F(); break;
			case _Adc: _Adc_F(); break;
			case IdxInd_Stage3: IdxInd_Stage3_F(); break;
			case IdxInd_Stage3a: IdxInd_Stage3a_F(); break;
			case IdxInd_Stage4: IdxInd_Stage4_F(); break;
			case IdxInd_Stage5: IdxInd_Stage5_F(); break;
			case IdxInd_Stage6_READ: IdxInd_Stage6_READ_F(); break;
			case IdxInd_Stage6_READa: IdxInd_Stage6_READa_F(); break;
			case IdxInd_Stage6_STA: IdxInd_Stage6_STA_F(); break;
			case IdxInd_Stage6_STAa: IdxInd_Stage6_STAa_F(); break;
			case PushP: PushP_F(); break;
			case PushA: PushA_F(); break;
			case PullA_NoInc: PullA_NoInc_F(); break;
			case PullP_NoInc: PullP_NoInc_F(); break;
			case Imp_ASL_A: Imp_ASL_A_F(); break;
			case Imp_ROL_A: Imp_ROL_A_F(); break;
			case Imp_ROR_A: Imp_ROR_A_F(); break;
			case Imp_LSR_A: Imp_LSR_A_F(); break;
			case JMP_abs: JMP_abs_F(); break;
			case JSL: JSL_F(); break;
			case BRL: BRL_F(); break;
			case IncPC: IncPC_F(); break;
			case ZP_RMW_Stage3: ZP_RMW_Stage3_F(); break;
			case ZP_RMW_Stage5: ZP_RMW_Stage5_F(); break;
			case ZP_RMW_INC: ZP_RMW_INC_F(); break;
			case ZP_RMW_DEC: ZP_RMW_DEC_F(); break;
			case ZP_RMW_ASL: ZP_RMW_ASL_F(); break;
			case ZP_RMW_LSR: ZP_RMW_LSR_F(); break;
			case ZP_RMW_ROR: ZP_RMW_ROR_F(); break;
			case ZP_RMW_ROL: ZP_RMW_ROL_F(); break;
			case AbsIdx_Stage3_X: AbsIdx_Stage3_X_F(); break;
			case AbsIdx_Stage4: AbsIdx_Stage4_F(); break;
			case AbsIdx_RMW_Stage5: AbsIdx_RMW_Stage5_F(); break;
			case AbsIdx_RMW_Stage7: AbsIdx_RMW_Stage7_F(); break;
			case AbsIdx_RMW_Stage6_DEC: AbsIdx_RMW_Stage6_DEC_F(); break;
			case AbsIdx_RMW_Stage6_INC: AbsIdx_RMW_Stage6_INC_F(); break;
			case AbsIdx_RMW_Stage6_ROL: AbsIdx_RMW_Stage6_ROL_F(); break;
			case AbsIdx_RMW_Stage6_LSR: AbsIdx_RMW_Stage6_LSR_F(); break;
			case AbsIdx_RMW_Stage6_ASL: AbsIdx_RMW_Stage6_ASL_F(); break;
			case AbsIdx_RMW_Stage6_ROR: AbsIdx_RMW_Stage6_ROR_F(); break;
			case AbsInd_JMP_Stage4: AbsInd_JMP_Stage4_F(); break;
			case AbsInd_JMP_Stage5: AbsInd_JMP_Stage5_F(); break;
			case Abs_RMW_Stage4: Abs_RMW_Stage4_F(); break;
			case Abs_RMW_Stage5_INC: Abs_RMW_Stage5_INC_F(); break;
			case Abs_RMW_Stage5_DEC: Abs_RMW_Stage5_DEC_F(); break;
			case Abs_RMW_Stage5_ASL: Abs_RMW_Stage5_ASL_F(); break;
			case Abs_RMW_Stage5_ROR: Abs_RMW_Stage5_ROR_F(); break;
			case Abs_RMW_Stage5_ROL: Abs_RMW_Stage5_ROL_F(); break;
			case Abs_RMW_Stage5_LSR: Abs_RMW_Stage5_LSR_F(); break;
			case Abs_RMW_Stage6: Abs_RMW_Stage6_F(); break;
			case StackEA_1: StackEA_1_F(); break;
			case StkRelInd3_READ: StkRelInd3_READ_F(); break;
			case StkRelInd3_READa: StkRelInd3_READa_F(); break;
			case StkRelInd5_READ: StkRelInd5_READ_F(); break;
			case StkRelInd5_READa: StkRelInd5_READa_F(); break;
			case StkRelInd5_STA: StkRelInd5_STA_F(); break;
			case StkRelInd5_STAa: StkRelInd5_STAa_F(); break;
			case EA_DX: EA_DX_F(); break;
			case EA_DY: EA_DY_F(); break;
			case Imm_READ: Imm_READ_F(); break;
			case Imm_READa: Imm_READa_F(); break;
			case Abs_Fetch_EA: Abs_Fetch_EA_F(); break;
			case Abs_Fetch_EA_Y: Abs_Fetch_EA_Y_F(); break;
			case Abs_Fetch_EA_X: Abs_Fetch_EA_X_F(); break;
			case Absl_Fetch_EA: Absl_Fetch_EA_F(); break;
			case Abslx_Fetch_EA: Abslx_Fetch_EA_F(); break;
			case Absl_READ: Absl_READ_F(); break;
			case Absl_READa: Absl_READa_F(); break;
			case Absl_STA: Absl_STA_F(); break;
			case Absl_STAa: Absl_STAa_F(); break;
			case StackEA_2: StackEA_2_F(); break;
			case DirectEA_RD: DirectEA_RD_F(); break;
			case REP: REP_F(); break;
			case SEP: SEP_F(); break;
			case TSB: TSB_F(); break;
			case TRB: TRB_F(); break;
			case End_ISpecial: End_ISpecial_F(); break;
			case End_NoInt: End_NoInt_F(); break;
			case End: End_F(); break;
			case End_LDA: End_LDA_F(); break;
			case End_LDX: End_LDX_F(); break;
			case End_LDY: End_LDY_F(); break;
			case End_ORA: End_ORA_F(); break;
			case End_CMP: End_CMP_F(); break;
			case End_CPX: End_CPX_F(); break;
			case End_CPY: End_CPY_F(); break;
			case End_ADC: End_ADC_F(); break;
			case End_AND: End_AND_F(); break;
			case End_EOR: End_EOR_F(); break;
			case End_SBC: End_SBC_F(); break;
			case End_BIT: End_BIT_F(); break;
			case DBR_EA_READ: DBR_EA_READ_F(); break;
			case DBR_EA_READa: DBR_EA_READa_F(); break;
			case DBR_EA_STA: DBR_EA_STA_F(); break;
			case DBR_EA_STAa: DBR_EA_STAa_F(); break;
			case DBR_EA_STY: DBR_EA_STY_F(); break;
			case DBR_EA_STYa: DBR_EA_STYa_F(); break;
			case DBR_EA_STX: DBR_EA_STX_F(); break;
			case DBR_EA_STXa: DBR_EA_STXa_F(); break;
			case DBR_EA_STZ: DBR_EA_STZ_F(); break;
			case DBR_EA_STZa: DBR_EA_STZa_F(); break;

			case End_BranchSpecial: End_BranchSpecial_F(); break;
			}
		}

		#pragma endregion

		#pragma region operations
		
		void FetchDummy_F() { DummyReadMemory(PBR | PC); }

		void Execute(int cycles)
		{
			for (int i = 0; i < cycles; i++)
			{
				ExecuteOne();
			}
		}

		void Fetch1_F()
		{
			my_iflag = FlagIget();
			FlagIset(iflag_pending);
			if (!branch_irq_hack)
			{
				interrupt_pending = false;
				if (NMI)
				{
					if (TraceCallback) { TraceCallback(2); }

					ea = NMIVector;
					opcode = VOP_NMI;
					NMI = false;
					mi = 0;
					ExecuteOneRetry();
					return;
				}

				if (IRQ && !my_iflag)
				{
					if (TraceCallback) { TraceCallback(1); }
					ea = IRQVector;
					opcode = VOP_IRQ;
					mi = 0;
					ExecuteOneRetry();
					return;
				}
			}

			Fetch1_Real_F();
		}

		void Fetch1_Real_F()
		{
			branch_irq_hack = false;
			//OnExecFetch(PC);
			if (TraceCallback) { TraceCallback(0); }
			opcode = ReadMemory(PBR | PC);
			PC++;
			mi = -1;
		}

		void Fetch2_F() { opcode2 = ReadMemory(PBR | PC); PC++; }

		void Fetch3_F() { opcode3 = ReadMemory(PBR | PC); PC++; }

		void Fetch4_F() { opcode4 = ReadMemory(PBR | PC); PC++; }

		void PushPCH_F() 
		{ 
			WriteMemory(S, (uint8_t)(PC >> 8)); 
			S--;
			if (EM) { S &= 0xFF; S |= 0x0100; }
		}

		void PushPCL_F() 
		{ 
			WriteMemory(S, (uint8_t)PC); 
			S--;
			if (EM) { S &= 0xFF; S |= 0x0100; }
		}

		void PushPBR_F() 
		{ 
			WriteMemory(S, (uint8_t)(PBR >> 16)); 
			S--;
			if (EM) { S &= 0xFF; S |= 0x0100; }
		}

		// for BRK and COP, skip pushing PBR if in emulation mode
		void PushPBR_BRK_F()
		{
			if (EM)
			{
				WriteMemory(S, (uint8_t)(PC >> 8));
				S--;
				S &= 0xFF; S |= 0x0100;
				mi++;
			}
			else 
			{
				WriteMemory(S, (uint8_t)(PBR >> 16));
				S--;
			}
		}

		void PushPERH_F() 
		{ 
			alu_temp = (uint8_t)((PC >> 8) + opcode2 + (FlagCget() ? 1 : 0));
			WriteMemory((uint16_t)(S-- + 0x100), alu_temp); 
		}

		void PushPERL_F() 
		{
			WriteMemory((uint16_t)(S-- + 0x100), (uint8_t)(PC + opcode3)); 
		}

		void PushP_BRK_F()
		{
			if (EM) 
			{
				// bit 4 will always be 1 anyway, so nothing to do
				WriteMemory(S, P);
				S--;
				S &= 0xFF; S |= 0x0100;
				FlagIset(true);
				ea = BRKVector;

				//in emulation mode only, DBR is set to 0
				DBR = 0;
			}
			else 
			{
				// bit 4 not effected
				WriteMemory(S, P);
				S--;
				FlagIset(true);
				ea = BRKVectorNative;
			}

			// always set PBR to zero
			PBR = 0;
		}

		void PushP_COP_F()
		{
			if (EM) 
			{
				WriteMemory(S, P);
				S--;
				S &= 0xFF; S |= 0x0100;
				FlagIset(true);
				ea = COPVector;

				//in emulation mode only, DBR is set to 0
				DBR = 0;
			}
			else 
			{
				WriteMemory(S, P);
				S--;
				FlagIset(true);
				ea = COPVector;
			}
			
			// always set PBR to zero
			PBR = 0;
		}

		void PushP_IRQ_F()
		{
			//FlagBset(false);
			WriteMemory((uint16_t)(S-- + 0x100), P);
			FlagIset(true);
			ea = IRQVector;
		}

		void PushP_NMI_F()
		{
			//FlagBset(false);
			WriteMemory((uint16_t)(S-- + 0x100), P);
			FlagIset(true); //is this right?
			ea = NMIVector;
		}

		void PushP_Reset_F()
		{
			// EM will always be true in reset
			ea = ResetVector;
			DummyReadMemory(S);
			S--;
			S &= 0xFF; S |= 0x0100;
			FlagIset(true);
		}

		void PushDummy_F() { DummyReadMemory((uint16_t)(S-- + 0x100)); }

		void FetchPCLVector_F()
		{
			if (ea == BRKVector && /*FlagBget() &&*/ NMI)
			{
				NMI = false;
				ea = NMIVector;
			}
			if (ea == IRQVector && /*!FlagBget() &&*/ NMI)
			{
				NMI = false;
				ea = NMIVector;
			}
			alu_temp = ReadMemory((uint16_t)ea);
		}

		void FetchPCHVector_F()
		{
			alu_temp += ReadMemory((uint16_t)(ea + 1)) << 8;
			PC = (uint16_t)alu_temp;
		}

		void Imp_INY_F() { DummyReadMemory(PBR | PC); Y++; NZ_Y_F(); }

		void Imp_DEY_F() { DummyReadMemory(PBR | PC); Y--; NZ_Y_F(); }

		void Imp_INX_F() { DummyReadMemory(PBR | PC); X++; NZ_X_F(); }

		void Imp_DEX_F() { DummyReadMemory(PBR | PC); X--; NZ_X_F(); }

		void NZ_A_F() { P = (uint8_t)((P & 0x7D) | TableNZ[A]); }

		void NZ_X_F() { P = (uint8_t)((P & 0x7D) | TableNZ[X]); }

		void NZ_Y_F() { P = (uint8_t)((P & 0x7D) | TableNZ[Y]); }

		void Imp_TSX_F() { DummyReadMemory(PBR | PC); X = S; NZ_X_F(); }

		void Imp_TXS_F() { DummyReadMemory(PBR | PC); S = X; }

		void Imp_TAX_F() { DummyReadMemory(PBR | PC); X = A; NZ_X_F(); }

		void Imp_TAY_F() { DummyReadMemory(PBR | PC); Y = A; NZ_Y_F(); }

		void Imp_TYA_F() { DummyReadMemory(PBR | PC); A = Y; NZ_A_F(); }

		void Imp_TXA_F() { DummyReadMemory(PBR | PC); A = X; NZ_A_F(); }

		void Imp_SEI_F() { DummyReadMemory(PBR | PC); iflag_pending = true; }

		void Imp_CLI_F() { DummyReadMemory(PBR | PC); iflag_pending = false; }

		void Imp_SEC_F() { DummyReadMemory(PBR | PC); FlagCset(true); }

		void Imp_CLC_F() { DummyReadMemory(PBR | PC); FlagCset(false); }

		void Imp_SED_F() { DummyReadMemory(PBR | PC); FlagDset(true); }

		void Imp_CLD_F() { DummyReadMemory(PBR | PC); FlagDset(false); }

		void Imp_CLV_F() { DummyReadMemory(PBR | PC); FlagVset(false); }

		void IndIdx_Stage3_F() {  }
		void IndIdx_Stage3a_F() {  }
		void IndIdx_Stage4_F() { }
		void IndIdx_Stage4a_F() { }
		void IndIdx_Stage5_F() { }
		void IndIdx_Stage5a_F() { }
		void IndIdx_Stage5_STA_F() { }
		void IndIdx_Stage5_STAa_F() { }

		void Ind_Stage3_F() {  }
		void Ind_Stage3a_F() {  }
		void Ind_Stage4_F() { }
		void Ind_Stage5_F() { }
		void Ind_Stage5a_F() { }
		void Ind_Stage5_STA_F() { }
		void Ind_Stage5_STAa_F() { }

		void RelBranch_Stage2_BVS_F()
		{
			branch_taken = FlagVget() == true;
			RelBranch_Stage2_F();
		}

		void RelBranch_Stage2_BVC_F()
		{
			branch_taken = FlagVget() == false;
			RelBranch_Stage2_F();
		}

		void RelBranch_Stage2_BMI_F()
		{
			branch_taken = FlagNget() == true;
			RelBranch_Stage2_F();
		}

		void RelBranch_Stage2_BPL_F()
		{
			branch_taken = FlagNget() == false;
			RelBranch_Stage2_F();
		}

		void RelBranch_Stage2_BCS_F()
		{
			branch_taken = FlagCget() == true;
			RelBranch_Stage2_F();
		}

		void RelBranch_Stage2_BCC_F()
		{
			branch_taken = FlagCget() == false;
			RelBranch_Stage2_F();
		}

		void RelBranch_Stage2_BEQ_F()
		{
			branch_taken = FlagZget() == true;
			RelBranch_Stage2_F();
		}

		void RelBranch_Stage2_BNE_F()
		{
			branch_taken = FlagZget() == false;
			RelBranch_Stage2_F();
		}

		void RelBranch_Stage2_A_F()
		{
			branch_taken = true;
			RelBranch_Stage2_F();
		}

		void RelBranch_Stage2_F()
		{
			opcode2 = ReadMemory(PBR | PC);
			PC++;
			if (branch_taken)
			{
				branch_taken = false;
				//if the branch is taken, we enter a different bit of microcode to calculate the PC and complete the branch
				opcode = VOP_RelativeStuff;
				mi = -1;
			}
		}

		void RelBranch_Stage3_F()
		{
			DummyReadMemory(PBR | PC);
			alu_temp = (uint8_t)PC + (int)(int8_t)opcode2;
			PC &= 0xFF00;
			PC |= (uint16_t)((alu_temp & 0xFF));
			if (((alu_temp & 0x100) == 0x100))
			{
				//we need to carry the add, and then we'll be ready to fetch the next instruction
				opcode = VOP_RelativeStuff2;
				mi = -1;
			}
			else
			{
				//to pass cpu_interrupts_v2/5-branch_delays_irq we need to handle a quirk here
				//if we decide to interrupt in the next cycle, this condition will cause it to get deferred by one instruction
				if (!interrupt_pending)
					branch_irq_hack = true;
			}
		}

		void RelBranch_Stage4_F()
		{
			DummyReadMemory(PBR | PC);
			if ((alu_temp & 0x80000000) == 0x80000000)
				PC = (uint16_t)(PC - 0x100);
			else PC = (uint16_t)(PC + 0x100);
		}

		void NOP_F() { DummyReadMemory(PBR | PC); }

		void MMT_F() { if (FlagMget()) { mi += 12; } }

		void XYT_F() { if (FlagXget()) { mi += 12; } }

		void CHP_F() {}

		void CHE_F() {}

		void CHS_F() {}

		void SKP_F() { mi += 6; }

		void DecS_F() { DummyReadMemory((uint16_t)(0x100 | --S)); }

		void IncS_F() { DummyReadMemory((uint16_t)(0x100 | S++)); }

		void JSR_F() { PC = (uint16_t)((ReadMemory((uint16_t)(PBR | PC)) << 8) + opcode2); }

		void PullP_F() { P = ReadMemory((uint16_t)(S++ + 0x100)); /*FlagTset(true);*/ }

		void PullPCL_F() { PC &= 0xFF00; PC |= ReadMemory((uint16_t)(S++ + 0x100)); }

		void PullPCH_NoInc_F() { PC &= 0xFF; PC |= (uint16_t)(ReadMemory((uint16_t)(S + 0x100)) << 8); }

		void ZpIdx_Stage3_X_F() { ReadMemory(opcode2); opcode2 = (uint8_t)(opcode2 + X); }

		void ZpIdx_RMW_Stage4_F() 	{ alu_temp = ReadMemory(opcode2); }

		void ZpIdx_RMW_Stage6_F() { WriteMemory(opcode2, (uint8_t)alu_temp); }

		void dir_READ_F() {}

		void dir_READa_F() {}

		void dir_STA_F() {}

		void dir_STAa_F() {}

		void dir_STX_F() {}

		void dir_STXa_F() {}

		void dir_STY_F() {}

		void dir_STYa_F() {}

		void dir_STZ_F() {}

		void dir_STZa_F() {}

		void _Cpx_F()
		{
			value8 = (uint8_t)alu_temp;
			value16 = (uint16_t)(X - value8);
			FlagCset((X >= value8));
			P = (uint8_t)((P & 0x7D) | TableNZ[(uint8_t)value16]);
		}

		void _Cpy_F()
		{
			value8 = (uint8_t)alu_temp;
			value16 = (uint16_t)(Y - value8);
			FlagCset((Y >= value8));
			P = (uint8_t)((P & 0x7D) | TableNZ[(uint8_t)value16]);
		}

		void _Cmp_F()
		{
			value8 = (uint8_t)alu_temp;
			value16 = (uint16_t)(A - value8);
			FlagCset((A >= value8));
			P = (uint8_t)((P & 0x7D) | TableNZ[(uint8_t)value16]);
		}

		void _Bit_F()
		{
			FlagNset((alu_temp & 0x80) != 0);
			FlagVset((alu_temp & 0x40) != 0);
			FlagZset((A & alu_temp) == 0);
		}

		void _Eor_F()
		{
			A ^= (uint8_t)alu_temp;
			NZ_A_F();
		}

		void _And_F()
		{
			A &= (uint8_t)alu_temp;
			NZ_A_F();
		}

		void _Ora_F()
		{
			A |= (uint8_t)alu_temp;
			NZ_A_F();
		}

		void _Anc_F()
		{
			A &= (uint8_t)alu_temp;
			FlagCset((A & 0x80) == 0x80);
			NZ_A_F();
		}

		void _Asr_F()
		{
			A &= (uint8_t)alu_temp;
			FlagCset((A & 0x1) == 0x1);
			A >>= 1;
			NZ_A_F();
		}

		void _Axs_F()
		{
			X &= A;
			alu_temp = X - (uint8_t)alu_temp;
			X = (uint8_t)alu_temp;
			FlagCset(!((alu_temp & 0x100) == 0x100));
			NZ_X_F();
		}

		void _Arr_F()
		{
			A &= (uint8_t)alu_temp;
			booltemp = ((A & 0x1) == 0x1);
			A = (uint8_t)((A >> 1) | (FlagCget() ? 0x80 : 0x00));
			FlagCset(booltemp);
			if (((A & 0x20) == 0x20))
				if (((A & 0x40) == 0x40))
				{
					FlagCset(true); FlagVset(false);
				}
				else { FlagVset(true); FlagCset(false); }
			else if (((A & 0x40) == 0x40))
			{
				FlagVset(true); FlagCset(true);
			}
			else { FlagVset(false); FlagCset(false); }
			FlagZset((A == 0));
		}

		void _Sbc_F()
		{
			value8 = (uint8_t)alu_temp;
			tempint = A - value8 - (FlagCget() ? 0 : 1);
			if (FlagDget() && BCD_Enabled)
			{
				lo = (A & 0x0F) - (value8 & 0x0F) - (FlagCget() ? 0 : 1);
				hi = (A & 0xF0) - (value8 & 0xF0);
				if ((lo & 0xF0) != 0) lo -= 0x06;
				if ((lo & 0x80) != 0) hi -= 0x10;
				if ((hi & 0x0F00) != 0) hi -= 0x60;
				FlagVset(((A ^ value8) & (A ^ tempint) & 0x80) != 0);
				FlagCset((hi & 0xFF00) == 0);
				A = (uint8_t)((lo & 0x0F) | (hi & 0xF0));
			}
			else
			{
				FlagVset(((A ^ value8) & (A ^ tempint) & 0x80) != 0);
				FlagCset(tempint >= 0);
				A = (uint8_t)tempint;
			}
			NZ_A_F();
		}

		void _Adc_F()
		{
			value8 = (uint8_t)alu_temp;
			if (FlagDget() && BCD_Enabled)
			{
				tempint = (A & 0x0F) + (value8 & 0x0F) + (FlagCget() ? 0x01 : 0x00);
				if (tempint > 0x09)
					tempint += 0x06;
				tempint = (tempint & 0x0F) + (A & 0xF0) + (value8 & 0xF0) + (tempint > 0x0F ? 0x10 : 0x00);
				FlagVset((~(A ^ value8) & (A ^ tempint) & 0x80) != 0);
				FlagZset(((A + value8 + (FlagCget() ? 1 : 0)) & 0xFF) == 0);
				FlagNset((tempint & 0x80) != 0);
				if ((tempint & 0x1F0) > 0x090)
					tempint += 0x060;
				FlagCset(tempint > 0xFF);
				A = (uint8_t)(tempint & 0xFF);
			}
			else
			{
				tempint = value8 + A + (FlagCget() ? 1 : 0);
				FlagVset((~(A ^ value8) & (A ^ tempint) & 0x80) != 0);
				FlagCset(tempint > 0xFF);
				A = (uint8_t)tempint;
				NZ_A_F();
			}
		}

		void IdxInd_Stage3_F() 
		{ 
			if ((D & 0xFF) == 0) 
			{
				if (EM)
				{
					ea = (opcode2 + D + X) & 0xFF;
				}
				else
				{
					ea = ((opcode2 + D + (X & XY_mask)) & 0xFFFF);
				}

				ea |= DBR;

				mi += 12;
			}

			// if low byte of D not 0, add one cycle (the operation below)
		}

		void IdxInd_Stage3a_F()
		{
			if (EM)
			{
				ea = (opcode2 + D + X) & 0xFF;
			}
			else
			{
				ea = ((opcode2 + D + (X & XY_mask)) & 0xFFFF);
			}

			ea |= DBR;
		}

		void IdxInd_Stage4_F() { alu_temp = ReadMemory(ea); ea = (ea + 1) & 0xFFFF; ea |= DBR; }

		void IdxInd_Stage5_F() { alu_temp += (ReadMemory(ea) << 8); ea = alu_temp; }

		void IdxInd_Stage6_READ_F() { alu_temp = ReadMemory((uint16_t)ea); ea = (ea + 1) & 0xFFFF; if (FlagMget()) { mi += 12; } }

		void IdxInd_Stage6_READa_F() { alu_temp |= (ReadMemory((uint16_t)ea) << 8); }

		void IdxInd_Stage6_STA_F() { WriteMemory((uint16_t)ea, A); ea = (ea + 1) & 0xFFFF; if (FlagMget()) { mi += 12; } }

		void IdxInd_Stage6_STAa_F() { WriteMemory((uint16_t)ea, (uint8_t)(A >> 8)); }

		void PushP_F() { /*FlagBset(true);*/ WriteMemory((uint16_t)(S-- + 0x100), P); }

		void PushA_F() { WriteMemory((uint16_t)(S-- + 0x100), A); }

		void PullA_NoInc_F() { A = ReadMemory((uint16_t)(S + 0x100)); NZ_A_F(); }

		void PullP_NoInc_F()
		{
			my_iflag = FlagIget();
			P = ReadMemory((uint16_t)(S + 0x100));
			iflag_pending = FlagIget();
			FlagIset(my_iflag);
			//FlagTset(true); //force T always to remain true
		}

		void Imp_ASL_A_F()
		{
			DummyReadMemory(PBR | PC);
			FlagCset((A & 0x80) != 0);
			A = (uint8_t)(A << 1);
			NZ_A_F();
		}

		void Imp_ROL_A_F()
		{
			DummyReadMemory(PBR | PC);
			temp8 = A;
			A = (uint8_t)((A << 1) | (P & 1));
			FlagCset((temp8 & 0x80) != 0);
			NZ_A_F();
		}

		void Imp_ROR_A_F()
		{
			DummyReadMemory(PBR | PC);
			temp8 = A;
			A = (uint8_t)((A >> 1) | ((P & 1) << 7));
			FlagCset((temp8 & 1) != 0);
			NZ_A_F();
		}

		void Imp_LSR_A_F()
		{
			DummyReadMemory(PBR | PC);
			FlagCset((A & 1) != 0);
			A = (uint8_t)(A >> 1);
			NZ_A_F();
		}

		void JMP_abs_F() { PC = (uint16_t)((ReadMemory(PBR | PC) << 8) + opcode2); }

		void BRL_F() { PC = (uint16_t)((opcode3 << 8) + opcode2); }

		void IncPC_F() { ReadMemory(PBR | PC); PC++; }

		void ZP_RMW_Stage3_F() { alu_temp = ReadMemory(opcode2); }

		void ZP_RMW_Stage5_F() { WriteMemory(opcode2, (uint8_t)alu_temp); }

		void ZP_RMW_INC_F()
		{
			WriteMemory(opcode2, (uint8_t)alu_temp);
			alu_temp = (uint8_t)((alu_temp + 1) & 0xFF);
			P = (uint8_t)((P & 0x7D) | TableNZ[alu_temp]);
		}

		void ZP_RMW_DEC_F()
		{
			WriteMemory(opcode2, (uint8_t)alu_temp);
			alu_temp = (uint8_t)((alu_temp - 1) & 0xFF);
			P = (uint8_t)((P & 0x7D) | TableNZ[alu_temp]);
		}

		void ZP_RMW_ASL_F()
		{
			WriteMemory(opcode2, (uint8_t)alu_temp);
			value8 = (uint8_t)alu_temp;
			FlagCset((value8 & 0x80) != 0);
			alu_temp = value8 = (uint8_t)(value8 << 1);
			P = (uint8_t)((P & 0x7D) | TableNZ[value8]);
		}

		void ZP_RMW_LSR_F()
		{
			WriteMemory(opcode2, (uint8_t)alu_temp);
			value8 = (uint8_t)alu_temp;
			FlagCset((value8 & 1) != 0);
			alu_temp = value8 = (uint8_t)(value8 >> 1);
			P = (uint8_t)((P & 0x7D) | TableNZ[value8]);
		}

		void ZP_RMW_ROR_F()
		{
			WriteMemory(opcode2, (uint8_t)alu_temp);
			value8 = temp8 = (uint8_t)alu_temp;
			alu_temp = value8 = (uint8_t)((value8 >> 1) | ((P & 1) << 7));
			FlagCset((temp8 & 1) != 0);
			P = (uint8_t)((P & 0x7D) | TableNZ[value8]);
		}

		void ZP_RMW_ROL_F()
		{
			WriteMemory(opcode2, (uint8_t)alu_temp);
			value8 = temp8 = (uint8_t)alu_temp;
			alu_temp = value8 = (uint8_t)((value8 << 1) | (P & 1));
			FlagCset((temp8 & 0x80) != 0);
			P = (uint8_t)((P & 0x7D) | TableNZ[value8]);
		}

		void AbsIdx_Stage3_X_F()
		{
			opcode3 = ReadMemory(PBR | PC);
			PC++;
			alu_temp = opcode2 + X;
			ea = (opcode3 << 8) + (alu_temp & 0xFF);
		}

		void AbsIdx_Stage4_F()
		{
			//bleh.. redundant code to make sure we don't clobber alu_temp before using it to decide whether to change ea

			if (((alu_temp & 0x100) == 0x100))
			{
				alu_temp = ReadMemory((uint16_t)ea);
				ea = (uint16_t)(ea + 0x100);
			}
			else alu_temp = ReadMemory((uint16_t)ea);
		}

		void AbsIdx_RMW_Stage5_F() { alu_temp = ReadMemory((uint16_t)ea); }

		void AbsIdx_RMW_Stage7_F() 	{ WriteMemory((uint16_t)ea, (uint8_t)alu_temp); }

		void AbsIdx_RMW_Stage6_DEC_F()
		{
			WriteMemory((uint16_t)ea, (uint8_t)alu_temp);
			alu_temp = value8 = (uint8_t)(alu_temp - 1);
			P = (uint8_t)((P & 0x7D) | TableNZ[value8]);
		}

		void AbsIdx_RMW_Stage6_INC_F()
		{
			WriteMemory((uint16_t)ea, (uint8_t)alu_temp);
			alu_temp = value8 = (uint8_t)(alu_temp + 1);
			P = (uint8_t)((P & 0x7D) | TableNZ[value8]);
		}

		void AbsIdx_RMW_Stage6_ROL_F()
		{
			WriteMemory((uint16_t)ea, (uint8_t)alu_temp);
			value8 = temp8 = (uint8_t)alu_temp;
			alu_temp = value8 = (uint8_t)((value8 << 1) | (P & 1));
			FlagCset((temp8 & 0x80) != 0);
			P = (uint8_t)((P & 0x7D) | TableNZ[value8]);
		}

		void AbsIdx_RMW_Stage6_LSR_F()
		{
			WriteMemory((uint16_t)ea, (uint8_t)alu_temp);
			value8 = (uint8_t)alu_temp;
			FlagCset((value8 & 1) != 0);
			alu_temp = value8 = (uint8_t)(value8 >> 1);
			P = (uint8_t)((P & 0x7D) | TableNZ[value8]);
		}

		void AbsIdx_RMW_Stage6_ASL_F()
		{
			WriteMemory((uint16_t)ea, (uint8_t)alu_temp);
			value8 = (uint8_t)alu_temp;
			FlagCset((value8 & 0x80) != 0);
			alu_temp = value8 = (uint8_t)(value8 << 1);
			P = (uint8_t)((P & 0x7D) | TableNZ[value8]);
		}

		void AbsIdx_RMW_Stage6_ROR_F()
		{
			WriteMemory((uint16_t)ea, (uint8_t)alu_temp);
			value8 = temp8 = (uint8_t)alu_temp;
			alu_temp = value8 = (uint8_t)((value8 >> 1) | ((P & 1) << 7));
			FlagCset((temp8 & 1) != 0);
			P = (uint8_t)((P & 0x7D) | TableNZ[value8]);
		}

		void AbsInd_JMP_Stage4_F() { ea = (opcode3 << 8) + opcode2; alu_temp = ReadMemory((uint16_t)ea); }

		void AbsInd_JMP_Stage5_F()
		{
			ea = (opcode3 << 8) + (uint8_t)(opcode2 + 1);
			alu_temp += ReadMemory((uint16_t)ea) << 8;
			PC = (uint16_t)alu_temp;
		}

		void Abs_RMW_Stage4_F() { ea = (opcode3 << 8) + opcode2; alu_temp = ReadMemory((uint16_t)ea); }

		void Abs_RMW_Stage5_INC_F()
		{
			WriteMemory((uint16_t)ea, (uint8_t)alu_temp);
			value8 = (uint8_t)(alu_temp + 1);
			alu_temp = value8;
			P = (uint8_t)((P & 0x7D) | TableNZ[value8]);
		}

		void Abs_RMW_Stage5_DEC_F()
		{
			WriteMemory((uint16_t)ea, (uint8_t)alu_temp);
			value8 = (uint8_t)(alu_temp - 1);
			alu_temp = value8;
			P = (uint8_t)((P & 0x7D) | TableNZ[value8]);
		}

		void Abs_RMW_Stage5_ASL_F()
		{
			WriteMemory((uint16_t)ea, (uint8_t)alu_temp);
			value8 = (uint8_t)alu_temp;
			FlagCset((value8 & 0x80) != 0);
			alu_temp = value8 = (uint8_t)(value8 << 1);
			P = (uint8_t)((P & 0x7D) | TableNZ[value8]);
		}

		void Abs_RMW_Stage5_ROR_F()
		{
			WriteMemory((uint16_t)ea, (uint8_t)alu_temp);
			value8 = temp8 = (uint8_t)alu_temp;
			alu_temp = value8 = (uint8_t)((value8 >> 1) | ((P & 1) << 7));
			FlagCset((temp8 & 1) != 0);
			P = (uint8_t)((P & 0x7D) | TableNZ[value8]);
		}

		void Abs_RMW_Stage5_ROL_F()
		{
			WriteMemory((uint16_t)ea, (uint8_t)alu_temp);
			value8 = temp8 = (uint8_t)alu_temp;
			alu_temp = value8 = (uint8_t)((value8 << 1) | (P & 1));
			FlagCset((temp8 & 0x80) != 0);
			P = (uint8_t)((P & 0x7D) | TableNZ[value8]);
		}

		void Abs_RMW_Stage5_LSR_F()
		{
			WriteMemory((uint16_t)ea, (uint8_t)alu_temp);
			value8 = (uint8_t)alu_temp;
			FlagCset((value8 & 1) != 0);
			alu_temp = value8 = (uint8_t)(value8 >> 1);
			P = (uint8_t)((P & 0x7D) | TableNZ[value8]);
		}

		void Abs_RMW_Stage6_F() { WriteMemory((uint16_t)ea, (uint8_t)alu_temp); }

		void REP_F() { P ^= opcode2; }
		void SEP_F() { P ^= opcode2; }

		void End_ISpecial_F()
		{
			opcode = VOP_Fetch1;
			mi = 0;
			ExecuteOneRetry();
			return;
		}

		void End_NoInt_F()
		{
			opcode = VOP_Fetch1_NoInterrupt;
			mi = 0;
			ExecuteOneRetry();
			return;
		}

		void End_F()
		{
			opcode = VOP_Fetch1;
			mi = 0;
			iflag_pending = FlagIget();
			ExecuteOneRetry();
		}

		void End_LDA_F()
		{
			A = alu_temp;
			NZ_A_F();
			opcode = VOP_Fetch1;
			mi = 0;
			iflag_pending = FlagIget();
			ExecuteOneRetry();
		}

		void End_LDX_F()
		{
			X = alu_temp;
			NZ_X_F();
			opcode = VOP_Fetch1;
			mi = 0;
			iflag_pending = FlagIget();
			ExecuteOneRetry();
		}

		void End_LDY_F()
		{
			Y = alu_temp;
			NZ_Y_F();
			opcode = VOP_Fetch1;
			mi = 0;
			iflag_pending = FlagIget();
			ExecuteOneRetry();
		}

		void End_ORA_F() 
		{
			_Ora_F();
			opcode = VOP_Fetch1;
			mi = 0;
			iflag_pending = FlagIget();
			ExecuteOneRetry();
		}

		void End_CMP_F()
		{
			_Cmp_F();
			opcode = VOP_Fetch1;
			mi = 0;
			iflag_pending = FlagIget();
			ExecuteOneRetry();
		}

		void End_CPX_F()
		{
			_Cpx_F();
			opcode = VOP_Fetch1;
			mi = 0;
			iflag_pending = FlagIget();
			ExecuteOneRetry();
		}

		void End_CPY_F()
		{
			_Cpy_F();
			opcode = VOP_Fetch1;
			mi = 0;
			iflag_pending = FlagIget();
			ExecuteOneRetry();
		}

		void End_ADC_F()
		{
			_Adc_F();
			opcode = VOP_Fetch1;
			mi = 0;
			iflag_pending = FlagIget();
			ExecuteOneRetry();
		}

		void End_AND_F()
		{
			_And_F();
			opcode = VOP_Fetch1;
			mi = 0;
			iflag_pending = FlagIget();
			ExecuteOneRetry();
		}

		void End_EOR_F()
		{
			_Eor_F();
			opcode = VOP_Fetch1;
			mi = 0;
			iflag_pending = FlagIget();
			ExecuteOneRetry();
		}

		void End_SBC_F()
		{
			_Sbc_F();
			opcode = VOP_Fetch1;
			mi = 0;
			iflag_pending = FlagIget();
			ExecuteOneRetry();
		}

		void End_BIT_F()
		{
			_Bit_F();
			opcode = VOP_Fetch1;
			mi = 0;
			iflag_pending = FlagIget();
			ExecuteOneRetry();
		}

		void DBR_EA_READ_F() {}
		void DBR_EA_READa_F() {}
		void DBR_EA_STA_F() {}
		void DBR_EA_STAa_F() {}
		void DBR_EA_STY_F() {}
		void DBR_EA_STYa_F() {}
		void DBR_EA_STX_F() {}
		void DBR_EA_STXa_F() {}
		void DBR_EA_STZ_F() {}
		void DBR_EA_STZa_F() {}

		void End_BranchSpecial_F() { End_F(); }

		void JSL_F()
		{
			
			PC = (uint16_t)((opcode2 << 8) | opcode3);
			opcode = VOP_Fetch1;
			mi = 0;
			iflag_pending = FlagIget();
			ExecuteOneRetry();
		}

		void StackEA_1_F() { ea = (uint16_t)(S + opcode2); }

		void StkRelInd3_READ_F() { alu_temp = ReadMemory((uint16_t)ea); ea = (ea + 1) & 0xFFFF; }
		
		void StkRelInd3_READa_F() { alu_temp |= (ReadMemory((uint16_t)ea) << 8); }

		void StackEA_2_F() { ea = (uint16_t)(alu_temp + Y); ea |= DBR; }

		void StkRelInd5_READ_F() { alu_temp = ReadMemory((uint16_t)ea); ea = (ea + 1) & 0xFFFF; ea |= DBR; if (FlagMget()) { mi += 12; } }

		void StkRelInd5_READa_F() { alu_temp |= (ReadMemory((uint16_t)ea) << 8); }

		void StkRelInd5_STA_F() { WriteMemory((uint16_t)ea, A); ea = (ea + 1) & 0xFFFF; ea |= DBR; if (FlagMget()) { mi += 12; } }

		void StkRelInd5_STAa_F() { WriteMemory((uint16_t)ea, (uint8_t)(A >> 8)); }

		void EA_DX_F() {}

		void EA_DY_F() {}

		void Imm_READ_F() {}

		void Imm_READa_F() {}

		void Abs_Fetch_EA_F()
		{
			opcode3 = ReadMemory(PBR | PC);
			PC++;
			alu_temp = opcode2;
			ea = (opcode3 << 8) + (alu_temp & 0xFF);
		}

		void Abs_Fetch_EA_Y_F() 
		{
			opcode3 = ReadMemory(PBR | PC);
			PC++;
			alu_temp = opcode2 + Y;
			ea = (opcode3 << 8) + (alu_temp & 0xFF);
		}

		void Abs_Fetch_EA_X_F() {}

		void Absl_Fetch_EA_F() {}
		void Abslx_Fetch_EA_F() {}
		void Absl_READ_F() {}
		void Absl_READa_F() {}
		void Absl_STA_F() {}
		void Absl_STAa_F() {}

		void DirectEA_RD_F()
		{
			ea = (uint16_t)(D + opcode2);
			alu_temp = ReadMemory(ea);
		}

		void TSB_F()
		{

		}

		void TRB_F()
		{

		}

		#pragma endregion

		#pragma region Disassemble

		// disassemblies will also return strings of the same length
		const char* TraceHeader = "6502: PC, machine code, mnemonic, operands, registers (A, X, Y, P, SP), flags (NVTBDIZCR)";

		const char* IRQ_event		= "                  ====IRQ====                  ";
		const char* NMI_event		= "                  ====NMI====                  ";
		const char* No_Reg			= "                                                              ";
		const char* Reg_template	= "A:AA X:XX Y:YY P:PP SP:SP Cy:FEDCBA9876543210 LY:LLY NVTBDIZCR";
		const char* Disasm_template = "PCPC: AA BB CC DD   Di Di, XXXXX               ";

		char replacer[32] = {};
		char* val_char_1 = nullptr;
		char* val_char_2 = nullptr;
		int temp_reg;


		void (*TraceCallback)(int);

		string CPURegisterState()
		{		
			val_char_1 = replacer;

			string reg_state = "A:";
			sprintf_s(val_char_1, 5, "%02X", A);
			reg_state.append(val_char_1, 2);

			reg_state.append(" X:");
			sprintf_s(val_char_1, 5, "%02X", X);
			reg_state.append(val_char_1, 2);

			reg_state.append(" Y:");
			sprintf_s(val_char_1, 5, "%02X", Y);
			reg_state.append(val_char_1, 2);

			reg_state.append(" P:");
			sprintf_s(val_char_1, 5, "%02X", P);
			reg_state.append(val_char_1, 2);

			reg_state.append(" SP:");;
			sprintf_s(val_char_1, 5, "%04X", S);
			reg_state.append(val_char_1, 4);

			reg_state.append(" Cy:");			
			reg_state.append(val_char_1, sprintf_s(val_char_1, 32, "%16u", (unsigned long)TotalExecutedCycles));
			reg_state.append(" ");

			reg_state.append(" LY:");
			reg_state.append(val_char_1, sprintf_s(val_char_1, 32, "%3u", LY));
			reg_state.append(" ");
			
			reg_state.append(FlagNget() ? "N" : "n");
			reg_state.append(FlagVget() ? "V" : "v");
			reg_state.append(FlagMget() ? "M" : "m");
			reg_state.append(FlagXget() ? "X" : "x");
			reg_state.append(FlagDget() ? "D" : "d");
			reg_state.append(FlagIget() ? "I" : "i");
			reg_state.append(FlagZget() ? "Z" : "z");
			reg_state.append(FlagCget() ? "C" : "c");
			
			reg_state.append(RDY ? "R" : "r");

			return reg_state;
		}

		string CPUDisassembly()
		{
			uint32_t bytes_read = 0;

			uint32_t* bytes_read_ptr = &bytes_read;

			string disasm = Disassemble(RegPCget(), bytes_read_ptr);
			string byte_code = "";

			val_char_1 = replacer;
			sprintf_s(val_char_1, 5, "%04X", RegPCget() & 0xFFFF);
			byte_code.append(val_char_1, 4);
			byte_code.append(": ");

			uint32_t i = 0;

			for (i = 0; i < bytes_read; i++)
			{
				char* val_char_1 = replacer;
				sprintf_s(val_char_1, 5, "%02X", PeekMemory((RegPCget() + i) & 0xFFFF));
				string val1(val_char_1, 2);
				
				byte_code.append(val1);
				byte_code.append(" ");
			}

			while (i < 4) 
			{
				byte_code.append("   ");
				i++;
			}

			byte_code.append("   ");

			byte_code.append(disasm);

			while (byte_code.length() < 48) 
			{
				byte_code.append(" ");
			}

			return byte_code;
		}

		string Result(string format, uint32_t* addr)
		{
			//d immediately succeeds the opcode
			//n immediate succeeds the opcode and the displacement (if present)
			//nn immediately succeeds the opcode and the displacement (if present)

			if (format.find("a16") != string::npos)
			{
				size_t str_loc = format.find("a16");

				val_char_1 = replacer;
				sprintf_s(val_char_1, 5, "%02X", PeekMemory(addr[0] & 0xFFFF));
				string val1(val_char_1, 2);
				addr[0]++;

				val_char_2 = replacer;
				sprintf_s(val_char_2, 5, "%02X", PeekMemory(addr[0] & 0xFFFF));
				string val2(val_char_2, 2);
				addr[0]++;

				format.erase(str_loc, 3);
				format.insert(str_loc, val1);
				format.insert(str_loc, val2);
			}

			if (format.find("d16") != string::npos)
			{
				size_t str_loc = format.find("d16");

				val_char_1 = replacer;
				sprintf_s(val_char_1, 5, "%02X", PeekMemory(addr[0] & 0xFFFF));
				string val1(val_char_1, 2);
				addr[0]++;

				val_char_2 = replacer;
				sprintf_s(val_char_2, 5, "%02X", PeekMemory(addr[0] & 0xFFFF));
				string val2(val_char_2, 2);
				addr[0]++;

				format.erase(str_loc, 3);
				format.insert(str_loc, val1);
				format.insert(str_loc, val2);
			}
			
			if (format.find("d8") != string::npos)
			{
				size_t str_loc = format.find("d8");

				val_char_1 = replacer;
				sprintf_s(val_char_1, 5, "%02X", PeekMemory(addr[0] & 0xFFFF));
				string val1(val_char_1, 2);
				addr[0]++;

				format.erase(str_loc, 2);
				format.insert(str_loc, val1);
			}

			if (format.find("a8") != string::npos)
			{
				size_t str_loc = format.find("a8");

				val_char_2 = replacer;
				sprintf_s(val_char_1, 5, "%02X", PeekMemory(addr[0] & 0xFFFF));
				string val1(val_char_1, 2);
				addr[0]++;

				string val2 = "FF";

				format.erase(str_loc, 2);
				format.insert(str_loc, val1);
				format.insert(str_loc, val2);
			}

			if (format.find("r8") != string::npos)
			{
				size_t str_loc = format.find("r8");

				val_char_1 = replacer;
				sprintf_s(val_char_1, 5, "%04X", ((addr[0] + 1) + (int8_t)PeekMemory(addr[0] & 0xFFFF)) & 0xFFFF);
				string val1(val_char_1, 4);
				addr[0]++;

				format.erase(str_loc, 2);
				format.insert(str_loc, val1);
			}
						
			return format;
		}

		string Disassemble(uint32_t addr, uint32_t* size)
		{
			uint32_t start_addr = addr;
			uint32_t extra_inc = 0;

			uint32_t A = PeekMemory(addr & 0xFFFF);
			addr++;
			string format;
			switch (A)
			{
			case 0xCB:
				A = PeekMemory(addr & 0xFFFF);
				addr++;
				format = mnemonics[A + 256];
				break;

			default: format = mnemonics[A]; break;
			}

			uint32_t* addr_ptr = &addr;
			string temp = Result(format, addr_ptr);

			addr += extra_inc;

			size[0] = addr - start_addr;
			// handle case of addr wrapping around at 16 bit boundary
			if (addr < start_addr)
			{
				size[0] = (0x10000 + addr) - start_addr;
			}

			return temp;
		}

		/*
		string Disassemble(MemoryDomain m, uuint32_t addr, out uint32_t length)
		{
			string ret = Disassemble((uint32_t)addr, a = > m.PeekByte(a), out length);
			return ret;
		}
		*/

		const string mnemonics[256] =
		{
			"BRK",							// 0x00
			"ORA (d8,X)",
			"COP d8",
			"ORA (d8,S)",
			"TBS d8",
			"ORA d8",
			"ASL d8",
			"ORA [d16]",
			"PHP",
			"ORA #d8",
			"ASL A",
			"PHD",
			"NOP (d16)",
			"ORA d16",
			"ASL d16",
			"TSB",
			"BPL r8",						// 0x10
			"ORA (d8),Y *",
			"ORA (d8)",
			"ORA (d8,S),Y",
			"NOP d8,X",
			"ORA d8,X",
			"ASL d8,X",
			"NOP",
			"CLC",
			"ORA d16,Y *",
			"NOP",
			"NOP",
			"NOP (d8,X)",
			"ORA d16,X *",
			"ASL d16,X",
			"NOP",
			"JSR d16",						// 0x20
			"AND (d8,X)",
			"JSL (d16)",
			"AND (d8,S)",
			"BIT d8",
			"AND d8",
			"ROL d8",
			"NOP",
			"PLP",
			"AND #d8",
			"ROL A",
			"NOP",
			"BIT d16",
			"AND d16",
			"ROL d16",
			"NOP",
			"BMI r8",						// 0x30
			"AND (d8),Y *",
			"AND (d8)",
			"ORA (d8,S),Y",
			"NOP d8,X",
			"AND d8,X",
			"ROL d8,X",
			"NOP",
			"SEC",
			"AND d16,Y *",
			"NOP",
			"NOP",
			"NOP (d8,X)",
			"AND d16,X *",
			"ROL d16,X",
			"NOP",
			"RTI",							// 0x40
			"EOR (d8,X)",
			"WDM",
			"EOR (d8,S)",
			"NOP d8",
			"EOR d8",
			"LSR d8",
			"NOP",
			"PHA",
			"EOR #d8",
			"LSR A",
			"NOP",
			"JMP d16",
			"EOR d16",
			"LSR d16",
			"NOP",
			"BVC r8",						// 0x50
			"EOR (d8),Y *",
			"EOR (d8)",
			"EOR (d8,S),Y",
			"NOP d8,X",
			"EOR d8,X",
			"LSR d8,X",
			"NOP",
			"CLI",
			"EOR d16,Y *",
			"NOP",
			"NOP",
			"NOP (d8,X)",
			"EOR d16,X *",
			"LSR d16,X",
			"NOP",
			"RTS",							// 0x60
			"ADC (d8,X)",
			"PER (d16)",
			"ADC (d8,S)",
			"NOP d8",
			"ADC d8",
			"ROR d8",
			"NOP",
			"PLA",
			"ADC #d8",
			"ROR A",
			"NOP",
			"JMP (d16)",
			"ADC d16",
			"ROR d16",
			"NOP",
			"BVS r8",						//0x70
			"ADC (d8),Y *",
			"ADC (d8)",
			"ADC (d8,S),Y",
			"NOP d8,X",
			"ADC d8,X",
			"ROR d8,X",
			"NOP",
			"SEI",
			"ADC d16,Y *",
			"NOP",
			"NOP",
			"NOP (d8,X)",
			"ADC d16,X *",
			"ROR d16,X",
			"NOP",
			"NOP #d8",						// 0x80
			"STA (d8,X)",
			"BRL",
			"STA (d8,S)",
			"STY d8",
			"STA d8",
			"STX d8",
			"NOP",
			"DEY",
			"NOP #d8",
			"TXA",
			"NOP",
			"STY d16",
			"STA d16",
			"STX d16",
			"NOP",
			"BCC r8",						// 0x90
			"STA (d8),Y",
			"STA (d8)",
			"STA (d8,S),Y",
			"STY d8,X",
			"STA d8,X",
			"STX d8,Y",
			"NOP",
			"TYA",
			"STA d16,Y",
			"TXS",
			"NOP",
			"NOP",
			"STA d16,X",
			"NOP",
			"NOP",
			"LDY #d8",						// 0xA0
			"LDA (d8,X)",
			"LDX #d8",
			"LDA (d8,S)",
			"LDY d8",
			"LDA d8",
			"LDX d8",
			"NOP",
			"TAY",
			"LDA #d8",
			"TAX",
			"NOP",
			"LDY d16",
			"LDA d16",
			"LDX d16",
			"NOP",
			"BCS r8",						// 0xB0
			"LDA (d8),Y *",
			"LDA (d8)",
			"LDA (d8,S),Y",
			"LDY d8,X",
			"LDA d8,X",
			"LDX d8,Y",
			"NOP",
			"CLV",
			"LDA d16,Y *",
			"TSX",
			"NOP",
			"LDY d16,X *",
			"LDA d16,X *",
			"LDX d16,Y *",
			"NOP",
			"CPY #d8",						// 0xC0
			"CMP (d8,X)",
			"REP #d8",
			"CMP (d8,S)",
			"CPY d8",
			"CMP d8",
			"DEC d8",
			"NOP",
			"INY",
			"CMP #d8",
			"DEX",
			"AXS d8",
			"CPY d16",
			"CMP d16",
			"DEC d16",
			"NOP",
			"BNE r8",						// 0xD0
			"CMP (d8),Y *",
			"CMP (d8)",
			"CMP (d8,S),Y",
			"NOP d8,X",
			"CMP d8,X",
			"DEC d8,X",
			"NOP",
			"CLD",
			"CMP d16,Y *",
			"NOP",
			"NOP",
			"NOP (d8,X)",
			"CMP d16,X *",
			"DEC d16,X",
			"NOP",
			"CPX #d8",						// 0xE0
			"SBC (d8,X)",
			"SEP #d8",
			"SBC (d8,S)",
			"CPX d8",
			"SBC d8",
			"INC d8",
			"NOP",
			"INX",
			"SBC #d8",
			"NOP",
			"NOP",
			"CPX d16",
			"SBC d16",
			"INC d16",
			"NOP",
			"BEQ r8",						// 0xF0
			"SBC (d8),Y *",
			"SBC (d8)",
			"SBC (d8,S),Y",
			"NOP d8,X",
			"SBC d8,X",
			"INC d8,X",
			"NOP",
			"SED",
			"SBC d16,Y *",
			"NOP",
			"NOP",
			"NOP (d8,X)",
			"SBC d16,X *",
			"INC d16,X",
			"NOP"
		};

		#pragma endregion

		#pragma region State Save / Load

		uint8_t* SaveState(uint8_t* saver)
		{
			saver = bool_saver(EM, saver);
			saver = bool_saver(branch_taken, saver);
			saver = bool_saver(my_iflag, saver);
			saver = bool_saver(booltemp, saver);
			saver = bool_saver(BCD_Enabled, saver);
			saver = bool_saver(RDY, saver);
			saver = bool_saver(NMI, saver);
			saver = bool_saver(IRQ, saver);
			saver = bool_saver(iflag_pending, saver);
			saver = bool_saver(rdy_freeze, saver);
			saver = bool_saver(interrupt_pending, saver);
			saver = bool_saver(branch_irq_hack, saver);

			saver = byte_saver(value8, saver);
			saver = byte_saver(temp8, saver);
			saver = byte_saver(P, saver);
			saver = short_saver(A, saver);
			saver = short_saver(X, saver);
			saver = short_saver(Y, saver);
			saver = short_saver(XY_mask, saver);
			saver = short_saver(S, saver);
			saver = int_saver(DBR, saver);
			saver = int_saver(PBR, saver);
			saver = byte_saver(opcode2, saver);
			saver = byte_saver(opcode3, saver);
			saver = byte_saver(opcode4, saver);

			saver = short_saver(PC, saver);
			saver = short_saver(D, saver);
			saver = short_saver(value16, saver);

			saver = int_saver(tempint, saver);
			saver = int_saver(lo, saver);
			saver = int_saver(hi, saver);
			saver = int_saver(opcode, saver);
			saver = int_saver(ea, saver);
			saver = int_saver(alu_temp, saver);
			saver = int_saver(mi, saver);
			saver = int_saver(LY, saver);

			*saver = (uint8_t)(TotalExecutedCycles & 0xFF); saver++; *saver = (uint8_t)((TotalExecutedCycles >> 8) & 0xFF); saver++;
			*saver = (uint8_t)((TotalExecutedCycles >> 16) & 0xFF); saver++; *saver = (uint8_t)((TotalExecutedCycles >> 24) & 0xFF); saver++;
			*saver = (uint8_t)((TotalExecutedCycles >> 32) & 0xFF); saver++; *saver = (uint8_t)((TotalExecutedCycles >> 40) & 0xFF); saver++;
			*saver = (uint8_t)((TotalExecutedCycles >> 48) & 0xFF); saver++; *saver = (uint8_t)((TotalExecutedCycles >> 56) & 0xFF); saver++;

			return saver;
		}

		uint8_t* LoadState(uint8_t* loader)
		{
			loader = bool_loader(&EM, loader);
			loader = bool_loader(&branch_taken, loader);
			loader = bool_loader(&my_iflag, loader);
			loader = bool_loader(&booltemp, loader);
			loader = bool_loader(&BCD_Enabled, loader);
			loader = bool_loader(&RDY, loader);
			loader = bool_loader(&NMI, loader);
			loader = bool_loader(&IRQ, loader);
			loader = bool_loader(&iflag_pending, loader);
			loader = bool_loader(&rdy_freeze, loader);
			loader = bool_loader(&interrupt_pending, loader);
			loader = bool_loader(&branch_irq_hack, loader);

			loader = byte_loader(&value8, loader);
			loader = byte_loader(&temp8, loader);
			loader = byte_loader(&P, loader);
			loader = short_loader(&A, loader);
			loader = short_loader(&X, loader);
			loader = short_loader(&Y, loader);
			loader = short_loader(&XY_mask, loader);
			loader = short_loader(&S, loader);
			loader = int_loader(&DBR, loader);
			loader = int_loader(&PBR, loader);
			loader = byte_loader(&opcode2, loader);
			loader = byte_loader(&opcode3, loader);
			loader = byte_loader(&opcode4, loader);

			loader = short_loader(&PC, loader);
			loader = short_loader(&D, loader);
			loader = short_loader(&value16, loader);

			loader = sint_loader(&tempint, loader);
			loader = sint_loader(&lo, loader);
			loader = sint_loader(&hi, loader);
			loader = sint_loader(&opcode, loader);
			loader = sint_loader(&ea, loader);
			loader = sint_loader(&alu_temp, loader);
			loader = sint_loader(&mi, loader);
			loader = sint_loader(&LY, loader);

			TotalExecutedCycles = *loader; loader++; TotalExecutedCycles |= ((uint64_t)*loader << 8); loader++;
			TotalExecutedCycles |= ((uint64_t)*loader << 16); loader++; TotalExecutedCycles |= ((uint64_t)*loader << 24); loader++;
			TotalExecutedCycles |= ((uint64_t)*loader << 32); loader++; TotalExecutedCycles |= ((uint64_t)*loader << 40); loader++;
			TotalExecutedCycles |= ((uint64_t)*loader << 48); loader++; TotalExecutedCycles |= ((uint64_t)*loader << 56); loader++;

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

		#pragma endregion

		#pragma region Instruction Table Building

		/*
{
	//0x00---------------
	// 0x00
	// 0x01
	// 0x02
	// 0x03
	Fetch2,					DirectEA_RD,		TSB,					IndIdx_RMW_Stage8,			End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// TSB d
	// 0x05
	Fetch2,					ZP_RMW_Stage3,		ZP_RMW_ASL,				ZP_RMW_Stage5,				End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// ASL d
	// 0x07
	// 0x08
	// 0x09
	Imp_ASL_A,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// ASL A
	// 0x0B
	NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// TSB a
	// 0x0D
	Fetch2,					Fetch3,				Abs_RMW_Stage4,			Abs_RMW_Stage5_ASL,			Abs_RMW_Stage6,				End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// ASL a
	// 0x0F
	//0x10---------------
	RelBranch_Stage2_BPL,	End ,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// BPL r
	// 0x11
	// 0x12
	// 0x13
	Fetch2,					DirectEA_RD,		TRB,					IndIdx_RMW_Stage8,			End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// TRB d
	// 0x15
	Fetch2,					ZpIdx_Stage3_X,		ZpIdx_RMW_Stage4,		ZP_RMW_ASL,					ZpIdx_RMW_Stage6,			End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// ASL d,x
	// 0x17
	Imp_CLC,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// CLC i
	// 0x19
	NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// INC A
	End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// TCS i
	NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// TRB a
	// 0x1D
	Fetch2,					AbsIdx_Stage3_X,	AbsIdx_Stage4,			AbsIdx_RMW_Stage5,			AbsIdx_RMW_Stage6_ASL,		AbsIdx_RMW_Stage7,			End,						NOP,		NOP,		NOP,		NOP,		NOP,	// ASL a,x
	// 0x1F
	//0x20-----------------
	Fetch2,					NOP,				PushPCH,				PushPCL,					JSR,						End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// JSR a
	// 0x21
	Fetch2,					Fetch3,				PushPBR,				NOP,						Fetch4,						PushPCH,					PushPCL,					JSL,		NOP,		NOP,		NOP,		NOP,	// JSL al
	// 0x23
	// 0x24
	// 0x25
	Fetch2,					ZP_RMW_Stage3,		ZP_RMW_ROL,				ZP_RMW_Stage5,				End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// ROL d
	// 0x27
	FetchDummy,				IncS,				PullP_NoInc,			End_ISpecial,				NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// PLP s
	// 0x29
	Imp_ROL_A,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// ROL A
	End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// PLD s
	// 0x2C
	// 0x2D
	Fetch2,					Fetch3,				Abs_RMW_Stage4,			Abs_RMW_Stage5_ROL,			Abs_RMW_Stage6,				End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// ROL a
	// 0x2F
	//0x30-----------------
	RelBranch_Stage2_BMI,	End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// BMI r
	// 0x31
	// 0x32
	// 0x33
	// 0x3$
	// 0x35
	Fetch2,					ZpIdx_Stage3_X,		ZpIdx_RMW_Stage4,		ZP_RMW_ROL,					ZpIdx_RMW_Stage6,			End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// ROL d,x
	// 0x37
	Imp_SEC,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// SEC i
	// 0x39
	NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// DEC A
	End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// TSC i
	// 0x3C
	// 0x3D
	Fetch2,					AbsIdx_Stage3_X,	AbsIdx_Stage4,			AbsIdx_RMW_Stage5,			AbsIdx_RMW_Stage6_ROL,		AbsIdx_RMW_Stage7,			End,						NOP,		NOP,		NOP,		NOP,		NOP,	// ROL a,x
	// 0x3F
	//0x40-----------------
	FetchDummy,				IncS,				PullP,					PullPCL,					PullPCH_NoInc,				End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// RTI s
	// 0x41
	// 0x42
	// 0x43
	NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// MVP xyc
	// 0x45
	Fetch2,					ZP_RMW_Stage3,		ZP_RMW_LSR,				ZP_RMW_Stage5,				End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// LSR d
	// 0x47
	// 0x48
	// 0x49
	Imp_LSR_A,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// LSR A
	// 0x4B
	Fetch2,					JMP_abs,			End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// JMP a
	// 0x4D
	Fetch2,					Fetch3,				Abs_RMW_Stage4,			Abs_RMW_Stage5_LSR,			Abs_RMW_Stage6,				End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// LSR a
	// 0x4F
	//0x50-------------------
	RelBranch_Stage2_BVC,	End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// BVC r
	// 0x51
	// 0x52
	// 0x53
	NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// MVN xyc
	// 0x55
	Fetch2,					ZpIdx_Stage3_X,		ZpIdx_RMW_Stage4,		ZP_RMW_LSR,					ZpIdx_RMW_Stage6,			End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// LSR d,x
	// 0x57
	Imp_CLI,				End_ISpecial,		NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// CLI i
	// 0x59
	// 0x5A
	End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// TCD i
	NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// JMP al
	// 0x5D
	Fetch2,					AbsIdx_Stage3_X,	AbsIdx_Stage4,			AbsIdx_RMW_Stage5,			AbsIdx_RMW_Stage6_LSR,		AbsIdx_RMW_Stage7,			End,						NOP,		NOP,		NOP,		NOP,		NOP,	// LSR a,x
	// 0x5F
	//0x60------------------
	FetchDummy,				IncS,				PullPCL,				PullPCH_NoInc,				IncPC,						End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// RTS s
	// 0x61
	// 0x62
	// 0x63
	// 0x64
	// 0x65
	Fetch2,					ZP_RMW_Stage3,		ZP_RMW_ROR,				ZP_RMW_Stage5,				End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// ROR d
	// 0x67
	FetchDummy,				IncS,				PullA_NoInc,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// PLA s
	// 0x69
	Imp_ROR_A,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// ROR A
	End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// RTL s
	Fetch2,					Fetch3,				AbsInd_JMP_Stage4,		AbsInd_JMP_Stage5,			End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// JMP (a)
	// 0x6D
	Fetch2,					Fetch3,				Abs_RMW_Stage4,			Abs_RMW_Stage5_ROR,			Abs_RMW_Stage6,				End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// ROR a
	// 0x6F
	//0x70-----------------
	RelBranch_Stage2_BVS,	End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// BVS r
	// 0x71
	// 0x72
	// 0x73
	Fetch2,					ZpIdx_Stage3_X,		ZP_WRITE_STZ,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// STZ d,x
	// 0x75
	Fetch2,					ZpIdx_Stage3_X,		ZpIdx_RMW_Stage4,		ZP_RMW_ROR,					ZpIdx_RMW_Stage6,			End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// ROR d,x
	// 0x77
	Imp_SEI,				End_ISpecial,		NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// SEI i
	// 0x79
	NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// PLY s
	End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// TDC i
	NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// JMP (a,x)
	// 0x7D
	Fetch2,					AbsIdx_Stage3_X,	AbsIdx_Stage4,			AbsIdx_RMW_Stage5,			AbsIdx_RMW_Stage6_ROR,		AbsIdx_RMW_Stage7,			End,						NOP,		NOP,		NOP,		NOP,		NOP,	// ROR a,x]
	// 0x7F
	//0x80---------------
	RelBranch_Stage2_A,		End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// BRA r
	// 0x81
	Fetch2,					Fetch3,				BRL,					End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// BRL rl
	// 0x83
	// 0x84
	// 0x85
	// 0x86
	// 0x87
	Imp_DEY,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// DEY i
	// 0x89
	Imp_TXA,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// TXA i
	// 0x8B
	// 0x8C
	// 0x8D
	// 0x8E
	// 0x8F
	//0x90------------------
	RelBranch_Stage2_BCC,	End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// BCC r
	// 0x91
	// 0x92
	// 0x93
	// 0x94
	// 0x95
	// 0x96
	// 0x97
	Imp_TYA,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// TYA i
	// 0x99
	Imp_TXS,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// TXS i
	End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// TXY i
	// 0x9C
	// 0x9D
	// 0x9E
	// 0x9F
	//0xA0-----------------
	// 0xA0
	// 0xA1
	// 0xA2
	// 0xA3
	// 0xA4
	// 0xA5
	// 0xA6
	// 0xA7
	Imp_TAY,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// TAY i
	// 0xA9
	Imp_TAX,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// TAX i
	End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// PLB s
	// 0xAC
	// 0xAD
	// 0xAE
	// 0xAF
	//0xB0------------------
	RelBranch_Stage2_BCS,	End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// BCS r
	// 0xB1
	// 0xB2
	// 0xB3
	// 0xB4
	// 0xB5
	// 0xB6
	// 0xB7
	Imp_CLV,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// CLV i
	// 0xB9
	Imp_TSX,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// TSX i
	End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// TYX i
	// 0xBC
	// 0xBD
	// 0xBE
	// 0xBF
	//0xC0----------------
	// 0C0
	// 0xC1
	Fetch2,					Fetch3,				REP,					End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// REP #
	// 0xC3
	Fetch2,					ZP_READ_CPY,		End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// CPY d
	// 0xC5
	Fetch2,					ZP_RMW_Stage3,		ZP_RMW_DEC,				ZP_RMW_Stage5,				End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// DEC d
	// 0xC7
	Imp_INY,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// INY i
	// 0xC9
	Imp_DEX,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// DEX i
	End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// WAI i
	Fetch2,					Fetch3,				Abs_READ_CPY,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// CPY a
	// 0xCD
	Fetch2,					Fetch3,				Abs_RMW_Stage4,			Abs_RMW_Stage5_DEC,			Abs_RMW_Stage6,				End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// DEC a
	// 0xCF
	//0xD0-------------------
	RelBranch_Stage2_BNE,	End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// BNE r
	// 0xD1
	// 0xD2
	// 0xD3
	Fetch2,					Fetch3,				NOP,					PushPERH,					PushPERL,					End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// PEI s
	// 0xD5
	Fetch2,					ZpIdx_Stage3_X,		ZpIdx_RMW_Stage4,		ZP_RMW_DEC,					ZpIdx_RMW_Stage6,			End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// DEC d,x
	// 0xD7
	Imp_CLD,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// CLD i
	// 0xD9
	// 0xDA
	End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// STP i
	NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// JML (a)
	// 0xDD
	Fetch2,					AbsIdx_Stage3_X,	AbsIdx_Stage4,			AbsIdx_RMW_Stage5,			AbsIdx_RMW_Stage6_DEC,		AbsIdx_RMW_Stage7,			End,						NOP,		NOP,		NOP,		NOP,		NOP,	// DEC a,x
	// 0xDF
	//0xE0--------------------
	// 0xE0
	// 0xE1
	Fetch2,					Fetch3,				SEP,					End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// SEP #
	// 0xE3
	Fetch2,					ZP_READ_CPX,		End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// CPX d
	// 0xE5
	Fetch2,					ZP_RMW_Stage3,		ZP_RMW_INC,				ZP_RMW_Stage5,				End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// INC d
	// 0xE7
	Imp_INX,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// INX i
	// 0xE9
	FetchDummy,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// NOP i
	End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// XBA i
	Fetch2,					Fetch3,				Abs_READ_CPX,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// CPX a
	// 0xED
	Fetch2,					Fetch3,				Abs_RMW_Stage4,			Abs_RMW_Stage5_INC,			Abs_RMW_Stage6,				End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// INC a
	// 0xEF
	//0xF0--------------------
	RelBranch_Stage2_BEQ,	End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// BEQ r
	// 0xF1
	// 0xF2
	// 0xF3
	Fetch2,					Fetch3,				PushPERH,				PushPERL,					End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// PEA s
	// 0xF5
	Fetch2,					ZpIdx_Stage3_X,		ZpIdx_RMW_Stage4,		ZP_RMW_INC,					ZpIdx_RMW_Stage6,			End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// INC d,x
	// 0xF7
	Imp_SED,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// SED i
	// 0xF9
	NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// PLX s
	End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// XCE i
	NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// JSR (a,x)
	// 0xFD
	Fetch2,					AbsIdx_Stage3_X,	AbsIdx_Stage4,			AbsIdx_RMW_Stage5,			AbsIdx_RMW_Stage6_INC,		AbsIdx_RMW_Stage7,			End,						NOP,		NOP,		NOP,		NOP,		NOP,	// INC a,x
	// 0xFF
	//0x100--------------------
	Fetch1,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// VOP_Fetch1
	RelBranch_Stage3,		End_BranchSpecial,	NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// VOP_RelativeStuff
	RelBranch_Stage4,		End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// VOP_RelativeStuff2
	End_NoInt,				NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// VOP_RelativeStuff3
	//i assume these are dummy fetches.... maybe theyre just nops? supposedly these take 7 cycles so that's the only way i can make sense of it,
	//one of them might be the next instruction's fetch,		and whatever fetch follows it.
	//the interrupt would then take place if necessary,		using a cached PC. but im not so sure about that.
	FetchDummy,				FetchDummy,			PushPCH,				PushPCL,					PushP_NMI,					FetchPCLVector,				FetchPCHVector,				End_NoInt,		NOP,	NOP,	NOP,	NOP,	// VOP_NMI
	FetchDummy,				FetchDummy,			PushPCH,				PushPCL,					PushP_IRQ,					FetchPCLVector,				FetchPCHVector,				End_NoInt,		NOP,	NOP,	NOP,	NOP,	// VOP_IRQ
	FetchDummy,				FetchDummy,			PushDummy,				PushDummy,					PushP_Reset,				FetchPCLVector,				FetchPCHVector,				End_NoInt,		NOP,	NOP,	NOP,	NOP,	// VOP_RESET
	Fetch1_Real,			NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	// VOP_Fetch1_NoInterrupt

};
*/

		void build_instruction_table()
		{
			// fill with nops first
			for (int i = 0; i < ((256 + 8) * 12 * 12); i++)
			{
				Microcode[i] = NOP;
			}

			// instructions are broken up according to their memory access method
			// see the w65c816s data sheet
			build_stack_instructions();
			build_diix_instructions();
			build_diiy_instructions();
			build_di_instructions();
			build_ds_instructions();
			build_dsii_instructions();
			build_d_instructions();
			build_dx_instructions();
			build_dil_instructions();
			build_dily_instructions();
			build_imm_instructions();
			build_ayx_instructions();
			build_a_instructions();
			build_al_instructions();
			build_alx_instructions();

		}

		void build_stack_instructions()
		{
			Uop temp_BRK_COP[8 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch2,
				NOP, CHS, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		PushPBR_BRK,
				NOP, CHS, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		PushPCH,
				NOP, CHS, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		PushPCL,
				NOP, CHS, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		PushP_BRK,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		FetchPCLVector,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		FetchPCHVector,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End_NoInt
			};
			num_uops = 8;
			int replace_position = 4 * 12 + 11;
			int replace_position2 = 0;
			
			assemble_instruction(0x00, PushP_BRK, replace_position, &temp_BRK_COP[0]); // BRK
			assemble_instruction(0x02, PushP_COP, replace_position, &temp_BRK_COP[0]); // COP

			Uop temp_PHv[3 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		FetchDummy,
				NOP, CHS, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		PushP,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End
			};
			num_uops = 3;
			replace_position = 1 * 12 + 11;

			assemble_instruction(0x08, PushP, replace_position, &temp_PHv[0]); // PHP
			assemble_instruction(0x48, PushA, replace_position, &temp_PHv[0]); // PHA
			assemble_instruction(0x4B, PushK, replace_position, &temp_PHv[0]); // PHK
			assemble_instruction(0x8B, PushB, replace_position, &temp_PHv[0]); // PHB

			op = 0x0B; // PHD
			Uop temp_PHvl[4 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		FetchDummy,
				NOP, CHS, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		PushDH,
				NOP, CHS, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		PushDL,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End
			};
			num_uops = 4;
			replace_position = 1 * 12 + 11;
			replace_position2 = 2 * 12 + 11;

			assemble_instruction2(0x0B, PushDH, replace_position, PushDL, replace_position2, &temp_PHvl[0]); // PHD

			// X and Y regs may be only 8 bits, so add in a check
			temp_PHvl[1 * 12] = XYT;

			assemble_instruction2(0x5A, PushYH, replace_position, PushYL, replace_position2, &temp_PHvl[0]); // PHY
			assemble_instruction2(0xDA, PushXH, replace_position, PushXL, replace_position2, &temp_PHvl[0]); // PHY

			op = 0x42; // WDM
			Uop temp_WDM[4 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		FetchDummy,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End
			};
			for (int i = 0; i < 2 * 12; i++) { Microcode[op * op_length * 12 + i] = temp_WDM[i]; }

			op = 0x62; // PER
			Uop temp_PER[6 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch2,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch3,
				NOP, SKP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		NOP,
				NOP, CHS, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		PushPERH,
				NOP, CHS, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		PushPERL,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End
			};
			for (int i = 0; i < 6 * 12; i++) { Microcode[op * op_length * 12 + i] = temp_PER[i]; }
		}

		// 'direct indexed indirect' (d,x)
		void build_diix_instructions() 
		{
			op = 0x01; // ORA
			Uop temp_DIIX[8 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch2,
				NOP, SKP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		IdxInd_Stage3,
				NOP, SKP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		IdxInd_Stage3a,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		IdxInd_Stage4,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		IdxInd_Stage5,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		IdxInd_Stage6_READ,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		IdxInd_Stage6_READa,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End_ORA
			};
			num_uops = 8;
			int replace_position = 7 * 12 + 11;

			assemble_instruction(0x01, End_ORA, replace_position, &temp_DIIX[0]); // ORA
			assemble_instruction(0x21, End_AND, replace_position, &temp_DIIX[0]); // AND
			assemble_instruction(0x41, End_EOR, replace_position, &temp_DIIX[0]); // EOR
			assemble_instruction(0x61, End_ADC, replace_position, &temp_DIIX[0]); // ADC
			assemble_instruction(0xA1, End_LDA, replace_position, &temp_DIIX[0]); // LDA
			assemble_instruction(0xC1, End_CMP, replace_position, &temp_DIIX[0]); // CMP
			assemble_instruction(0xE1, End_SBC, replace_position, &temp_DIIX[0]); // SBC

			// need to change the reads to writes for STA
			temp_DIIX[5 * 12 + 11] = IdxInd_Stage6_STA;
			temp_DIIX[6 * 12 + 11] = IdxInd_Stage6_STAa;

			assemble_instruction(0x81, End, replace_position, &temp_DIIX[0]); // STA
		}

		// 'direct indirect indexed' (d),y
		void build_diiy_instructions()
		{
			Uop temp_DIIY[8 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch2,
				NOP, SKP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		IndIdx_Stage3,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		IndIdx_Stage3a,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		IndIdx_Stage4,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		IndIdx_Stage4a,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		IndIdx_Stage5,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		IndIdx_Stage5a,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End_ORA
			};
			num_uops = 8;
			int replace_position = 7 * 12 + 11;

			assemble_instruction(0x11, End_ORA, replace_position, &temp_DIIY[0]); // ORA
			assemble_instruction(0x31, End_AND, replace_position, &temp_DIIY[0]); // AND
			assemble_instruction(0x51, End_EOR, replace_position, &temp_DIIY[0]); // EOR
			assemble_instruction(0x71, End_ADC, replace_position, &temp_DIIY[0]); // ADC
			assemble_instruction(0xB1, End_LDA, replace_position, &temp_DIIY[0]); // LDA
			assemble_instruction(0xD1, End_CMP, replace_position, &temp_DIIY[0]); // CMP
			assemble_instruction(0xF1, End_SBC, replace_position, &temp_DIIY[0]); // SBC

			// need to change the reads to writes for STA
			temp_DIIY[5 * 12 + 11] = IdxInd_Stage6_STA;
			temp_DIIY[6 * 12 + 11] = IdxInd_Stage6_STAa;

			assemble_instruction(0x91, End, replace_position, &temp_DIIY[0]); // STA
		}

		// 'direct indirect' (d)
		void build_di_instructions()
		{
			Uop temp_DI[7 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch2,
				NOP, SKP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Ind_Stage3,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Ind_Stage3a,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Ind_Stage4,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Ind_Stage5,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Ind_Stage5a,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End_ORA
			};
			num_uops = 7;
			int replace_position = 6 * 12 + 11;

			assemble_instruction(0x12, End_ORA, replace_position, &temp_DI[0]); // ORA
			assemble_instruction(0x32, End_AND, replace_position, &temp_DI[0]); // AND
			assemble_instruction(0x52, End_EOR, replace_position, &temp_DI[0]); // EOR
			assemble_instruction(0x72, End_ADC, replace_position, &temp_DI[0]); // ADC
			assemble_instruction(0xB2, End_LDA, replace_position, &temp_DI[0]); // LDA
			assemble_instruction(0xD2, End_CMP, replace_position, &temp_DI[0]); // CMP
			assemble_instruction(0xF2, End_SBC, replace_position, &temp_DI[0]); // SBC

			// need to change the reads to writes for STA
			// need to change the reads to writes for STA
			temp_DI[4 * 12 + 11] = IndIdx_Stage5_STA;
			temp_DI[5 * 12 + 11] = IndIdx_Stage5_STAa;

			assemble_instruction(0x92, End, replace_position, &temp_DI[0]); // STA
		}

		// 'stack relative' d,s
		void build_ds_instructions()
		{
			// NOTE: reusing IdxInd_Stage6_READ becauae effective address is always zero page
			Uop temp_DS[5 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch2,
				NOP, SKP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		StackEA_1,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		IdxInd_Stage6_READ,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		IdxInd_Stage6_READa,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End_ORA
			};
			num_uops = 5;
			int replace_position = 4 * 12 + 11;

			assemble_instruction(0x03, End_ORA, replace_position, &temp_DS[0]); // ORA
			assemble_instruction(0x23, End_AND, replace_position, &temp_DS[0]); // AND
			assemble_instruction(0x43, End_EOR, replace_position, &temp_DS[0]); // EOR
			assemble_instruction(0x63, End_ADC, replace_position, &temp_DS[0]); // ADC
			assemble_instruction(0xA3, End_LDA, replace_position, &temp_DS[0]); // LDA
			assemble_instruction(0xC3, End_CMP, replace_position, &temp_DS[0]); // CMP
			assemble_instruction(0xE3, End_SBC, replace_position, &temp_DS[0]); // SBC

			// need to change the reads to writes for STA
			temp_DS[2 * 12 + 11] = IdxInd_Stage6_STA;
			temp_DS[3 * 12 + 11] = IdxInd_Stage6_STAa;

			assemble_instruction(0x83, End, replace_position, &temp_DS[0]); // STA
		}

		// 'stack relative indirect indexed' (d,s),y
		void build_dsii_instructions()
		{
			Uop temp_DSII[8 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch2,
				NOP, SKP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		StackEA_1,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		StkRelInd3_READ,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		StkRelInd3_READa,
				NOP, SKP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		StackEA_2,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		StkRelInd5_READ,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		StkRelInd5_READa,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End_ORA
			};
			num_uops = 8;
			int replace_position = 7 * 12 + 11;

			assemble_instruction(0x13, End_ORA, replace_position, &temp_DSII[0]); // ORA
			assemble_instruction(0x33, End_AND, replace_position, &temp_DSII[0]); // AND
			assemble_instruction(0x53, End_EOR, replace_position, &temp_DSII[0]); // EOR
			assemble_instruction(0x73, End_ADC, replace_position, &temp_DSII[0]); // ADC
			assemble_instruction(0xB3, End_LDA, replace_position, &temp_DSII[0]); // LDA
			assemble_instruction(0xD3, End_CMP, replace_position, &temp_DSII[0]); // CMP
			assemble_instruction(0xF3, End_SBC, replace_position, &temp_DSII[0]); // SBC

			// need to change the reads to writes for STA
			temp_DSII[5 * 12 + 11] = StkRelInd5_STA;
			temp_DSII[6 * 12 + 11] = StkRelInd5_STAa;

			assemble_instruction(0x93, End, replace_position, &temp_DSII[0]); // STA
		}

		// TODO: need to adjust for LDX LDY / STX STY
		// 'direct' d
		void build_d_instructions()
		{	
			Uop temp_D[5 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch2,
				NOP, SKP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		NOP,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		dir_READ,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		dir_READa,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End_ORA
			};
			num_uops = 5;
			int replace_position = 4 * 12 + 11;
			int replace_position2 = 0;

			assemble_instruction(0x05, End_ORA, replace_position, &temp_D[0]); // ORA
			assemble_instruction(0x25, End_AND, replace_position, &temp_D[0]); // AND
			assemble_instruction(0x45, End_EOR, replace_position, &temp_D[0]); // EOR
			assemble_instruction(0x65, End_ADC, replace_position, &temp_D[0]); // ADC
			assemble_instruction(0xA5, End_LDA, replace_position, &temp_D[0]); // LDA
			assemble_instruction(0xC5, End_CMP, replace_position, &temp_D[0]); // CMP
			assemble_instruction(0xE5, End_SBC, replace_position, &temp_D[0]); // SBC
			assemble_instruction(0xA4, End_LDY, replace_position, &temp_D[0]); // LDY
			assemble_instruction(0xA6, End_LDX, replace_position, &temp_D[0]); // LDX
			assemble_instruction(0x24, End_BIT, replace_position, &temp_D[0]); // BIT

			// need to change the reads to writes for STA
			temp_D[4 * 12 + 11] = End;
			replace_position = 2 * 12 + 11;
			replace_position2 = 3 * 12 + 11;

			assemble_instruction2(0x85, dir_STA, replace_position, dir_STAa, replace_position2, &temp_D[0]); // STA
			assemble_instruction2(0x84, dir_STY, replace_position, dir_STYa, replace_position2, &temp_D[0]); // STY
			assemble_instruction2(0x86, dir_STX, replace_position, dir_STXa, replace_position2, &temp_D[0]); // STX
			assemble_instruction2(0x64, dir_STZ, replace_position, dir_STZa, replace_position2, &temp_D[0]); // STZ
		}

		// TODO: need to adjust for LDX LDY / STX STY
		// 'direct x' d,x
		// 'direct y' d,y
		void build_dx_instructions()
		{
			Uop temp_DX[6 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch2,
				NOP, SKP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		NOP,
				NOP, SKP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		EA_DX,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		dir_READ,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		dir_READa,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End_ORA
			};
			num_uops = 6;
			int replace_position = 5 * 12 + 11;
			int replace_position2 = 0;

			assemble_instruction(0x15, End_ORA, replace_position, &temp_DX[0]); // ORA
			assemble_instruction(0x35, End_AND, replace_position, &temp_DX[0]); // AND
			assemble_instruction(0x55, End_EOR, replace_position, &temp_DX[0]); // EOR
			assemble_instruction(0x75, End_ADC, replace_position, &temp_DX[0]); // ADC
			assemble_instruction(0xB5, End_LDA, replace_position, &temp_DX[0]); // LDA
			assemble_instruction(0xD5, End_CMP, replace_position, &temp_DX[0]); // CMP
			assemble_instruction(0xF5, End_SBC, replace_position, &temp_DX[0]); // SBC
			assemble_instruction(0xB4, End_LDY, replace_position, &temp_DX[0]); // LDY
			assemble_instruction(0x34, End_BIT, replace_position, &temp_DX[0]); // BIT

			// need to change the reads to writes for STA
			temp_DX[5 * 12 + 11] = End;
			replace_position = 3 * 12 + 11;
			replace_position2 = 4 * 12 + 11;

			assemble_instruction2(0x95, dir_STA, replace_position, dir_STAa, replace_position2, &temp_DX[0]); // STA
			assemble_instruction2(0x94, dir_STY, replace_position, dir_STYa, replace_position2, &temp_DX[0]); // STY
			assemble_instruction2(0x74, dir_STZ, replace_position, dir_STZa, replace_position2, &temp_DX[0]); // STZ

			// change iundexing variable to Y
			temp_DX[2 * 12 + 11] = EA_DY;

			assemble_instruction2(0x95, dir_STX, replace_position, dir_STXa, replace_position2, &temp_DX[0]); // STX

			// change back to reads again
			temp_DX[5 * 12 + 11] = End_LDX;

			assemble_instruction2(0x94, dir_READ, replace_position, dir_READa, replace_position2, &temp_DX[0]); // LDX
		}

		// 'direct indirect long' [d]
		void build_dil_instructions()
		{
			Uop temp_DIL[8 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch2,
				NOP, SKP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		NOP,
				NOP, SKP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		EA_DX,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		dir_READ,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		dir_READa,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		dir_READ,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		dir_READa,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End_ORA
			};
			num_uops = 8;
			int replace_position = 7 * 12 + 11;

			assemble_instruction(0x07, End_ORA, replace_position, &temp_DIL[0]); // ORA
			assemble_instruction(0x27, End_AND, replace_position, &temp_DIL[0]); // AND
			assemble_instruction(0x47, End_EOR, replace_position, &temp_DIL[0]); // EOR
			assemble_instruction(0x67, End_ADC, replace_position, &temp_DIL[0]); // ADC
			assemble_instruction(0xA7, End_LDA, replace_position, &temp_DIL[0]); // LDA
			assemble_instruction(0xC7, End_CMP, replace_position, &temp_DIL[0]); // CMP
			assemble_instruction(0xE7, End_SBC, replace_position, &temp_DIL[0]); // SBC

			// need to change the reads to writes for STA
			temp_DIL[5 * 12 + 11] = dir_STA;
			temp_DIL[6 * 12 + 11] = dir_STAa;

			assemble_instruction(0x87, End, replace_position, &temp_DIL[0]); // STA
		}

		// 'direct indirect long y' [d],y
		void build_dily_instructions()
		{
			Uop temp_DILY[8 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch2,
				NOP, SKP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		NOP,
				NOP, SKP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		EA_DX,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		dir_READ,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		dir_READa,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		dir_READ,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		dir_READa,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End_ORA
			};
			num_uops = 8;
			int replace_position = 7 * 12 + 11;

			assemble_instruction(0x17, End_ORA, replace_position, &temp_DILY[0]); // ORA
			assemble_instruction(0x37, End_AND, replace_position, &temp_DILY[0]); // AND
			assemble_instruction(0x57, End_EOR, replace_position, &temp_DILY[0]); // EOR
			assemble_instruction(0x77, End_ADC, replace_position, &temp_DILY[0]); // ADC
			assemble_instruction(0xB7, End_LDA, replace_position, &temp_DILY[0]); // LDA
			assemble_instruction(0xD7, End_CMP, replace_position, &temp_DILY[0]); // CMP
			assemble_instruction(0xF7, End_SBC, replace_position, &temp_DILY[0]); // SBC

			// need to change the reads to writes for STA
			temp_DILY[5 * 12 + 11] = dir_STA;
			temp_DILY[6 * 12 + 11] = dir_STAa;

			assemble_instruction(0x97, End, replace_position, &temp_DILY[0]); // STA
		}

		// 'immediate' #
		void build_imm_instructions()
		{
			Uop temp_IMM[3 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Imm_READ,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Imm_READa,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End_ORA
			};
			num_uops = 3;
			int replace_position = 2 * 12 + 11;

			assemble_instruction(0x09, End_ORA, replace_position, &temp_IMM[0]); // ORA
			assemble_instruction(0x29, End_AND, replace_position, &temp_IMM[0]); // AND
			assemble_instruction(0x49, End_EOR, replace_position, &temp_IMM[0]); // EOR
			assemble_instruction(0x69, End_ADC, replace_position, &temp_IMM[0]); // ADC
			assemble_instruction(0xA9, End_LDA, replace_position, &temp_IMM[0]); // LDA
			assemble_instruction(0xC9, End_CMP, replace_position, &temp_IMM[0]); // CMP
			assemble_instruction(0xE9, End_SBC, replace_position, &temp_IMM[0]); // SBC
			assemble_instruction(0xC0, End_CPY, replace_position, &temp_IMM[0]); // CPY
			assemble_instruction(0xE0, End_CPX, replace_position, &temp_IMM[0]); // CPX
			assemble_instruction(0xA0, End_LDY, replace_position, &temp_IMM[0]); // LDY
			assemble_instruction(0xA2, End_LDX, replace_position, &temp_IMM[0]); // LDX
			assemble_instruction(0x89, End_BIT, replace_position, &temp_IMM[0]); // BIT
		}

		// 'absolute y' a,y
		// 'absolute x' a,x
		void build_ayx_instructions()
		{
			Uop temp_DAY[6 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch2,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Abs_Fetch_EA_Y,
				NOP, SKP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		NOP,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		DBR_EA_READ,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		DBR_EA_READa,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End_ORA
			};

			num_uops = 6;
			int replace_position = 5 * 12 + 11;

			assemble_instruction(0x19, End_ORA, replace_position, &temp_DAY[0]); // ORA
			assemble_instruction(0x39, End_AND, replace_position, &temp_DAY[0]); // AND
			assemble_instruction(0x59, End_EOR, replace_position, &temp_DAY[0]); // EOR
			assemble_instruction(0x79, End_ADC, replace_position, &temp_DAY[0]); // ADC
			assemble_instruction(0xB9, End_LDA, replace_position, &temp_DAY[0]); // LDA
			assemble_instruction(0xD9, End_CMP, replace_position, &temp_DAY[0]); // CMP
			assemble_instruction(0xF9, End_SBC, replace_position, &temp_DAY[0]); // SBC

			// need to change the reads to writes for STA
			temp_DAY[3 * 12 + 11] = DBR_EA_STA;
			temp_DAY[4 * 12 + 11] = DBR_EA_STAa;

			assemble_instruction(0x99, End, replace_position, &temp_DAY[0]); // STA

			temp_DAY[3 * 12 + 11] = DBR_EA_STX;
			temp_DAY[4 * 12 + 11] = DBR_EA_STXa;

			assemble_instruction(0xBE, End, replace_position, &temp_DAY[0]); // STX

			// change the indexing variable
			temp_DAY[1 * 12 + 11] = Abs_Fetch_EA_X;

			temp_DAY[3 * 12 + 11] = DBR_EA_STA;
			temp_DAY[4 * 12 + 11] = DBR_EA_STAa;

			assemble_instruction(0x9D, End, replace_position, &temp_DAY[0]); // STA

			temp_DAY[3 * 12 + 11] = DBR_EA_STZ;
			temp_DAY[4 * 12 + 11] = DBR_EA_STZa;

			assemble_instruction(0x9E, End, replace_position, &temp_DAY[0]); // STZ

			// change back to reads
			temp_DAY[3 * 12 + 11] = DBR_EA_READ;
			temp_DAY[4 * 12 + 11] = DBR_EA_READa;

			assemble_instruction(0x1D, End_ORA, replace_position, &temp_DAY[0]); // ORA
			assemble_instruction(0x3D, End_AND, replace_position, &temp_DAY[0]); // AND
			assemble_instruction(0x5D, End_EOR, replace_position, &temp_DAY[0]); // EOR
			assemble_instruction(0x7D, End_ADC, replace_position, &temp_DAY[0]); // ADC
			assemble_instruction(0xBD, End_LDA, replace_position, &temp_DAY[0]); // LDA
			assemble_instruction(0xDD, End_CMP, replace_position, &temp_DAY[0]); // CMP
			assemble_instruction(0xFD, End_SBC, replace_position, &temp_DAY[0]); // SBC
			assemble_instruction(0xBC, End_LDY, replace_position, &temp_DAY[0]); // LDY
			assemble_instruction(0x3C, End_BIT, replace_position, &temp_DAY[0]); // BIT
		}

		// 'absolute' a
		void build_a_instructions()
		{
			Uop temp_A[6 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch2,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Abs_Fetch_EA,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		DBR_EA_READ,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		DBR_EA_READa,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End_ORA
			};

			num_uops = 5;
			int replace_position = 4 * 12 + 11;

			assemble_instruction(0x0D, End_ORA, replace_position, &temp_A[0]); // ORA
			assemble_instruction(0x2D, End_AND, replace_position, &temp_A[0]); // AND
			assemble_instruction(0x4D, End_EOR, replace_position, &temp_A[0]); // EOR
			assemble_instruction(0x6D, End_ADC, replace_position, &temp_A[0]); // ADC
			assemble_instruction(0xAD, End_LDA, replace_position, &temp_A[0]); // LDA
			assemble_instruction(0xCD, End_CMP, replace_position, &temp_A[0]); // CMP
			assemble_instruction(0xED, End_SBC, replace_position, &temp_A[0]); // SBC
			assemble_instruction(0xAC, End_LDY, replace_position, &temp_A[0]); // LDY
			assemble_instruction(0xAE, End_LDX, replace_position, &temp_A[0]); // LDX
			assemble_instruction(0x2C, End_BIT, replace_position, &temp_A[0]); // BIT

			// need to change the reads to writes for STA
			temp_A[2 * 12 + 11] = DBR_EA_STA;
			temp_A[3 * 12 + 11] = DBR_EA_STAa;

			assemble_instruction(0x8D, End, replace_position, &temp_A[0]); // STA

			temp_A[2 * 12 + 11] = DBR_EA_STY;
			temp_A[3 * 12 + 11] = DBR_EA_STYa;

			assemble_instruction(0x8C, End, replace_position, &temp_A[0]); // STY

			temp_A[2 * 12 + 11] = DBR_EA_STX;
			temp_A[3 * 12 + 11] = DBR_EA_STXa;

			assemble_instruction(0x8E, End, replace_position, &temp_A[0]); // STX

			temp_A[2 * 12 + 11] = DBR_EA_STZ;
			temp_A[3 * 12 + 11] = DBR_EA_STZa;

			assemble_instruction(0x9C, End, replace_position, &temp_A[0]); // STZ
		}

		// 'absolute long' al
		void build_al_instructions()
		{
			Uop temp_AL[6 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch2,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch3,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Absl_Fetch_EA,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Absl_READ,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Absl_READa,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End_ORA
			};

			num_uops = 6;
			int replace_position = 5 * 12 + 11;

			assemble_instruction(0x0F, End_ORA, replace_position, &temp_AL[0]); // ORA
			assemble_instruction(0x2F, End_AND, replace_position, &temp_AL[0]); // AND
			assemble_instruction(0x4F, End_EOR, replace_position, &temp_AL[0]); // EOR
			assemble_instruction(0x6F, End_ADC, replace_position, &temp_AL[0]); // ADC
			assemble_instruction(0xAF, End_LDA, replace_position, &temp_AL[0]); // LDA
			assemble_instruction(0xCF, End_CMP, replace_position, &temp_AL[0]); // CMP
			assemble_instruction(0xEF, End_SBC, replace_position, &temp_AL[0]); // SBC

			// need to change the reads to writes for STA
			temp_AL[3 * 12 + 11] = Absl_STA;
			temp_AL[4 * 12 + 11] = Absl_STAa;

			assemble_instruction(0x8F, End, replace_position, &temp_AL[0]); // STA
		}

		// 'absolute long x' al,x
		void build_alx_instructions()
		{
			Uop temp_ALX[6 * 12] =
			{
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch2,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Fetch3,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Abslx_Fetch_EA,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Absl_READ,
				NOP, CHE, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		Absl_READa,
				NOP, CHP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP,		End_ORA
			};

			num_uops = 6;
			int replace_position = 5 * 12 + 11;

			assemble_instruction(0x1F, End_ORA, replace_position, &temp_ALX[0]); // ORA
			assemble_instruction(0x3F, End_AND, replace_position, &temp_ALX[0]); // AND
			assemble_instruction(0x5F, End_EOR, replace_position, &temp_ALX[0]); // EOR
			assemble_instruction(0x7F, End_ADC, replace_position, &temp_ALX[0]); // ADC
			assemble_instruction(0xBF, End_LDA, replace_position, &temp_ALX[0]); // LDA
			assemble_instruction(0xDF, End_CMP, replace_position, &temp_ALX[0]); // CMP
			assemble_instruction(0xFF, End_SBC, replace_position, &temp_ALX[0]); // SBC

			// need to change the reads to writes for STA
			temp_ALX[3 * 12 + 11] = Absl_STA;
			temp_ALX[4 * 12 + 11] = Absl_STAa;

			assemble_instruction(0x9F, End, replace_position, &temp_ALX[0]); // STA
		}

		void assemble_instruction(int op, Uop to_be_replaced, int position, Uop *temp_array) 
		{
			temp_array[position] = to_be_replaced;
			for (int i = 0; i < num_uops * 12; i++) { Microcode[op * op_length * 12 + i] = temp_array[i]; }
		}

		void assemble_instruction2(int op, Uop to_be_replaced, int position, Uop to_be_replaced2, int position2, Uop* temp_array)
		{
			temp_array[position] = to_be_replaced;
			temp_array[position2] = to_be_replaced2;
			for (int i = 0; i < num_uops * 12; i++) { Microcode[op * op_length * 12 + i] = temp_array[i]; }
		}

		#pragma endregion
	};
}
