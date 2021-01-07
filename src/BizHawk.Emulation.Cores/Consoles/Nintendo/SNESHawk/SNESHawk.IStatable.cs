using System.IO;
using BizHawk.Common;

namespace BizHawk.Emulation.Cores.Nintendo.SNESHawk
{
	public partial class SNESHawk
	{
		private void SyncState(Serializer ser)
		{
			ser.BeginSection("SNES");

			ser.Sync("Frame", ref _frame);
			ser.Sync("LagCount", ref _lagCount);

			ser.EndSection();

			if (ser.IsReader)
			{
				ser.Sync(nameof(SNES_core), ref SNES_core, false);
				LibSNESHawk.SNES_load_state(SNES_Pntr, SNES_core);
			}
			else
			{
				LibSNESHawk.SNES_save_state(SNES_Pntr, SNES_core);
				ser.Sync(nameof(SNES_core), ref SNES_core, false);
			}
		}
	}
}
