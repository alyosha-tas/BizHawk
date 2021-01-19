// SNESHawk.cpp : Defines the exported functions for the DLL.
//

#include "SNESHawk.h"
#include "Core.h"

#include <iostream>
#include <cstdint>
#include <iomanip>
#include <string>

using namespace SNESHawk;

#pragma region Core
// Create pointer to a core instance
SNESHawk_EXPORT SNESCore* SNES_create()
{
	return new SNESCore();
}

// free the memory from the core pointer
SNESHawk_EXPORT void SNES_destroy(SNESCore* p)
{
	delete p->MemMap.bios_rom;
	delete p->MemMap.ROM;
	delete p->MemMap.mapped_ROM;
	std::free(p);
}

// load bios into the core
SNESHawk_EXPORT void SNES_load_bios(SNESCore* p, uint8_t* bios)
{
	p->Load_BIOS(bios);
}

// load a rom into the core
SNESHawk_EXPORT void SNES_load(SNESCore* p, uint8_t* ext_rom, uint32_t ext_rom_size, char* MD5, bool is_PAL)
{
	string MD5_s(MD5, 32);
	
	p->Load_ROM(ext_rom, ext_rom_size, MD5_s, is_PAL);
}

// Hard reset (note: does not change RTC, that only happens on load)
SNESHawk_EXPORT void SNES_Reset(SNESCore* p)
{
	p->Reset();
}

// advance a frame
SNESHawk_EXPORT bool SNES_frame_advance(SNESCore* p, uint8_t new_ctrl1, uint8_t new_ctrl2, bool render, bool sound, bool hard_reset, bool soft_reset)
{
	return p->FrameAdvance(new_ctrl1, new_ctrl2, render, sound, hard_reset, soft_reset);
}

// advance a single step
SNESHawk_EXPORT void SNES_do_single_step(SNESCore* p)
{
	p->do_single_step();
}

// send video data to external video provider
SNESHawk_EXPORT void SNES_get_video(SNESCore* p, uint32_t* dest)
{
	p->GetVideo(dest);
}

// send audio data to external audio provider
SNESHawk_EXPORT uint32_t SNES_get_audio(SNESCore* p, int32_t* dest_L, int32_t* n_samp_L, int32_t* dest_R, int32_t* n_samp_R)
{
	return p->GetAudio(dest_L, n_samp_L, dest_R, n_samp_R);
}
#pragma endregion

#pragma region State Save / Load

// save state
SNESHawk_EXPORT void SNES_save_state(SNESCore* p, uint8_t* saver)
{
	p->SaveState(saver);
}

// load state
SNESHawk_EXPORT void SNES_load_state(SNESCore* p, uint8_t* loader)
{
	p->LoadState(loader);
}

#pragma endregion

#pragma region Memory Domain Functions

SNESHawk_EXPORT uint8_t SNES_getrom(SNESCore* p, uint32_t addr) {
	return p->GetROM(addr);
}

SNESHawk_EXPORT uint8_t SNES_getram(SNESCore* p, uint32_t addr) {
	return p->GetRAM(addr);
}

SNESHawk_EXPORT uint8_t SNES_getvram(SNESCore* p, uint32_t addr) {
	return p->GetVRAM(addr);
}

SNESHawk_EXPORT uint8_t SNES_getoam(SNESCore* p, uint32_t addr) {
	return p->GetOAM(addr);
}

SNESHawk_EXPORT uint8_t SNES_getsysbus(SNESCore* p, uint32_t addr) {
	return p->GetSysBus(addr);
}

SNESHawk_EXPORT void SNES_setrom(SNESCore* p, uint32_t addr, uint8_t value) {
	p->SetROM(addr, value);
}

SNESHawk_EXPORT void SNES_setram(SNESCore* p, uint32_t addr, uint8_t value) {
	 p->SetRAM(addr, value);
}

SNESHawk_EXPORT void SNES_setvram(SNESCore* p, uint32_t addr, uint8_t value) {
	 p->SetVRAM(addr, value);
}

SNESHawk_EXPORT void SNES_setoam(SNESCore* p, uint32_t addr, uint8_t value) {
	 p->SetOAM(addr, value);
}

SNESHawk_EXPORT void SNES_setsysbus(SNESCore* p, uint32_t addr, uint8_t value) {
	 p->SetSysBus(addr, value);
}

#pragma endregion

#pragma region Tracer

// set tracer callback
SNESHawk_EXPORT void SNES_settracecallback(SNESCore* p, void (*callback)(int)) {
	p->SetTraceCallback(callback);
}

// return the cpu trace header length
SNESHawk_EXPORT int SNES_getheaderlength(SNESCore* p) {
	return p->GetHeaderLength();
}

// return the cpu disassembly length
SNESHawk_EXPORT int SNES_getdisasmlength(SNESCore* p) {
	return p->GetDisasmLength();
}

// return the cpu register string length
SNESHawk_EXPORT int SNES_getregstringlength(SNESCore* p) {
	return p->GetRegStringLength();
}

// return the cpu trace header
SNESHawk_EXPORT void SNES_getheader(SNESCore* p, char* h, int l) {
	p->GetHeader(h, l);
}

// return the cpu register state
SNESHawk_EXPORT void SNES_getregisterstate(SNESCore* p, char* r, int t, int l) {
	p->GetRegisterState(r, t, l);
}

// return the cpu disassembly
SNESHawk_EXPORT void SNES_getdisassembly(SNESCore* p, char* d, int t, int l) {
	p->GetDisassembly(d, t, l);
}

#pragma endregion

#pragma region Debuggable functions

// return cpu cycle count
SNESHawk_EXPORT uint64_t SNES_cpu_cycles(SNESCore* p) {
	return p->cpu.TotalExecutedCycles;
}

// return cpu registers
SNESHawk_EXPORT uint8_t SNES_cpu_get_regs(SNESCore* p, int reg) {
	uint8_t ret = 0;
	
	switch (reg)
	{
		case (0): ret = p->cpu.A; break;
		case (1): ret = p->cpu.X; break;
		case (2): ret = p->cpu.Y; break;
		case (3): ret = p->cpu.P; break;
		case (4): ret = p->cpu.S; break;
		case (5): ret = (uint8_t)(p->cpu.PC); break;
		case (6): ret = (uint8_t)((p->cpu.PC) >> 8); break;
	}
	return ret;
}

// return cpu flags
SNESHawk_EXPORT bool SNES_cpu_get_flags(SNESCore* p, int reg) {
	bool ret = false;

	switch (reg)
	{
		case (0): ret = p->cpu.FlagCget(); break;
		case (1): ret = p->cpu.FlagZget(); break;
		case (2): ret = p->cpu.FlagIget(); break;
		case (3): ret = p->cpu.FlagDget(); break;
		case (4): ret = p->cpu.FlagBget(); break;
		case (5): ret = p->cpu.FlagTget(); break;
		case (6): ret = p->cpu.FlagVget(); break;
		case (7): ret = p->cpu.FlagNget(); break;
	}

	return ret;
}

// set cpu registers
SNESHawk_EXPORT void SNES_cpu_set_regs(SNESCore* p, int reg, uint8_t value) {
	switch (reg)
	{
		case (0): p->cpu.A = value; break;
		case (1): p->cpu.X = value; break;
		case (2): p->cpu.Y = value; break;
		case (3): p->cpu.P = value; break;
		case (4): p->cpu.S = value; break;
		case (5): p->cpu.PC &= 0xFF00; p->cpu.PC |= value; break;
		case (6): p->cpu.PC &= 0x00FF; p->cpu.PC |= (value << 8); break;
	}
}

// set cpu flags
SNESHawk_EXPORT void SNES_cpu_set_flags(SNESCore* p, int reg, bool value) {

	switch (reg)
	{
		case (0): p->cpu.FlagCset(value); break;
		case (1): p->cpu.FlagZset(value); break;
		case (2): p->cpu.FlagIset(value); break;
		case (3): p->cpu.FlagDset(value); break;
		case (4): p->cpu.FlagBset(value); break;
		case (5): p->cpu.FlagTset(value); break;
		case (6): p->cpu.FlagVset(value); break;
		case (7): p->cpu.FlagNset(value); break;
	}
}

#pragma endregion