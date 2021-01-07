using System;
using System.ComponentModel;
using System.Linq;

using BizHawk.Common;
using BizHawk.Emulation.Common;

namespace BizHawk.Emulation.Cores.Nintendo.SNESHawk
{
	/// <summary>
	/// Represents a SNES add on
	/// </summary>
	public interface IPort
	{
		byte Read(IController c);

		ControllerDefinition Definition { get; }

		void SyncState(Serializer ser);

		int PortNum { get; }
	}

	[DisplayName("SNES Controller")]
	public class StandardControls : IPort
	{
		public StandardControls(int portNum)
		{
			PortNum = portNum;
			Definition = new ControllerDefinition
			{
				Name = "SNES Controller N",
				BoolButtons = BaseDefinition
				.Select(b => "P" + PortNum + " " + b)
				.ToList()
			};
		}

		public int PortNum { get; }

		public ControllerDefinition Definition { get; }

		public byte Read(IController c)
		{
			byte result = 0;

			if (c.IsPressed(Definition.BoolButtons[0]))
			{
				result |= 0x10;
			}
			if (c.IsPressed(Definition.BoolButtons[1]))
			{
				result |= 0x20;
			}
			if (c.IsPressed(Definition.BoolButtons[2]))
			{
				result |= 0x40;
			}
			if (c.IsPressed(Definition.BoolButtons[3]))
			{
				result |= 0x80;
			}
			if (c.IsPressed(Definition.BoolButtons[4]))
			{
				result |= 0x08;
			}
			if (c.IsPressed(Definition.BoolButtons[5]))
			{
				result |= 0x04;
			}
			if (c.IsPressed(Definition.BoolButtons[6]))
			{
				result |= 0x02;
			}
			if (c.IsPressed(Definition.BoolButtons[7]))
			{
				result |= 0x01;
			}

			return result;
		}

		private static readonly string[] BaseDefinition =
		{
			"Up", "Down", "Left", "Right", "Start", "Select", "B", "A", "Power"
		};

		public void SyncState(Serializer ser)
		{
			//nothing
		}
	}
}