using System;
using System.ComponentModel;

using Newtonsoft.Json;

using BizHawk.Common;
using BizHawk.Emulation.Common;

namespace BizHawk.Emulation.Cores.Nintendo.SNESHawk
{
	public partial class SNESHawk : IEmulator, ISettable<SNESHawk.SNESSettings, SNESHawk.SNESSyncSettings>
	{
		public SNESSettings GetSettings()
		{
			return _settings.Clone();
		}

		public SNESSyncSettings GetSyncSettings()
		{
			return _syncSettings.Clone();
		}

		public PutSettingsDirtyBits PutSettings(SNESSettings o)
		{
			_settings = o;
			return PutSettingsDirtyBits.None;
		}

		public PutSettingsDirtyBits PutSyncSettings(SNESSyncSettings o)
		{
			bool ret = SNESSyncSettings.NeedsReboot(_syncSettings, o);
			_syncSettings = o;
			return ret ? PutSettingsDirtyBits.RebootCore : PutSettingsDirtyBits.None;
		}

		public SNESSettings _settings = new SNESSettings();
		public SNESSyncSettings _syncSettings = new SNESSyncSettings();

		public class SNESSettings
		{
			public enum PaletteType
			{
				BW,
				Gr
			}

			public enum Cycle_Return
			{
				CPU,
				SNESI
			}

			[DisplayName("Color Mode")]
			[Description("Pick Between Green scale and Grey scale colors")]
			[DefaultValue(PaletteType.BW)]
			public PaletteType Palette { get; set; }

			[DisplayName("Read Domains on VBlank")]
			[Description("When true, memory domains are only updated on VBlank. More consistent for LUA. NOTE: Does not work for system bus, does not apply to writes.")]
			[DefaultValue(false)]
			public bool VBL_sync { get; set; }

			[DisplayName("TotalExecutedCycles Return Value")]
			[Description("CPU returns the actual CPU cycles executed, SNESI returns the values needed for console verification")]
			[DefaultValue(Cycle_Return.CPU)]
			public Cycle_Return cycle_return_setting { get; set; }

			public SNESSettings Clone()
			{
				return (SNESSettings)MemberwiseClone();
			}

			public SNESSettings()
			{
				SettingsUtil.SetDefaultValues(this);
			}
		}

		public class SNESSyncSettings
		{
			[JsonIgnore]
			public string Port1 = SNESHawkControllerDeck.DefaultControllerName;

			[JsonIgnore]
			public string Port2 = SNESHawkControllerDeck.DefaultControllerName;

			public enum ControllerType
			{
				Standard
			}

			[JsonIgnore]
			private ControllerType _SNESController1;

			[JsonIgnore]
			private ControllerType _SNESController2;

			[DisplayName("SNES Controller 1")]
			[Description("Standard SNES Controllers")]
			[DefaultValue(ControllerType.Standard)]
			public ControllerType SNESController1
			{
				get => _SNESController1;
				set
				{
					if (value == ControllerType.Standard) { Port1 = "SNES Controller"; }

					_SNESController1 = value;
				}
			}

			[DisplayName("SNES Controller 2")]
			[Description("Standard SNES Controllers")]
			[DefaultValue(ControllerType.Standard)]
			public ControllerType SNESController2
			{
				get => _SNESController2;
				set
				{
					if (value == ControllerType.Standard) { Port2 = "SNES Controller"; }

					_SNESController2 = value;
				}
			}

			public enum ConsoleModeType
			{
				Auto,
				NTSC,
				PAL
			}

			[DisplayName("Console Mode")]
			[Description("Pick which console to run, 'Auto' chooses from ROM extension, 'SNES' and 'SNESC' chooses the respective system")]
			[DefaultValue(ConsoleModeType.Auto)]
			public ConsoleModeType ConsoleMode { get; set; }

			[DisplayName("RTC Initial Time")]
			[Description("Set the initial RTC time in terms of elapsed seconds.")]
			[DefaultValue(0)]
			public int RTCInitialTime
			{
				get => _RTCInitialTime;
				set => _RTCInitialTime = Math.Max(0, Math.Min(1024 * 24 * 60 * 60, value));
			}

			[DisplayName("RTC Offset")]
			[Description("Set error in RTC clocking (-127 to 127)")]
			[DefaultValue(0)]
			public int RTCOffset
			{
				get => _RTCOffset;
				set => _RTCOffset = Math.Max(-127, Math.Min(127, value));
			}

			[DisplayName("Use Existing SaveRAM")]
			[Description("(Intended for development, for regular use leave as true.) When true, existing SaveRAM will be loaded at boot up.")]
			[DefaultValue(true)]
			public bool Use_SRAM { get; set; }

			[JsonIgnore]
			private int _RTCInitialTime;
			[JsonIgnore]
			private int _RTCOffset;
			[JsonIgnore]
			public ushort _DivInitialTime = 8;

			public SNESSyncSettings Clone()
			{
				return (SNESSyncSettings)MemberwiseClone();
			}

			public SNESSyncSettings()
			{
				SettingsUtil.SetDefaultValues(this);
			}

			public static bool NeedsReboot(SNESSyncSettings x, SNESSyncSettings y)
			{
				return !DeepEquality.DeepEquals(x, y);
			}
		}
	}
}
