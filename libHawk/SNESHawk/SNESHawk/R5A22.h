#include <iostream>
#include <cstdint>
#include <iomanip>
#include <string>

using namespace std;

namespace SNESHawk
{
	class MemoryManager;
	
	class R5A22
	{
	public:
		#pragma region Variable Declarations

		// pointer to controlling memory manager goes here
		// this will be iplementation dependent
		MemoryManager* mem_ctrl;

		void WriteMemory(uint32_t, uint8_t);
		uint8_t ReadMemory(uint32_t);
		uint8_t DummyReadMemory(uint32_t);
		uint8_t PeekMemory(uint32_t);

		// State variables

		// variables for operations
		bool branch_taken;
		bool my_iflag;
		bool booltemp;
		bool BCD_Enabled;
		uint8_t value8, temp8;
		uint16_t value16;
		int32_t tempint;
		int32_t lo, hi;

		// CPU
		uint8_t A;
		uint8_t X;
		uint8_t Y;
		uint8_t P;
		uint8_t S;
		uint16_t PC;

		//opcode bytes.. theoretically redundant with the temp variables? who knows.
		bool RDY;
		bool NMI;
		bool IRQ;
		bool iflag_pending; //iflag must be stored after it is checked in some cases (CLI and SEI).
		bool rdy_freeze; //true if the CPU must be frozen
		//tracks whether an interrupt condition has popped up recently.
		//not sure if this is real or not but it helps with the branch_irq_hack
		bool interrupt_pending;
		bool branch_irq_hack; //see Uop.RelBranch_Stage3 for more details		
		uint8_t opcode2, opcode3;
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
		const uint16_t IRQVector = 0xFFFE;
		
		// operations that can take place in an instruction
		enum Uop
		{
			Fetch1, Fetch1_Real, Fetch2, Fetch3,
			//used by instructions with no second opcode byte (6502 fetches a byte anyway but won't increment PC for these)
			FetchDummy,

			NOP,

			JSR,
			IncPC, //from RTS

			//[absolute WRITE]
			Abs_WRITE_STA, Abs_WRITE_STX, Abs_WRITE_STY,
			//[absolute READ]
			Abs_READ_BIT, Abs_READ_LDA, Abs_READ_LDY, Abs_READ_ORA, Abs_READ_LDX, Abs_READ_CMP, Abs_READ_ADC, Abs_READ_CPX, Abs_READ_SBC, Abs_READ_AND, Abs_READ_EOR, Abs_READ_CPY, Abs_READ_NOP,
			//[absolute RMW]
			Abs_RMW_Stage4, Abs_RMW_Stage6,
			Abs_RMW_Stage5_INC, Abs_RMW_Stage5_DEC, Abs_RMW_Stage5_LSR, Abs_RMW_Stage5_ROL, Abs_RMW_Stage5_ASL, Abs_RMW_Stage5_ROR,

			//[absolute JUMP]
			JMP_abs,

			//[zero page misc]
			ZpIdx_Stage3_X, ZpIdx_Stage3_Y,
			ZpIdx_RMW_Stage4, ZpIdx_RMW_Stage6,
			//[zero page WRITE]
			ZP_WRITE_STA, ZP_WRITE_STX, ZP_WRITE_STY, ZP_WRITE_SAX,
			//[zero page RMW]
			ZP_RMW_Stage3, ZP_RMW_Stage5,
			ZP_RMW_DEC, ZP_RMW_INC, ZP_RMW_ASL, ZP_RMW_LSR, ZP_RMW_ROR, ZP_RMW_ROL,
			//[zero page READ]
			ZP_READ_EOR, ZP_READ_BIT, ZP_READ_ORA, ZP_READ_LDA, ZP_READ_LDY, ZP_READ_LDX, ZP_READ_CPX, ZP_READ_SBC, ZP_READ_CPY, ZP_READ_NOP, ZP_READ_ADC, ZP_READ_AND, ZP_READ_CMP, ZP_READ_LAX,

			//[indexed indirect READ] (addr,X)
			//[indexed indirect WRITE] (addr,X)
			IdxInd_Stage3, IdxInd_Stage4, IdxInd_Stage5,
			IdxInd_Stage6_READ_ORA, IdxInd_Stage6_READ_SBC, IdxInd_Stage6_READ_LDA, IdxInd_Stage6_READ_EOR, IdxInd_Stage6_READ_CMP, IdxInd_Stage6_READ_ADC, IdxInd_Stage6_READ_AND,
			IdxInd_Stage6_READ_LAX,
			IdxInd_Stage6_WRITE_STA, IdxInd_Stage6_WRITE_SAX,
			IdxInd_Stage6_RMW, //work happens in stage 7
			IdxInd_Stage8_RMW,

			//[absolute indexed]
			AbsIdx_Stage3_X, AbsIdx_Stage3_Y, AbsIdx_Stage4,
			//[absolute indexed WRITE]
			AbsIdx_WRITE_Stage5_STA,
			AbsIdx_WRITE_Stage5_ERROR,
			//[absolute indexed READ]
			AbsIdx_READ_Stage4,
			AbsIdx_READ_Stage5_LDA, AbsIdx_READ_Stage5_CMP, AbsIdx_READ_Stage5_SBC, AbsIdx_READ_Stage5_ADC, AbsIdx_READ_Stage5_EOR, AbsIdx_READ_Stage5_LDX, AbsIdx_READ_Stage5_AND, AbsIdx_READ_Stage5_ORA, AbsIdx_READ_Stage5_LDY, AbsIdx_READ_Stage5_NOP,
			AbsIdx_READ_Stage5_ERROR,
			//[absolute indexed RMW]
			AbsIdx_RMW_Stage5, AbsIdx_RMW_Stage7,
			AbsIdx_RMW_Stage6_ROR, AbsIdx_RMW_Stage6_DEC, AbsIdx_RMW_Stage6_INC, AbsIdx_RMW_Stage6_ASL, AbsIdx_RMW_Stage6_LSR, AbsIdx_RMW_Stage6_ROL,

			IncS, DecS,
			PushPCL, PushPCH, PushP, PullP, PullPCL, PullPCH_NoInc, PushA, PullA_NoInc, PullP_NoInc,
			PushP_BRK, PushP_NMI, PushP_IRQ, PushP_Reset, PushDummy,
			FetchPCLVector, FetchPCHVector, //todo - may not need these ?? can reuse fetch2 and fetch3?

			//[implied] and [accumulator]
			Imp_ASL_A, Imp_ROL_A, Imp_ROR_A, Imp_LSR_A,
			Imp_SEC, Imp_CLI, Imp_SEI, Imp_CLD, Imp_CLC, Imp_CLV, Imp_SED,
			Imp_INY, Imp_DEY, Imp_INX, Imp_DEX,
			Imp_TSX, Imp_TXS, Imp_TAX, Imp_TAY, Imp_TYA, Imp_TXA,

			//[immediate]
			Imm_CMP, Imm_ADC, Imm_AND, Imm_SBC, Imm_ORA, Imm_EOR, Imm_CPY, Imm_CPX, Imm_ANC, Imm_ASR, Imm_ARR, Imm_AXS,
			Imm_LDA, Imm_LDX, Imm_LDY,

			//sub-ops
			NZ_X, NZ_Y, NZ_A,
			RelBranch_Stage2_BNE, RelBranch_Stage2_BPL, RelBranch_Stage2_BCC, RelBranch_Stage2_BCS, RelBranch_Stage2_BEQ, RelBranch_Stage2_BMI, RelBranch_Stage2_BVC, RelBranch_Stage2_BVS, RelBranch_Stage2_A,
			RelBranch_Stage2, RelBranch_Stage3, RelBranch_Stage4,
			_Eor, _Bit, _Cpx, _Cpy, _Cmp, _Adc, _Sbc, _Ora, _And, _Anc, _Asr, _Arr, _Axs, //alu-related sub-ops

			//JMP (addr) 0x6C
			AbsInd_JMP_Stage4, AbsInd_JMP_Stage5,

			//[indirect indexed] (i.e. LDA (addr),Y	)
			IndIdx_Stage3, IndIdx_Stage4, IndIdx_READ_Stage5, IndIdx_WRITE_Stage5,
			IndIdx_WRITE_Stage6_STA, IndIdx_WRITE_Stage6_SHA,
			IndIdx_READ_Stage6_LDA, IndIdx_READ_Stage6_CMP, IndIdx_READ_Stage6_ORA, IndIdx_READ_Stage6_SBC, IndIdx_READ_Stage6_ADC, IndIdx_READ_Stage6_AND, IndIdx_READ_Stage6_EOR,
			IndIdx_READ_Stage6_LAX,
			IndIdx_RMW_Stage5,
			IndIdx_RMW_Stage6, //just reads from effective address
			IndIdx_RMW_Stage8,

			Ind_zp_Stage4,

			End,
			End_ISpecial, //same as end, but preserves the iflag set by the instruction
			End_BranchSpecial,
			End_SuppressInterrupt,

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
		Uop Microcode[(256 + 8) * 12]  =
		{
			//0x00
			Fetch2,					PushPCH,			PushPCL,				PushP_BRK,					FetchPCLVector,				FetchPCHVector,				End_SuppressInterrupt,		NOP,		NOP,		NOP,		NOP,		NOP,	/*BRK [implied]*/ 
			Fetch2,					IdxInd_Stage3,		IdxInd_Stage4,			IdxInd_Stage5,				IdxInd_Stage6_READ_ORA,		End ,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ORA (addr,X) [indexed indirect READ]*/ 
			Fetch2,					End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP [immediate]*/ 
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					ZP_READ_ORA,		End ,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ORA zp [zero page READ]*/ 
			Fetch2,					ZP_RMW_Stage3,		ZP_RMW_ASL,				ZP_RMW_Stage5,				End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ASL zp [zero page RMW]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			FetchDummy,				PushP,				End ,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*PHP [implied]*/ 
			Imm_ORA,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ORA #nn [immediate]*/ 
			Imp_ASL_A,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ASL A [accumulator]*/ 
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					Fetch3,				Abs_READ_ORA,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ORA addr [absolute READ]*/
			Fetch2,					Fetch3,				Abs_RMW_Stage4,			Abs_RMW_Stage5_ASL,			Abs_RMW_Stage6,				End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ASL addr [absolute RMW]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			//0x10
			RelBranch_Stage2_BPL,	End ,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*BPL +/-rel*/ 
			Fetch2,					IndIdx_Stage3,		IndIdx_Stage4,			IndIdx_READ_Stage5,			IndIdx_READ_Stage6_ORA,		End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ORA (addr),Y* [indirect indexed READ]*/
			Fetch2,					IndIdx_Stage3,		Ind_zp_Stage4,			IndIdx_READ_Stage6_ORA,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ORA (zp) [indirect zp READ]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					ZpIdx_Stage3_X,		ZP_READ_NOP,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP zp,X [zero page indexed READ]*/
			Fetch2,					ZpIdx_Stage3_X,		ZP_READ_ORA,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ORA zp,X [zero page indexed READ]*/
			Fetch2,					ZpIdx_Stage3_X,		ZpIdx_RMW_Stage4,		ZP_RMW_ASL,					ZpIdx_RMW_Stage6,			End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ASL zp,X [zero page indexed RMW]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Imp_CLC,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CLC [implied]*/ 
			Fetch2,					AbsIdx_Stage3_Y,	AbsIdx_READ_Stage4,		AbsIdx_READ_Stage5_ORA,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ORA addr,Y* [absolute indexed READ Y]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					AbsIdx_Stage3_X,	AbsIdx_READ_Stage4,		AbsIdx_READ_Stage5_ORA,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ORA addr,X* [absolute indexed READ X]*/
			Fetch2,					AbsIdx_Stage3_X,	AbsIdx_Stage4,			AbsIdx_RMW_Stage5,			AbsIdx_RMW_Stage6_ASL,		AbsIdx_RMW_Stage7,			End,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ASL addr,X [absolute indexed RMW X]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			//0x20
			Fetch2,					NOP,				PushPCH,				PushPCL,					JSR,						End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*JSR*/
			Fetch2,					IdxInd_Stage3,		IdxInd_Stage4,			IdxInd_Stage5,				IdxInd_Stage6_READ_AND,		End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*AND (addr,X) [indexed indirect READ]*/
			Fetch2,					End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP [immediate]*/ 
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					ZP_READ_BIT,		End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*BIT zp [zero page READ]*/
			Fetch2,					ZP_READ_AND,		End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*AND zp [zero page READ]*/
			Fetch2,					ZP_RMW_Stage3,		ZP_RMW_ROL,				ZP_RMW_Stage5,				End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ROL zp [zero page RMW]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			FetchDummy,				IncS,				PullP_NoInc,			End_ISpecial,				NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*PLP [implied] */
			Imm_AND,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*AND #nn [immediate]*/
			Imp_ROL_A,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ROL A [accumulator]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					Fetch3,				Abs_READ_BIT,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*BIT addr [absolute]*/
			Fetch2,					Fetch3,				Abs_READ_AND,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*AND addr [absolute READ]*/
			Fetch2,					Fetch3,				Abs_RMW_Stage4,			Abs_RMW_Stage5_ROL,			Abs_RMW_Stage6,				End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ROL addr [absolute RMW]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			//0x30,
			RelBranch_Stage2_BMI,	End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*BMI +/-rel [relative]*/
			Fetch2,					IndIdx_Stage3,		IndIdx_Stage4,			IndIdx_READ_Stage5,			IndIdx_READ_Stage6_AND,		End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*AND (addr),Y* [indirect indexed READ]*/
			Fetch2,					IndIdx_Stage3,		Ind_zp_Stage4,			IndIdx_READ_Stage6_AND,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*AND (zp) [indirect zp READ]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					ZpIdx_Stage3_X,		ZP_READ_AND,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*AND zp,X [zero page indexed READ]*/
			Fetch2,					ZpIdx_Stage3_X,		ZpIdx_RMW_Stage4,		ZP_RMW_ROL,					ZpIdx_RMW_Stage6,			End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ROL zp,X [zero page indexed RMW]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Imp_SEC,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*SEC [implied]*/
			Fetch2,					AbsIdx_Stage3_Y,	AbsIdx_READ_Stage4,		AbsIdx_READ_Stage5_AND,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*AND addr,Y* [absolute indexed READ Y]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					AbsIdx_Stage3_X,	AbsIdx_READ_Stage4,		AbsIdx_READ_Stage5_AND,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*AND addr,X* [absolute indexed READ X]*/
			Fetch2,					AbsIdx_Stage3_X,	AbsIdx_Stage4,			AbsIdx_RMW_Stage5,			AbsIdx_RMW_Stage6_ROL,		AbsIdx_RMW_Stage7,			End,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ROL addr,X [absolute indexed RMW X]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			//0x40,
			FetchDummy,				IncS,				PullP,					PullPCL,					PullPCH_NoInc,				End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*RTI*/
			Fetch2,					IdxInd_Stage3,		IdxInd_Stage4,			IdxInd_Stage5,				IdxInd_Stage6_READ_EOR,		End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*EOR (addr,X) [indexed indirect READ]*/
			Fetch2,					End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP [immediate]*/ 
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					ZP_READ_EOR,		End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*EOR zp [zero page READ]*/
			Fetch2,					ZP_RMW_Stage3,		ZP_RMW_LSR,				ZP_RMW_Stage5,				End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LSR zp [zero page RMW]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			FetchDummy,				PushA,				End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*PHA [implied]*/
			Imm_EOR,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*EOR #nn [immediate]*/
			Imp_LSR_A,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LSR A [accumulator]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					JMP_abs,			End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*JMP addr [absolute]*/
			Fetch2,					Fetch3,				Abs_READ_EOR,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*EOR addr [absolute READ]*/
			Fetch2,					Fetch3,				Abs_RMW_Stage4,			Abs_RMW_Stage5_LSR,			Abs_RMW_Stage6,				End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LSR addr [absolute RMW]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			//0x50,
			RelBranch_Stage2_BVC,	End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*BVC +/-rel [relative]*/
			Fetch2,					IndIdx_Stage3,		IndIdx_Stage4,			IndIdx_READ_Stage5,			IndIdx_READ_Stage6_EOR,		End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*EOR (addr),Y* [indirect indexed READ]*/
			Fetch2,					IndIdx_Stage3,		Ind_zp_Stage4,			IndIdx_READ_Stage6_EOR,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*EOR (zp) [indirect zp READ]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					ZpIdx_Stage3_X,		ZP_READ_EOR,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*EOR zp,X [zero page indexed READ]*/
			Fetch2,					ZpIdx_Stage3_X,		ZpIdx_RMW_Stage4,		ZP_RMW_LSR,					ZpIdx_RMW_Stage6,			End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LSR zp,X [zero page indexed RMW]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Imp_CLI,				End_ISpecial,		NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CLI [implied]*/
			Fetch2,					AbsIdx_Stage3_Y,	AbsIdx_READ_Stage4,		AbsIdx_READ_Stage5_EOR,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*EOR addr,Y* [absolute indexed READ Y]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					AbsIdx_Stage3_X,	AbsIdx_READ_Stage4,		AbsIdx_READ_Stage5_EOR,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*EOR addr,X* [absolute indexed READ X]*/
			Fetch2,					AbsIdx_Stage3_X,	AbsIdx_Stage4,			AbsIdx_RMW_Stage5,			AbsIdx_RMW_Stage6_LSR,		AbsIdx_RMW_Stage7,			End,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LSR addr,X [absolute indexed RMW X]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			//0x60,
			FetchDummy,				IncS,				PullPCL,				PullPCH_NoInc,				IncPC,						End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*RTS*/	//can't fetch here because the PC isnt ready until the end of the last clock
			Fetch2,					IdxInd_Stage3,		IdxInd_Stage4,			IdxInd_Stage5,				IdxInd_Stage6_READ_ADC,		End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ADC (addr,X) [indexed indirect READ]*/
			Fetch2,					End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP [immediate]*/ 
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					ZP_READ_ADC,		End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ADC zp [zero page READ]*/
			Fetch2,					ZP_RMW_Stage3,		ZP_RMW_ROR,				ZP_RMW_Stage5,				End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ROR zp [zero page RMW]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			FetchDummy,				IncS,				PullA_NoInc,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*PLA [implied]*/
			Imm_ADC,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ADC #nn [immediate]*/
			Imp_ROR_A,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ROR A [accumulator]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					Fetch3,				AbsInd_JMP_Stage4,		AbsInd_JMP_Stage5,			End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*JMP (addr) [absolute indirect JMP]*/
			Fetch2,					Fetch3,				Abs_READ_ADC,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ADC addr [absolute READ]*/
			Fetch2,					Fetch3,				Abs_RMW_Stage4,			Abs_RMW_Stage5_ROR,			Abs_RMW_Stage6,				End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ROR addr [absolute RMW]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			//0x70,
			RelBranch_Stage2_BVS,	End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*BVS +/-rel [relative]*/
			Fetch2,					IndIdx_Stage3,		IndIdx_Stage4,			IndIdx_READ_Stage5,			IndIdx_READ_Stage6_ADC,		End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ADC (addr),Y [indirect indexed READ]*/
			Fetch2,					IndIdx_Stage3,		Ind_zp_Stage4,			IndIdx_READ_Stage6_ADC,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ADC (zp) [indirect zp READ]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					ZpIdx_Stage3_X,		ZP_READ_ADC,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ADC zp,X [zero page indexed READ]*/
			Fetch2,					ZpIdx_Stage3_X,		ZpIdx_RMW_Stage4,		ZP_RMW_ROR,					ZpIdx_RMW_Stage6,			End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ROR zp,X [zero page indexed RMW]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Imp_SEI,				End_ISpecial,		NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*SEI [implied]*/
			Fetch2,					AbsIdx_Stage3_Y,	AbsIdx_READ_Stage4,		AbsIdx_READ_Stage5_ADC,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ADC addr,Y* [absolute indexed READ Y]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					AbsIdx_Stage3_X,	AbsIdx_READ_Stage4,		AbsIdx_READ_Stage5_ADC,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ADC addr,X* [absolute indexed READ X]*/
			Fetch2,					AbsIdx_Stage3_X,	AbsIdx_Stage4,			AbsIdx_RMW_Stage5,			AbsIdx_RMW_Stage6_ROR,		AbsIdx_RMW_Stage7,			End,						NOP,		NOP,		NOP,		NOP,		NOP,	/*ROR addr,X [absolute indexed RMW X]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			//0x80,
			RelBranch_Stage2_A,		End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*BRA +/-rel [relative]*/
			Fetch2,					IdxInd_Stage3,		IdxInd_Stage4,			IdxInd_Stage5,				IdxInd_Stage6_WRITE_STA,	End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*STA (addr,X) [indexed indirect WRITE]*/
			Fetch2,					End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP [immediate]*/ 
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					ZP_WRITE_STY,		End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*STY zp [zero page WRITE]*/
			Fetch2,					ZP_WRITE_STA,		End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*STA zp [zero page WRITE]*/
			Fetch2,					ZP_WRITE_STX,		End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*STX zp [zero page WRITE]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Imp_DEY,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*DEY [implied]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Imp_TXA,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*TXA [implied]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					Fetch3,				Abs_WRITE_STY,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*STY addr [absolute WRITE]*/
			Fetch2,					Fetch3,				Abs_WRITE_STA,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*STA addr [absolute WRITE]*/
			Fetch2,					Fetch3,				Abs_WRITE_STX,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*STX addr [absolute WRITE]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			//0x90,
			RelBranch_Stage2_BCC,	End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*BCC +/-rel [relative]*/
			Fetch2,					IndIdx_Stage3,		IndIdx_Stage4,			IndIdx_WRITE_Stage5,		IndIdx_WRITE_Stage6_STA,	End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*STA (addr),Y [indirect indexed WRITE]*/
			Fetch2,					IndIdx_Stage3,		Ind_zp_Stage4,			IndIdx_READ_Stage6_STA,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*STA (zp) [indirect zp READ]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					ZpIdx_Stage3_X,		ZP_WRITE_STY,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*STY zp,X [zero page indexed WRITE X]*/
			Fetch2,					ZpIdx_Stage3_X,		ZP_WRITE_STA,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*STA zp,X [zero page indexed WRITE X]*/
			Fetch2,					ZpIdx_Stage3_Y,		ZP_WRITE_STX,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*STX zp,Y [zero page indexed WRITE Y]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Imp_TYA,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*TYA [implied]*/
			Fetch2,					AbsIdx_Stage3_Y,	AbsIdx_Stage4,			AbsIdx_WRITE_Stage5_STA,	End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*STA addr,Y [absolute indexed WRITE]*/
			Imp_TXS,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*TXS [implied]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					AbsIdx_Stage3_X,	AbsIdx_Stage4,			AbsIdx_WRITE_Stage5_STA,	End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*STA addr,X [absolute indexed WRITE]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			//0xA0,
			Imm_LDY,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDY #nn [immediate]*/
			Fetch2,					IdxInd_Stage3,		IdxInd_Stage4,			IdxInd_Stage5,				IdxInd_Stage6_READ_LDA,		End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDA (addr,X) [indexed indirect READ]*/
			Imm_LDX,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDX #nn [immediate]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					ZP_READ_LDY,		End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDY zp [zero page READ]*/
			Fetch2,					ZP_READ_LDA,		End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDA zp [zero page READ]*/
			Fetch2,					ZP_READ_LDX,		End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDX zp [zero page READ]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Imp_TAY,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*TAY [implied]*/
			Imm_LDA,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDA #nn [immediate]*/
			Imp_TAX,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*TAX [implied]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					Fetch3,				Abs_READ_LDY,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDY addr [absolute READ]*/
			Fetch2,					Fetch3,				Abs_READ_LDA,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDA addr [absolute READ]*/
			Fetch2,					Fetch3,				Abs_READ_LDX,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDX addr [absolute READ]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			//0xB0,
			RelBranch_Stage2_BCS,	End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*BCS +/-rel [relative]*/
			Fetch2,					IndIdx_Stage3,		IndIdx_Stage4,			IndIdx_READ_Stage5,			IndIdx_READ_Stage6_LDA,		End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDA (addr),Y* [indirect indexed READ]*/
			Fetch2,					IndIdx_Stage3,		Ind_zp_Stage4,			IndIdx_READ_Stage6_LDA,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDA (zp) [indirect zp READ]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					ZpIdx_Stage3_X,		ZP_READ_LDY,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDY zp,X [zero page indexed READ X]*/
			Fetch2,					ZpIdx_Stage3_X,		ZP_READ_LDA,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDA zp,X [zero page indexed READ X]*/
			Fetch2,					ZpIdx_Stage3_Y,		ZP_READ_LDX,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDX zp,Y [zero page indexed READ Y]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Imp_CLV,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CLV [implied]*/
			Fetch2,					AbsIdx_Stage3_Y,	AbsIdx_READ_Stage4,		AbsIdx_READ_Stage5_LDA,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDA addr,Y* [absolute indexed READ Y]*/
			Imp_TSX,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*TSX [implied]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					AbsIdx_Stage3_X,	AbsIdx_READ_Stage4,		AbsIdx_READ_Stage5_LDY,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDY addr,X* [absolute indexed READ X]*/
			Fetch2,					AbsIdx_Stage3_X,	AbsIdx_READ_Stage4,		AbsIdx_READ_Stage5_LDA,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDA addr,X* [absolute indexed READ X]*/
			Fetch2,					AbsIdx_Stage3_Y,	AbsIdx_READ_Stage4,		AbsIdx_READ_Stage5_LDX,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*LDX addr,Y* [absolute indexed READ Y]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			//0xC0,
			Imm_CPY,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CPY #nn [immediate]*/
			Fetch2,					IdxInd_Stage3,		IdxInd_Stage4,			IdxInd_Stage5,				IdxInd_Stage6_READ_CMP,		End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CMP (addr,X) [indexed indirect READ]*/
			Fetch2,					End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP [immediate]*/ 
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					ZP_READ_CPY,		End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CPY zp [zero page READ]*/
			Fetch2,					ZP_READ_CMP,		End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CMP zp [zero page READ]*/
			Fetch2,					ZP_RMW_Stage3,		ZP_RMW_DEC,				ZP_RMW_Stage5,				End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*DEC zp [zero page RMW]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Imp_INY,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*INY [implied]*/
			Imm_CMP,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CMP #nn [immediate]*/
			Imp_DEX,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*DEX  [implied]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					Fetch3,				Abs_READ_CPY,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CPY addr [absolute READ]*/
			Fetch2,					Fetch3,				Abs_READ_CMP,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CMP addr [absolute READ]*/
			Fetch2,					Fetch3,				Abs_RMW_Stage4,			Abs_RMW_Stage5_DEC,			Abs_RMW_Stage6,				End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*DEC addr [absolute RMW]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			//0xD0,
			RelBranch_Stage2_BNE,	End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*BNE +/-rel [relative]*/
			Fetch2,					IndIdx_Stage3,		IndIdx_Stage4,			IndIdx_READ_Stage5,			IndIdx_READ_Stage6_CMP,		End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CMP (addr),Y* [indirect indexed READ]*/
			Fetch2,					IndIdx_Stage3,		Ind_zp_Stage4,			IndIdx_READ_Stage6_CMP,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CMP (zp) [indirect zp READ]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					ZpIdx_Stage3_X,		ZP_READ_CMP,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CMP zp,X [zero page indexed READ]*/
			Fetch2,					ZpIdx_Stage3_X,		ZpIdx_RMW_Stage4,		ZP_RMW_DEC,					ZpIdx_RMW_Stage6,			End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*DEC zp,X [zero page indexed RMW X]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Imp_CLD,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CLD [implied]*/
			Fetch2,					AbsIdx_Stage3_Y,	AbsIdx_READ_Stage4,		AbsIdx_READ_Stage5_CMP,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CMP addr,Y* [absolute indexed READ Y]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					AbsIdx_Stage3_X,	AbsIdx_READ_Stage4,		AbsIdx_READ_Stage5_CMP,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CMP addr,X* [absolute indexed READ X]*/
			Fetch2,					AbsIdx_Stage3_X,	AbsIdx_Stage4,			AbsIdx_RMW_Stage5,			AbsIdx_RMW_Stage6_DEC,		AbsIdx_RMW_Stage7,			End,						NOP,		NOP,		NOP,		NOP,		NOP,	/*DEC addr,X [absolute indexed RMW X]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			//0xE0,
			Imm_CPX,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CPX #nn [immediate]*/
			Fetch2,					IdxInd_Stage3,		IdxInd_Stage4,			IdxInd_Stage5,				IdxInd_Stage6_READ_SBC,		End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*SBC (addr,X) [indirect indexed]*/
			Fetch2,					End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP [immediate]*/ 
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					ZP_READ_CPX,		End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CPX zp [zero page READ]*/
			Fetch2,					ZP_READ_SBC,		End,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*SBC zp [zero page READ]*/
			Fetch2,					ZP_RMW_Stage3,		ZP_RMW_INC,				ZP_RMW_Stage5,				End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*INC zp [zero page RMW]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Imp_INX,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*INX [implied]*/
			Imm_SBC,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*SBC #nn [immediate READ]*/
			FetchDummy,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP EA [implied]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					Fetch3,				Abs_READ_CPX,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*CPX addr [absolute READ]*/
			Fetch2,					Fetch3,				Abs_READ_SBC,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*SBC addr [absolute READ]*/
			Fetch2,					Fetch3,				Abs_RMW_Stage4,			Abs_RMW_Stage5_INC,			Abs_RMW_Stage6,				End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*INC addr [absolute RMW]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			//0xF0,
			RelBranch_Stage2_BEQ,	End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*BEQ +/-rel [relative]*/
			Fetch2,					IndIdx_Stage3,		IndIdx_Stage4,			IndIdx_READ_Stage5,			IndIdx_READ_Stage6_SBC,		End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*SBC (addr),Y* [indirect indexed READ]*/
			Fetch2,					IndIdx_Stage3,		Ind_zp_Stage4,			IndIdx_READ_Stage6_SBC,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*SBC (zp) [indirect zp READ]*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					ZpIdx_Stage3_X,		ZP_READ_SBC,			End,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*SBC zp,X [zero page indexed READ X]*/
			Fetch2,					ZpIdx_Stage3_X,		ZpIdx_RMW_Stage4,		ZP_RMW_INC,					ZpIdx_RMW_Stage6,			End,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*INC zp,X [zero page indexed RMW X]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Imp_SED,				End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*SED [implied]*/
			Fetch2,					AbsIdx_Stage3_Y,	AbsIdx_READ_Stage4,		AbsIdx_READ_Stage5_SBC,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*SBC addr,Y* [absolute indexed READ Y]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			End,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			Fetch2,					AbsIdx_Stage3_X,	AbsIdx_READ_Stage4,		AbsIdx_READ_Stage5_SBC,		End,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*SBC addr,X* [absolute indexed READ X]*/
			Fetch2,					AbsIdx_Stage3_X,	AbsIdx_Stage4,			AbsIdx_RMW_Stage5,			AbsIdx_RMW_Stage6_INC,		AbsIdx_RMW_Stage7,			End,						NOP,		NOP,		NOP,		NOP,		NOP,	/*INC addr,X [absolute indexed RMW X]*/
			NOP,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*NOP*/
			//0x100,
			Fetch1,					NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*VOP_Fetch1*/
			RelBranch_Stage3,		End_BranchSpecial,	NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*VOP_RelativeStuff*/
			RelBranch_Stage4,		End,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*VOP_RelativeStuff2*/
			End_SuppressInterrupt,	NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*VOP_RelativeStuff2*/
			//i assume these are dummy fetches.... maybe theyre just nops? supposedly these take 7 cycles so that's the only way i can make sense of it,
			//one of them might be the next instruction's fetch,		and whatever fetch follows it.
			//the interrupt would then take place if necessary,		using a cached PC. but im not so sure about that.
			FetchDummy,				FetchDummy,			PushPCH,				PushPCL,					PushP_NMI,					FetchPCLVector,				FetchPCHVector,				End_SuppressInterrupt,		NOP,	NOP,	NOP,	NOP,	/*VOP_NMI*/
			FetchDummy,				FetchDummy,			PushPCH,				PushPCL,					PushP_IRQ,					FetchPCLVector,				FetchPCHVector,				End_SuppressInterrupt,		NOP,	NOP,	NOP,	NOP,	/*VOP_IRQ*/
			FetchDummy,				FetchDummy,			PushDummy,				PushDummy,					PushP_Reset,				FetchPCLVector,				FetchPCHVector,				End_SuppressInterrupt,		NOP,	NOP,	NOP,	NOP,	/*VOP_RESET*/
			Fetch1_Real,			NOP,				NOP,					NOP,						NOP,						NOP,						NOP,						NOP,		NOP,		NOP,		NOP,		NOP,	/*VOP_Fetch1_NoInterrupt*/

		};

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

		inline bool FlagBget() { return (P & 0x10) != 0; };
		inline void FlagBset(bool value) { P = (uint8_t)((P & ~0x10) | (value ? 0x10 : 0x00)); }

		inline bool FlagTget() { return (P & 0x20) != 0; };
		inline void FlagTset(bool value) { P = (uint8_t)((P & ~0x20) | (value ? 0x20 : 0x00)); }

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
			P = 0x20; // 5th bit always set
			S = 0;
			PC = 0;
			TotalExecutedCycles = 0;
			mi = 0;
			opcode = VOP_RESET;
			iflag_pending = true;
			RDY = true;
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
			case FetchDummy: FetchDummy_F(); break;
			case PushPCH: PushPCH_F(); break;
			case PushPCL: PushPCL_F(); break;
			case PushP_BRK: PushP_BRK_F(); break;
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
			case Abs_WRITE_STA: Abs_WRITE_STA_F(); break;
			case Abs_WRITE_STX: Abs_WRITE_STX_F(); break;
			case Abs_WRITE_STY: Abs_WRITE_STY_F(); break;
			case ZP_WRITE_STA: ZP_WRITE_STA_F(); break;
			case ZP_WRITE_STY: ZP_WRITE_STY_F(); break;
			case ZP_WRITE_STX: ZP_WRITE_STX_F(); break;
			case ZP_WRITE_SAX: ZP_WRITE_SAX_F(); break;
			case IndIdx_Stage3: IndIdx_Stage3_F(); break;
			case IndIdx_Stage4: IndIdx_Stage4_F(); break;
			case IndIdx_WRITE_Stage5: IndIdx_WRITE_Stage5_F(); break;
			case IndIdx_READ_Stage5: IndIdx_READ_Stage5_F(); break;
			case IndIdx_RMW_Stage5: IndIdx_RMW_Stage5_F(); break;
			case IndIdx_WRITE_Stage6_STA: IndIdx_WRITE_Stage6_STA_F(); break;
			case IndIdx_WRITE_Stage6_SHA: IndIdx_WRITE_Stage6_SHA_F(); break;
			case IndIdx_READ_Stage6_LDA: IndIdx_READ_Stage6_LDA_F(); break;
			case IndIdx_READ_Stage6_CMP: IndIdx_READ_Stage6_CMP_F(); break;
			case IndIdx_READ_Stage6_AND: IndIdx_READ_Stage6_AND_F(); break;
			case IndIdx_READ_Stage6_EOR: IndIdx_READ_Stage6_EOR_F(); break;
			case IndIdx_READ_Stage6_LAX: IndIdx_READ_Stage6_LAX_F(); break;
			case IndIdx_READ_Stage6_ADC: IndIdx_READ_Stage6_ADC_F(); break;
			case IndIdx_READ_Stage6_SBC: IndIdx_READ_Stage6_SBC_F(); break;
			case IndIdx_READ_Stage6_ORA: IndIdx_READ_Stage6_ORA_F(); break;
			case IndIdx_RMW_Stage6: IndIdx_RMW_Stage6_F(); break;
			case IndIdx_RMW_Stage8: IndIdx_RMW_Stage8_F(); break;
			case Ind_zp_Stage4: Ind_zp_Stage4_F(); break;
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
			case DecS: DecS_F(); break;
			case IncS: IncS_F(); break;
			case JSR: JSR_F(); break;
			case PullP: PullP_F(); break;
			case PullPCL: PullPCL_F(); break;
			case PullPCH_NoInc: PullPCH_NoInc_F(); break;
			case Abs_READ_LDA: Abs_READ_LDA_F(); break;
			case Abs_READ_LDY: Abs_READ_LDY_F(); break;
			case Abs_READ_LDX: Abs_READ_LDX_F(); break;
			case Abs_READ_BIT: Abs_READ_BIT_F(); break;
			case Abs_READ_AND: Abs_READ_AND_F(); break;
			case Abs_READ_EOR: Abs_READ_EOR_F(); break;
			case Abs_READ_ORA: Abs_READ_ORA_F(); break;
			case Abs_READ_ADC: Abs_READ_ADC_F(); break;
			case Abs_READ_CMP: Abs_READ_CMP_F(); break;
			case Abs_READ_CPY: Abs_READ_CPY_F(); break;
			case Abs_READ_NOP: Abs_READ_NOP_F(); break;
			case Abs_READ_CPX: Abs_READ_CPX_F(); break;
			case Abs_READ_SBC: Abs_READ_SBC_F(); break;
			case ZpIdx_Stage3_X: ZpIdx_Stage3_X_F(); break;
			case ZpIdx_Stage3_Y: ZpIdx_Stage3_Y_F(); break;
			case ZpIdx_RMW_Stage4: ZpIdx_RMW_Stage4_F(); break;
			case ZpIdx_RMW_Stage6: ZpIdx_RMW_Stage6_F(); break;
			case ZP_READ_EOR: ZP_READ_EOR_F(); break;
			case ZP_READ_BIT: ZP_READ_BIT_F(); break;
			case ZP_READ_LDA: ZP_READ_LDA_F(); break;
			case ZP_READ_LDY: ZP_READ_LDY_F(); break;
			case ZP_READ_LDX: ZP_READ_LDX_F(); break;
			case ZP_READ_LAX: ZP_READ_LAX_F(); break;
			case ZP_READ_CPY: ZP_READ_CPY_F(); break;
			case ZP_READ_CMP: ZP_READ_CMP_F(); break;
			case ZP_READ_CPX: ZP_READ_CPX_F(); break;
			case ZP_READ_ORA: ZP_READ_ORA_F(); break;
			case ZP_READ_NOP: ZP_READ_NOP_F(); break;
			case ZP_READ_SBC: ZP_READ_SBC_F(); break;
			case ZP_READ_ADC: ZP_READ_ADC_F(); break;
			case ZP_READ_AND: ZP_READ_AND_F(); break;
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
			case Imm_EOR: Imm_EOR_F(); break;
			case Imm_ANC: Imm_ANC_F(); break;
			case Imm_ASR: Imm_ASR_F(); break;
			case Imm_AXS: Imm_AXS_F(); break;
			case Imm_ARR: Imm_ARR_F(); break;
			case Imm_ORA: Imm_ORA_F(); break;
			case Imm_CPY: Imm_CPY_F(); break;
			case Imm_CPX: Imm_CPX_F(); break;
			case Imm_CMP: Imm_CMP_F(); break;
			case Imm_SBC: Imm_SBC_F(); break;
			case Imm_AND: Imm_AND_F(); break;
			case Imm_ADC: Imm_ADC_F(); break;
			case Imm_LDA: Imm_LDA_F(); break;
			case Imm_LDX: Imm_LDX_F(); break;
			case Imm_LDY: Imm_LDY_F(); break;
			case IdxInd_Stage3: IdxInd_Stage3_F(); break;
			case IdxInd_Stage4: IdxInd_Stage4_F(); break;
			case IdxInd_Stage5: IdxInd_Stage5_F(); break;
			case IdxInd_Stage6_READ_LDA: IdxInd_Stage6_READ_LDA_F(); break;
			case IdxInd_Stage6_READ_ORA: IdxInd_Stage6_READ_ORA_F(); break;
			case IdxInd_Stage6_READ_LAX: IdxInd_Stage6_READ_LAX_F(); break;
			case IdxInd_Stage6_READ_CMP: IdxInd_Stage6_READ_CMP_F(); break;
			case IdxInd_Stage6_READ_ADC: IdxInd_Stage6_READ_ADC_F(); break;
			case IdxInd_Stage6_READ_AND: IdxInd_Stage6_READ_AND_F(); break;
			case IdxInd_Stage6_READ_EOR: IdxInd_Stage6_READ_EOR_F(); break;
			case IdxInd_Stage6_READ_SBC: IdxInd_Stage6_READ_SBC_F(); break;
			case IdxInd_Stage6_WRITE_STA: IdxInd_Stage6_WRITE_STA_F(); break;
			case IdxInd_Stage6_WRITE_SAX: IdxInd_Stage6_WRITE_SAX_F(); break;
			case IdxInd_Stage6_RMW: IdxInd_Stage6_RMW_F(); break;
			case IdxInd_Stage8_RMW: IdxInd_Stage8_RMW_F(); break;
			case PushP: PushP_F(); break;
			case PushA: PushA_F(); break;
			case PullA_NoInc: PullA_NoInc_F(); break;
			case PullP_NoInc: PullP_NoInc_F(); break;
			case Imp_ASL_A: Imp_ASL_A_F(); break;
			case Imp_ROL_A: Imp_ROL_A_F(); break;
			case Imp_ROR_A: Imp_ROR_A_F(); break;
			case Imp_LSR_A: Imp_LSR_A_F(); break;
			case JMP_abs: JMP_abs_F(); break;
			case IncPC: IncPC_F(); break;
			case ZP_RMW_Stage3: ZP_RMW_Stage3_F(); break;
			case ZP_RMW_Stage5: ZP_RMW_Stage5_F(); break;
			case ZP_RMW_INC: ZP_RMW_INC_F(); break;
			case ZP_RMW_DEC: ZP_RMW_DEC_F(); break;
			case ZP_RMW_ASL: ZP_RMW_ASL_F(); break;
			case ZP_RMW_LSR: ZP_RMW_LSR_F(); break;
			case ZP_RMW_ROR: ZP_RMW_ROR_F(); break;
			case ZP_RMW_ROL: ZP_RMW_ROL_F(); break;
			case AbsIdx_Stage3_Y: AbsIdx_Stage3_Y_F(); break;
			case AbsIdx_Stage3_X: AbsIdx_Stage3_X_F(); break;
			case AbsIdx_READ_Stage4: AbsIdx_READ_Stage4_F(); break;
			case AbsIdx_Stage4: AbsIdx_Stage4_F(); break;
			case AbsIdx_WRITE_Stage5_STA: AbsIdx_WRITE_Stage5_STA_F(); break;
			case AbsIdx_WRITE_Stage5_ERROR: AbsIdx_WRITE_Stage5_ERROR_F(); break;
			case AbsIdx_RMW_Stage5: AbsIdx_RMW_Stage5_F(); break;
			case AbsIdx_RMW_Stage7: AbsIdx_RMW_Stage7_F(); break;
			case AbsIdx_RMW_Stage6_DEC: AbsIdx_RMW_Stage6_DEC_F(); break;
			case AbsIdx_RMW_Stage6_INC: AbsIdx_RMW_Stage6_INC_F(); break;
			case AbsIdx_RMW_Stage6_ROL: AbsIdx_RMW_Stage6_ROL_F(); break;
			case AbsIdx_RMW_Stage6_LSR: AbsIdx_RMW_Stage6_LSR_F(); break;
			case AbsIdx_RMW_Stage6_ASL: AbsIdx_RMW_Stage6_ASL_F(); break;
			case AbsIdx_RMW_Stage6_ROR: AbsIdx_RMW_Stage6_ROR_F(); break;
			case AbsIdx_READ_Stage5_LDA: AbsIdx_READ_Stage5_LDA_F(); break;
			case AbsIdx_READ_Stage5_LDX: AbsIdx_READ_Stage5_LDX_F(); break;
			case AbsIdx_READ_Stage5_LDY: AbsIdx_READ_Stage5_LDY_F(); break;
			case AbsIdx_READ_Stage5_ORA: AbsIdx_READ_Stage5_ORA_F(); break;
			case AbsIdx_READ_Stage5_NOP: AbsIdx_READ_Stage5_NOP_F(); break;
			case AbsIdx_READ_Stage5_CMP: AbsIdx_READ_Stage5_CMP_F(); break;
			case AbsIdx_READ_Stage5_SBC: AbsIdx_READ_Stage5_SBC_F(); break;
			case AbsIdx_READ_Stage5_ADC: AbsIdx_READ_Stage5_ADC_F(); break;
			case AbsIdx_READ_Stage5_EOR: AbsIdx_READ_Stage5_EOR_F(); break;
			case AbsIdx_READ_Stage5_AND: AbsIdx_READ_Stage5_AND_F(); break;
			case AbsIdx_READ_Stage5_ERROR: AbsIdx_READ_Stage5_ERROR_F(); break;
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
			case End_ISpecial: End_ISpecial_F(); break;
			case End_SuppressInterrupt: End_SuppressInterrupt_F(); break;
			case End: End_F(); break;
			case End_BranchSpecial: End_BranchSpecial_F(); break;
			}
		}

		#pragma endregion

		#pragma region operations
		
		void FetchDummy_F() { DummyReadMemory(PC); }

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
			opcode = ReadMemory(PC++);
			mi = -1;
		}

		void Fetch2_F() { opcode2 = ReadMemory(PC++); }

		void Fetch3_F() { opcode3 = ReadMemory(PC++); }

		void PushPCH_F() { WriteMemory((uint16_t)(S-- + 0x100), (uint8_t)(PC >> 8)); }

		void PushPCL_F() { WriteMemory((uint16_t)(S-- + 0x100), (uint8_t)PC); }

		void PushP_BRK_F()
		{
			FlagBset(true);
			WriteMemory((uint16_t)(S-- + 0x100), P);
			FlagIset(true);
			ea = BRKVector;
		}

		void PushP_IRQ_F()
		{
			FlagBset(false);
			WriteMemory((uint16_t)(S-- + 0x100), P);
			FlagIset(true);
			ea = IRQVector;
		}

		void PushP_NMI_F()
		{
			FlagBset(false);
			WriteMemory((uint16_t)(S-- + 0x100), P);
			FlagIset(true); //is this right?
			ea = NMIVector;
		}

		void PushP_Reset_F()
		{
			ea = ResetVector;
			DummyReadMemory((uint16_t)(S-- + 0x100));
			FlagIset(true);
		}

		void PushDummy_F() { DummyReadMemory((uint16_t)(S-- + 0x100)); }

		void FetchPCLVector_F()
		{
			if (ea == BRKVector && FlagBget() && NMI)
			{
				NMI = false;
				ea = NMIVector;
			}
			if (ea == IRQVector && !FlagBget() && NMI)
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

		void Imp_INY_F() { DummyReadMemory(PC); Y++; NZ_Y_F(); }

		void Imp_DEY_F() { DummyReadMemory(PC); Y--; NZ_Y_F(); }

		void Imp_INX_F() { DummyReadMemory(PC); X++; NZ_X_F(); }

		void Imp_DEX_F() { DummyReadMemory(PC); X--; NZ_X_F(); }

		void NZ_A_F() { P = (uint8_t)((P & 0x7D) | TableNZ[A]); }

		void NZ_X_F() { P = (uint8_t)((P & 0x7D) | TableNZ[X]); }

		void NZ_Y_F() { P = (uint8_t)((P & 0x7D) | TableNZ[Y]); }

		void Imp_TSX_F() { DummyReadMemory(PC); X = S; NZ_X_F(); }

		void Imp_TXS_F() { DummyReadMemory(PC); S = X; }

		void Imp_TAX_F() { DummyReadMemory(PC); X = A; NZ_X_F(); }

		void Imp_TAY_F() { DummyReadMemory(PC); Y = A; NZ_Y_F(); }

		void Imp_TYA_F() { DummyReadMemory(PC); A = Y; NZ_A_F(); }

		void Imp_TXA_F() { DummyReadMemory(PC); A = X; NZ_A_F(); }

		void Imp_SEI_F() { DummyReadMemory(PC); iflag_pending = true; }

		void Imp_CLI_F() { DummyReadMemory(PC); iflag_pending = false; }

		void Imp_SEC_F() { DummyReadMemory(PC); FlagCset(true); }

		void Imp_CLC_F() { DummyReadMemory(PC); FlagCset(false); }

		void Imp_SED_F() { DummyReadMemory(PC); FlagDset(true); }

		void Imp_CLD_F() { DummyReadMemory(PC); FlagDset(false); }

		void Imp_CLV_F() { DummyReadMemory(PC); FlagVset(false); }

		void Abs_WRITE_STA_F() { WriteMemory((uint16_t)((opcode3 << 8) + opcode2), A); }

		void Abs_WRITE_STX_F() { WriteMemory((uint16_t)((opcode3 << 8) + opcode2), X); }

		void Abs_WRITE_STY_F() { WriteMemory((uint16_t)((opcode3 << 8) + opcode2), Y); }

		void ZP_WRITE_STA_F() { WriteMemory(opcode2, A); }

		void ZP_WRITE_STY_F() { WriteMemory(opcode2, Y); }

		void ZP_WRITE_STX_F() { WriteMemory(opcode2, X); }

		void ZP_WRITE_SAX_F() { WriteMemory(opcode2, (uint8_t)(X & A)); }

		void Ind_zp_Stage4_F() 
		{
			ea = (ReadMemory((uint8_t)(opcode2 + 1)) << 8)
				| alu_temp;
		}

		void IndIdx_Stage3_F() { ea = ReadMemory(opcode2); }

		void IndIdx_Stage4_F()
		{
			alu_temp = ea + Y;
			ea = (ReadMemory((uint8_t)(opcode2 + 1)) << 8)
				| ((alu_temp & 0xFF));
		}

		void IndIdx_WRITE_Stage5_F()
		{
			ReadMemory((uint16_t)ea);
			ea += (alu_temp >> 8) << 8;
		}

		void IndIdx_READ_Stage5_F()
		{
			if (!((alu_temp & 0x100) == 0x100))
			{
				mi++;
				ExecuteOneRetry();
				return;
			}
			else
			{		
				ReadMemory((uint16_t)ea);
				ea = (uint16_t)(ea + 0x100);
			}
		}

		void IndIdx_RMW_Stage5_F()
		{
			ReadMemory((uint16_t)ea);
			if (((alu_temp & 0x100) == 0x100))
				ea = (uint16_t)(ea + 0x100);
		}

		void IndIdx_WRITE_Stage6_STA_F() { WriteMemory((uint16_t)ea, A); }

		void IndIdx_WRITE_Stage6_SHA_F() { WriteMemory((uint16_t)ea, (uint8_t)(A & X & 7)); }

		void IndIdx_READ_Stage6_LDA_F() { A = ReadMemory((uint16_t)ea); NZ_A_F(); }

		void IndIdx_READ_Stage6_CMP_F() { alu_temp = ReadMemory((uint16_t)ea); _Cmp_F(); }

		void IndIdx_READ_Stage6_AND_F() { alu_temp = ReadMemory((uint16_t)ea); _And_F(); }

		void IndIdx_READ_Stage6_EOR_F() { alu_temp = ReadMemory((uint16_t)ea); _Eor_F(); }

		void IndIdx_READ_Stage6_LAX_F() { A = X = ReadMemory((uint16_t)ea); NZ_A_F(); }

		void IndIdx_READ_Stage6_ADC_F() { alu_temp = ReadMemory((uint16_t)ea); _Adc_F(); }

		void IndIdx_READ_Stage6_SBC_F() { alu_temp = ReadMemory((uint16_t)ea); _Sbc_F(); }

		void IndIdx_READ_Stage6_ORA_F() { alu_temp = ReadMemory((uint16_t)ea); _Ora_F(); }

		void IndIdx_RMW_Stage6_F() { alu_temp = ReadMemory((uint16_t)ea); }

		void IndIdx_RMW_Stage8_F() { WriteMemory((uint16_t)ea, (uint8_t)alu_temp); }

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
			opcode2 = ReadMemory(PC++);
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
			DummyReadMemory(PC);
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
			DummyReadMemory(PC);
			if ((alu_temp & 0x80000000) == 0x80000000)
				PC = (uint16_t)(PC - 0x100);
			else PC = (uint16_t)(PC + 0x100);
		}

		void NOP_F() { DummyReadMemory(PC); }

		void DecS_F() { DummyReadMemory((uint16_t)(0x100 | --S)); }

		void IncS_F() { DummyReadMemory((uint16_t)(0x100 | S++)); }

		void JSR_F() { PC = (uint16_t)((ReadMemory((uint16_t)(PC)) << 8) + opcode2); }

		void PullP_F() { P = ReadMemory((uint16_t)(S++ + 0x100)); FlagTset(true); }

		void PullPCL_F() { PC &= 0xFF00; PC |= ReadMemory((uint16_t)(S++ + 0x100)); }

		void PullPCH_NoInc_F() { PC &= 0xFF; PC |= (uint16_t)(ReadMemory((uint16_t)(S + 0x100)) << 8); }

		void Abs_READ_LDA_F() { A = ReadMemory((uint16_t)((opcode3 << 8) + opcode2)); NZ_A_F(); }

		void Abs_READ_LDY_F() { Y = ReadMemory((uint16_t)((opcode3 << 8) + opcode2)); NZ_Y_F(); }

		void Abs_READ_LDX_F() { X = ReadMemory((uint16_t)((opcode3 << 8) + opcode2)); NZ_X_F(); }

		void Abs_READ_BIT_F() { alu_temp = ReadMemory((uint16_t)((opcode3 << 8) + opcode2)); _Bit_F(); }

		void Abs_READ_AND_F() { alu_temp = ReadMemory((uint16_t)((opcode3 << 8) + opcode2)); _And_F(); }

		void Abs_READ_EOR_F() { alu_temp = ReadMemory((uint16_t)((opcode3 << 8) + opcode2)); _Eor_F(); }

		void Abs_READ_ORA_F() { alu_temp = ReadMemory((uint16_t)((opcode3 << 8) + opcode2)); _Ora_F(); }

		void Abs_READ_ADC_F() { alu_temp = ReadMemory((uint16_t)((opcode3 << 8) + opcode2)); _Adc_F(); }

		void Abs_READ_CMP_F() { alu_temp = ReadMemory((uint16_t)((opcode3 << 8) + opcode2)); _Cmp_F(); }

		void Abs_READ_CPY_F() { alu_temp = ReadMemory((uint16_t)((opcode3 << 8) + opcode2)); _Cpy_F(); }

		void Abs_READ_NOP_F() { alu_temp = ReadMemory((uint16_t)((opcode3 << 8) + opcode2)); }

		void Abs_READ_CPX_F() { alu_temp = ReadMemory((uint16_t)((opcode3 << 8) + opcode2)); _Cpx_F(); }

		void Abs_READ_SBC_F() { alu_temp = ReadMemory((uint16_t)((opcode3 << 8) + opcode2)); _Sbc_F(); }

		void ZpIdx_Stage3_X_F() { ReadMemory(opcode2); opcode2 = (uint8_t)(opcode2 + X); }

		void ZpIdx_Stage3_Y_F() { ReadMemory(opcode2); opcode2 = (uint8_t)(opcode2 + Y); }

		void ZpIdx_RMW_Stage4_F() 	{ alu_temp = ReadMemory(opcode2); }

		void ZpIdx_RMW_Stage6_F() { WriteMemory(opcode2, (uint8_t)alu_temp); }

		void ZP_READ_EOR_F() { alu_temp = ReadMemory(opcode2); _Eor_F(); }

		void ZP_READ_BIT_F() { alu_temp = ReadMemory(opcode2); _Bit_F(); }

		void ZP_READ_LDA_F() { A = ReadMemory(opcode2); NZ_A_F(); }

		void ZP_READ_LDY_F() { Y = ReadMemory(opcode2); NZ_Y_F(); }

		void ZP_READ_LDX_F() { X = ReadMemory(opcode2); NZ_X_F(); }

		void ZP_READ_LAX_F() { X = ReadMemory(opcode2); A = X; NZ_A_F(); }

		void ZP_READ_CPY_F() { alu_temp = ReadMemory(opcode2); _Cpy_F(); }

		void ZP_READ_CMP_F() { alu_temp = ReadMemory(opcode2); _Cmp_F(); }

		void ZP_READ_CPX_F() { alu_temp = ReadMemory(opcode2); _Cpx_F(); }

		void ZP_READ_ORA_F() { alu_temp = ReadMemory(opcode2); _Ora_F(); }

		void ZP_READ_NOP_F() { ReadMemory(opcode2); }

		void ZP_READ_SBC_F() { alu_temp = ReadMemory(opcode2); _Sbc_F(); }

		void ZP_READ_ADC_F() { alu_temp = ReadMemory(opcode2); _Adc_F(); }

		void ZP_READ_AND_F() { alu_temp = ReadMemory(opcode2); _And_F(); }

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
			//TODO - an extra cycle penalty on 65C02 only
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

		void Imm_EOR_F() { alu_temp = ReadMemory(PC++); _Eor_F(); }

		void Imm_ANC_F() { alu_temp = ReadMemory(PC++); _Anc_F(); }

		void Imm_ASR_F() { alu_temp = ReadMemory(PC++); _Asr_F(); }

		void Imm_AXS_F() { alu_temp = ReadMemory(PC++); _Axs_F(); }

		void Imm_ARR_F() { alu_temp = ReadMemory(PC++); _Arr_F(); }

		void Imm_ORA_F() { alu_temp = ReadMemory(PC++); _Ora_F(); }

		void Imm_CPY_F() { alu_temp = ReadMemory(PC++); _Cpy_F(); }

		void Imm_CPX_F() { alu_temp = ReadMemory(PC++); _Cpx_F(); }

		void Imm_CMP_F() { alu_temp = ReadMemory(PC++); _Cmp_F(); }

		void Imm_SBC_F() { alu_temp = ReadMemory(PC++); _Sbc_F(); }

		void Imm_AND_F() { alu_temp = ReadMemory(PC++); _And_F(); }

		void Imm_ADC_F() { alu_temp = ReadMemory(PC++); _Adc_F(); }

		void Imm_LDA_F() { A = ReadMemory(PC++); NZ_A_F(); }

		void Imm_LDX_F() { X = ReadMemory(PC++); NZ_X_F(); }

		void Imm_LDY_F() { Y = ReadMemory(PC++); NZ_Y_F(); }

		void IdxInd_Stage3_F() { DummyReadMemory(opcode2); alu_temp = (opcode2 + X) & 0xFF; }

		void IdxInd_Stage4_F() { ea = ReadMemory((uint16_t)alu_temp); }

		void IdxInd_Stage5_F() { ea += (ReadMemory((uint8_t)(alu_temp + 1)) << 8); }

		void IdxInd_Stage6_READ_LDA_F() { A = ReadMemory((uint16_t)ea); NZ_A_F(); }

		void IdxInd_Stage6_READ_ORA_F() { alu_temp = ReadMemory((uint16_t)ea); _Ora_F(); }

		void IdxInd_Stage6_READ_LAX_F() { A = X = ReadMemory((uint16_t)ea); NZ_A_F(); }

		void IdxInd_Stage6_READ_CMP_F() { alu_temp = ReadMemory((uint16_t)ea); _Cmp_F(); }

		void IdxInd_Stage6_READ_ADC_F() { alu_temp = ReadMemory((uint16_t)ea); _Adc_F(); }

		void IdxInd_Stage6_READ_AND_F() { alu_temp = ReadMemory((uint16_t)ea); _And_F(); }

		void IdxInd_Stage6_READ_EOR_F() { alu_temp = ReadMemory((uint16_t)ea); _Eor_F(); }

		void IdxInd_Stage6_READ_SBC_F() { alu_temp = ReadMemory((uint16_t)ea); _Sbc_F(); }

		void IdxInd_Stage6_WRITE_STA_F() { WriteMemory((uint16_t)ea, A); }

		void IdxInd_Stage6_WRITE_SAX_F()
		{
			alu_temp = A & X;
			WriteMemory((uint16_t)ea, (uint8_t)alu_temp);
			//flag writing skipped on purpose
		}

		void IdxInd_Stage6_RMW_F() { alu_temp = ReadMemory((uint16_t)ea); }

		void IdxInd_Stage8_RMW_F() { WriteMemory((uint16_t)ea, (uint8_t)alu_temp); }

		void PushP_F() { FlagBset(true); WriteMemory((uint16_t)(S-- + 0x100), P); }

		void PushA_F() { WriteMemory((uint16_t)(S-- + 0x100), A); }

		void PullA_NoInc_F() { A = ReadMemory((uint16_t)(S + 0x100)); NZ_A_F(); }

		void PullP_NoInc_F()
		{
			my_iflag = FlagIget();
			P = ReadMemory((uint16_t)(S + 0x100));
			iflag_pending = FlagIget();
			FlagIset(my_iflag);
			FlagTset(true); //force T always to remain true
		}

		void Imp_ASL_A_F()
		{
			DummyReadMemory(PC);
			FlagCset((A & 0x80) != 0);
			A = (uint8_t)(A << 1);
			NZ_A_F();
		}

		void Imp_ROL_A_F()
		{
			DummyReadMemory(PC);
			temp8 = A;
			A = (uint8_t)((A << 1) | (P & 1));
			FlagCset((temp8 & 0x80) != 0);
			NZ_A_F();
		}

		void Imp_ROR_A_F()
		{
			DummyReadMemory(PC);
			temp8 = A;
			A = (uint8_t)((A >> 1) | ((P & 1) << 7));
			FlagCset((temp8 & 1) != 0);
			NZ_A_F();
		}

		void Imp_LSR_A_F()
		{
			DummyReadMemory(PC);
			FlagCset((A & 1) != 0);
			A = (uint8_t)(A >> 1);
			NZ_A_F();
		}

		void JMP_abs_F() { PC = (uint16_t)((ReadMemory(PC) << 8) + opcode2); }

		void IncPC_F() { ReadMemory(PC); PC++; }

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

		void AbsIdx_Stage3_Y_F()
		{
			opcode3 = ReadMemory(PC++);
			alu_temp = opcode2 + Y;
			ea = (opcode3 << 8) + (alu_temp & 0xFF);
		}

		void AbsIdx_Stage3_X_F()
		{
			opcode3 = ReadMemory(PC++);
			alu_temp = opcode2 + X;
			ea = (opcode3 << 8) + (alu_temp & 0xFF);
		}

		void AbsIdx_READ_Stage4_F()
		{
			if (!((alu_temp & 0x100) == 0x100))
			{
				mi++;
				ExecuteOneRetry();
			}
			else
			{
				alu_temp = ReadMemory((uint16_t)ea);
				ea = (uint16_t)(ea + 0x100);
			}
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

		void AbsIdx_WRITE_Stage5_STA_F() { WriteMemory((uint16_t)ea, A); }

		void AbsIdx_WRITE_Stage5_ERROR_F()
		{
			S = (uint8_t)(X & A);
			WriteMemory((uint16_t)ea, (uint8_t)(S & (opcode3 + 1)));
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

		void AbsIdx_READ_Stage5_LDA_F() { A = ReadMemory((uint16_t)ea); NZ_A_F(); }

		void AbsIdx_READ_Stage5_LDX_F() { X = ReadMemory((uint16_t)ea); NZ_X_F(); }

		void AbsIdx_READ_Stage5_LDY_F() { Y = ReadMemory((uint16_t)ea); NZ_Y_F(); }

		void AbsIdx_READ_Stage5_ORA_F() { alu_temp = ReadMemory((uint16_t)ea); _Ora_F(); }

		void AbsIdx_READ_Stage5_NOP_F() { alu_temp = ReadMemory((uint16_t)ea); }

		void AbsIdx_READ_Stage5_CMP_F() { alu_temp = ReadMemory((uint16_t)ea); _Cmp_F(); }

		void AbsIdx_READ_Stage5_SBC_F() { alu_temp = ReadMemory((uint16_t)ea); _Sbc_F(); }

		void AbsIdx_READ_Stage5_ADC_F() { alu_temp = ReadMemory((uint16_t)ea); _Adc_F(); }

		void AbsIdx_READ_Stage5_EOR_F() { alu_temp = ReadMemory((uint16_t)ea); _Eor_F(); }

		void AbsIdx_READ_Stage5_AND_F() { alu_temp = ReadMemory((uint16_t)ea); _And_F(); }

		void AbsIdx_READ_Stage5_ERROR_F()
		{
			alu_temp = ReadMemory((uint16_t)ea);
			S &= (uint8_t)alu_temp;
			X = S;
			A = S;
			P = (uint8_t)((P & 0x7D) | TableNZ[S]);
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

		void End_ISpecial_F()
		{
			opcode = VOP_Fetch1;
			mi = 0;
			ExecuteOneRetry();
			return;
		}

		void End_SuppressInterrupt_F()
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

		void End_BranchSpecial_F() { End_F(); }

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
			reg_state.append(FlagTget() ? "T" : "t");
			reg_state.append(FlagBget() ? "B" : "b");
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
			"NOP",
			"NOP",
			"NOP d8",
			"ORA d8",
			"ASL d8",
			"NOP",
			"PHP",
			"ORA #d8",
			"ASL A",
			"NOP",
			"NOP (d16)",
			"ORA d16",
			"ASL d16",
			"NOP",
			"BPL r8",						// 0x10
			"ORA (d8),Y *",
			"NOP",
			"NOP",
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
			"NOP",
			"NOP",
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
			"NOP",
			"NOP",
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
			"NOP",
			"NOP",
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
			"NOP",
			"NOP",
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
			"NOP",
			"NOP",
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
			"NOP",
			"NOP",
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
			"NOP #d8",
			"NOP",
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
			"NOP",
			"NOP",
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
			"NOP",
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
			"NOP",
			"LAX (d8),Y *",
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
			"NOP #d8",
			"NOP",
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
			"NOP",
			"NOP",
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
			"NOP #d8",
			"NOP",
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
			"NOP",
			"NOP",
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
			saver = byte_saver(A, saver);
			saver = byte_saver(X, saver);
			saver = byte_saver(Y, saver);
			saver = byte_saver(P, saver);
			saver = byte_saver(S, saver);
			saver = byte_saver(opcode2, saver);
			saver = byte_saver(opcode3, saver);

			saver = short_saver(PC, saver);
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
			loader = byte_loader(&A, loader);
			loader = byte_loader(&X, loader);
			loader = byte_loader(&Y, loader);
			loader = byte_loader(&P, loader);
			loader = byte_loader(&S, loader);
			loader = byte_loader(&opcode2, loader);
			loader = byte_loader(&opcode3, loader);

			loader = short_loader(&PC, loader);
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
	};
}
