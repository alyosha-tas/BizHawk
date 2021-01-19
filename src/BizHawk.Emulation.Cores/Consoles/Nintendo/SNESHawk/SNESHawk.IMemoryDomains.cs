using System.Collections.Generic;
using BizHawk.Emulation.Common;

namespace BizHawk.Emulation.Cores.Nintendo.SNESHawk
{
	public partial class SNESHawk
	{
		private IMemoryDomains MemoryDomains;

		public void SetupMemoryDomains()
		{
			var domains = new List<MemoryDomain>
			{
				new MemoryDomainDelegate(
					"RAM",
					0x800,
					MemoryDomain.Endian.Little,
					addr => LibSNESHawk.SNES_getram(SNES_Pntr, (int)(addr & 0xFFFF)),
					(addr, value) => LibSNESHawk.SNES_setram(SNES_Pntr, (int)(addr & 0xFFFF), value),
					1),
				new MemoryDomainDelegate(
					"ROM",
					ROM_Length,
					MemoryDomain.Endian.Little,
					addr => LibSNESHawk.SNES_getrom(SNES_Pntr, (int)(addr & (ROM_Length - 1))),
					(addr, value) => LibSNESHawk.SNES_setrom(SNES_Pntr, (int)(addr & (ROM_Length - 1)), value),
					1),
				new MemoryDomainDelegate(
					"VRAM",
					0x4000,
					MemoryDomain.Endian.Little,
					addr => LibSNESHawk.SNES_getvram(SNES_Pntr, (int)(addr & 0xFFFF)),
					(addr, value) => LibSNESHawk.SNES_setvram(SNES_Pntr, (int)(addr & 0xFFFF), value),
					1),
				new MemoryDomainDelegate(
					"OAM",
					0xA0,
					MemoryDomain.Endian.Little,
					addr => LibSNESHawk.SNES_getoam(SNES_Pntr, (int)(addr & 0xFFFF)),
					(addr, value) => LibSNESHawk.SNES_setoam(SNES_Pntr, (int)(addr & 0xFFFF), value),
					1),
				new MemoryDomainDelegate(
					"System Bus",
					0X10000,
					MemoryDomain.Endian.Little,
					addr => LibSNESHawk.SNES_getsysbus(SNES_Pntr, (int)(addr & 0xFFFF)),
					(addr, value) => LibSNESHawk.SNES_setsysbus(SNES_Pntr, (int)(addr & 0xFFFF), value),
					1),
			};
			/*
			if (cart_RAM != null)
			{
				var CartRam = new MemoryDomainByteArray("CartRAM", MemoryDomain.Endian.Little, cart_RAM, true, 1);
				domains.Add(CartRam);
			}
			*/

			MemoryDomains = new MemoryDomainList(domains);
			(ServiceProvider as BasicServiceProvider).Register<IMemoryDomains>(MemoryDomains);
		}
	}
}
