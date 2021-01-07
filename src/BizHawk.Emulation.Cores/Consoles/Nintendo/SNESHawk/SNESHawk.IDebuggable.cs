using System;
using System.Collections.Generic;
using BizHawk.Emulation.Common;

namespace BizHawk.Emulation.Cores.Nintendo.SNESHawk
{
	public partial class SNESHawk : IDebuggable
	{
		public IDictionary<string, RegisterValue> GetCpuFlagsAndRegisters()
		{
			return new Dictionary<string, RegisterValue>
			{
				["PCl"] = LibSNESHawk.SNES_cpu_get_regs(SNES_Pntr, 0),
				["PCh"] = LibSNESHawk.SNES_cpu_get_regs(SNES_Pntr, 1),
				["SPl"] = LibSNESHawk.SNES_cpu_get_regs(SNES_Pntr, 2),
				["SPh"] = LibSNESHawk.SNES_cpu_get_regs(SNES_Pntr, 3),
				["A"] = LibSNESHawk.SNES_cpu_get_regs(SNES_Pntr, 4),
				["F"] = LibSNESHawk.SNES_cpu_get_regs(SNES_Pntr, 5),
				["B"] = LibSNESHawk.SNES_cpu_get_regs(SNES_Pntr, 6),
				["C"] = LibSNESHawk.SNES_cpu_get_regs(SNES_Pntr, 7),
				["D"] = LibSNESHawk.SNES_cpu_get_regs(SNES_Pntr, 8),
				["E"] = LibSNESHawk.SNES_cpu_get_regs(SNES_Pntr, 9),
				["H"] = LibSNESHawk.SNES_cpu_get_regs(SNES_Pntr, 10),
				["L"] = LibSNESHawk.SNES_cpu_get_regs(SNES_Pntr, 11),
				["Flag I"] = LibSNESHawk.SNES_cpu_get_flags(SNES_Pntr, 0),
				["Flag C"] = LibSNESHawk.SNES_cpu_get_flags(SNES_Pntr, 1),
				["Flag H"] = LibSNESHawk.SNES_cpu_get_flags(SNES_Pntr, 2),
				["Flag N"] = LibSNESHawk.SNES_cpu_get_flags(SNES_Pntr, 3),
				["Flag Z"] = LibSNESHawk.SNES_cpu_get_flags(SNES_Pntr, 4)
			};
		}

		public void SetCpuRegister(string register, int value)
		{
			switch (register)
			{
				case ("PCl"): LibSNESHawk.SNES_cpu_set_regs(SNES_Pntr, 0, (byte)value); break;
				case ("PCh"): LibSNESHawk.SNES_cpu_set_regs(SNES_Pntr, 1, (byte)value); break;
				case ("SPl"): LibSNESHawk.SNES_cpu_set_regs(SNES_Pntr, 2, (byte)value); break;
				case ("SPh"): LibSNESHawk.SNES_cpu_set_regs(SNES_Pntr, 3, (byte)value); break;
				case ("A"): LibSNESHawk.SNES_cpu_set_regs(SNES_Pntr, 4, (byte)value); break;
				case ("F"): LibSNESHawk.SNES_cpu_set_regs(SNES_Pntr, 5, (byte)value); break;
				case ("B"): LibSNESHawk.SNES_cpu_set_regs(SNES_Pntr, 6, (byte)value); break;
				case ("C"): LibSNESHawk.SNES_cpu_set_regs(SNES_Pntr, 7, (byte)value); break;
				case ("D"): LibSNESHawk.SNES_cpu_set_regs(SNES_Pntr, 8, (byte)value); break;
				case ("E"): LibSNESHawk.SNES_cpu_set_regs(SNES_Pntr, 9, (byte)value); break;
				case ("H"): LibSNESHawk.SNES_cpu_set_regs(SNES_Pntr, 10, (byte)value); break;
				case ("L"): LibSNESHawk.SNES_cpu_set_regs(SNES_Pntr, 11, (byte)value); break;

				case ("Flag I"): LibSNESHawk.SNES_cpu_set_flags(SNES_Pntr, 0, value > 0); break;
				case ("Flag C"): LibSNESHawk.SNES_cpu_set_flags(SNES_Pntr, 1, value > 0); break;
				case ("Flag H"): LibSNESHawk.SNES_cpu_set_flags(SNES_Pntr, 2, value > 0); break;
				case ("Flag N"): LibSNESHawk.SNES_cpu_set_flags(SNES_Pntr, 3, value > 0); break;
				case ("Flag Z"): LibSNESHawk.SNES_cpu_set_flags(SNES_Pntr, 4, value > 0); break;
			}
		}

		public IMemoryCallbackSystem MemoryCallbacks { get; } = new MemoryCallbackSystem(new[] { "System Bus" });

		public bool CanStep(StepType type) => false;

		[FeatureNotImplemented]
		public void Step(StepType type) => throw new NotImplementedException();

		public long TotalExecutedCycles => (long)LibSNESHawk.SNES_cpu_cycles(SNES_Pntr);
	}
}
