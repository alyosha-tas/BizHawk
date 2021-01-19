using System;
using System.Text;

using BizHawk.Common.BufferExtensions;
using BizHawk.Emulation.Common;

namespace BizHawk.Emulation.Cores.Nintendo.SNESHawk
{
	[Core(
		CoreNames.NesHawk,
		"",
		isPorted: false,
		isReleased: true)]
	[ServiceNotApplicable(new[] { typeof(IDriveLight) })]
	public partial class SNESHawk : IEmulator, ISaveRam, IDebuggable, IInputPollable, IRegionable,
	ISettable<SNESHawk.SNESSettings, SNESHawk.SNESSyncSettings>
	{
		public IntPtr SNES_Pntr { get; set; } = IntPtr.Zero;
		byte[] SNES_core = new byte[0x100000];

		private int _frame = 0;
		public int _lagCount = 0;
		public bool is_PAL = false;
		public int ROM_Length;

		public byte[] _bios;

		[CoreConstructor("SNES")]
		public SNESHawk(CoreComm comm, GameInfo game, byte[] rom, /*string gameDbFn,*/ SNESSettings settings, SNESSyncSettings syncSettings)
		{
			ServiceProvider = new BasicServiceProvider(this);

			_settings = (SNESSettings)settings ?? new SNESSettings();
			_syncSettings = (SNESSyncSettings)syncSettings ?? new SNESSyncSettings();
			_controllerDeck = new SNESHawkControllerDeck(_syncSettings.Port1, _syncSettings.Port2);

			// for SPC700
			byte[] Bios = null;

			ROM_Length = rom.Length;

			SNES_Pntr = LibSNESHawk.SNES_create();

			char[] MD5_temp = rom.HashMD5(0, rom.Length).ToCharArray();

			//LibSNESHawk.SNES_load_bios(SNES_Pntr, _bios);
			LibSNESHawk.SNES_load(SNES_Pntr, rom, (uint)rom.Length, MD5_temp, is_PAL);

			blip_L.SetRates(4194304, 44100);
			blip_R.SetRates(4194304, 44100);

			(ServiceProvider as BasicServiceProvider).Register<ISoundProvider>(this);

			SetupMemoryDomains();

			Header_Length = LibSNESHawk.SNES_getheaderlength(SNES_Pntr);
			Disasm_Length = LibSNESHawk.SNES_getdisasmlength(SNES_Pntr);
			Reg_String_Length = LibSNESHawk.SNES_getregstringlength(SNES_Pntr);

			var newHeader = new StringBuilder(Header_Length);
			LibSNESHawk.SNES_getheader(SNES_Pntr, newHeader, Header_Length);

			Console.WriteLine(Header_Length + " " + Disasm_Length + " " + Reg_String_Length);

			Tracer = new TraceBuffer { Header = newHeader.ToString() };

			var serviceProvider = ServiceProvider as BasicServiceProvider;
			serviceProvider.Register<ITraceable>(Tracer);
			serviceProvider.Register<IStatable>(new StateSerializer(SyncState));

			Console.WriteLine("MD5: " + rom.HashMD5(0, rom.Length));
			Console.WriteLine("SHA1: " + rom.HashSHA1(0, rom.Length));

			HardReset();
		}

		public DisplayType Region => DisplayType.NTSC;

		private readonly SNESHawkControllerDeck _controllerDeck;

		public void HardReset()
		{
			LibSNESHawk.SNES_Reset(SNES_Pntr);

			frame_buffer = new int[VirtualWidth * VirtualHeight];
		}

		private void ExecFetch(ushort addr)
		{
			uint flags = (uint)(MemoryCallbackFlags.AccessRead);
			MemoryCallbacks.CallMemoryCallbacks(addr, 0, flags, "System Bus");
		}

		#region Trace Logger
		public ITraceable Tracer;

		public LibSNESHawk.TraceCallback tracecb;

		// these will be constant values assigned during core construction
		public int Header_Length;
		public int Disasm_Length;
		public int Reg_String_Length;

		public void MakeTrace(int t)
		{
			StringBuilder new_d = new StringBuilder(Disasm_Length);
			StringBuilder new_r = new StringBuilder(Reg_String_Length);

			LibSNESHawk.SNES_getdisassembly(SNES_Pntr, new_d, t, Disasm_Length);
			LibSNESHawk.SNES_getregisterstate(SNES_Pntr, new_r, t, Reg_String_Length);

			Tracer.Put(new TraceInfo
			{
				Disassembly = new_d.ToString().PadRight(36),
				RegisterInfo = new_r.ToString()
			});
		}

		#endregion
	}
}
