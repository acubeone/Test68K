#include "wrapper.h"

#include "types.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct CPU_Context {
	u8 *ram;
} CPU_Context;

static CPU_Context _ctx;

bool cpu_init() {
	memset(&_ctx, 0, sizeof(CPU_Context));
	_ctx.ram = malloc(T68K_MAX_RAM);
	assert(_ctx.ram && "Failed to allocate RAM");
	if (!_ctx.ram)
		return false;
	memset(_ctx.ram, 0, T68K_MAX_RAM);

	return true;
}

void cpu_deinit() {
	if (_ctx.ram)
		free(_ctx.ram);
}

bool cpu_load_rom(usize size, u8 bytes[static size]) {
	assert(bytes);
	if (size == 0)
		return false;

	memcpy(_ctx.ram, bytes, size);
	return true;
}

bool cpu_read_byte(u32 addr, u8 *byte) {
	assert(_ctx.ram);
	assert(byte);
	addr &= 0x003f'ffff;

	*byte = _ctx.ram[addr];
	return true;
}

bool cpu_read_word(u32 addr, u16 *word) {
	assert(_ctx.ram);
	assert(word);
	addr &= 0x003f'ffff;

	// prevent wraparound in word access
	if ((addr + 1) >= T68K_MAX_RAM)
		return false;

	u16 hi = _ctx.ram[addr] << 8;
	u16 lo = _ctx.ram[addr + 1];
	*word = hi | lo;
	return true;
}

bool cpu_write_byte(u32 addr, u8 byte) {
	assert(_ctx.ram);
	addr &= 0x003f'ffff;

	_ctx.ram[addr] = byte;
	return true;
}

bool cpu_write_word(u32 addr, u16 word) {
	assert(_ctx.ram);
	addr &= 0x003f'ffff;

	// prevent wraparound in word access
	if ((addr + 1) >= T68K_MAX_RAM)
		return false;

	_ctx.ram[addr + 0] = (u8)((word >> 8) & 0xff);
	_ctx.ram[addr + 1] = (u8)(word & 0xff);
	return true;
}

//===================
// Musashi callbacks
//===================

extern u32 m68k_read_memory_8(u32 addr) {
	u8 byte;
	bool err = cpu_read_byte(addr, &byte);
	assert(err);

	return byte;
}

extern u32 m68k_read_memory_16(u32 addr) {
	u16 word;
	bool err = cpu_read_word(addr, &word);
	assert(err);

	return word;
}

extern u32 m68k_read_memory_32(u32 addr) {
	u16 hi;
	bool err = cpu_read_word(addr, &hi);
	assert(err);

	u16 lo;
	err = cpu_read_word(addr + 2, &lo);
	assert(err);

	return ((u32)hi << 16) | (u32)lo;
}

extern void m68k_write_memory_8(u32 addr, u32 byte) {
	bool err = cpu_write_byte(addr, byte);
	assert(err);
}

extern void m68k_write_memory_16(u32 addr, u32 word) {
	bool err = cpu_write_word(addr, word);
	assert(err);
}

extern void m68k_write_memory_32(u32 addr, u32 long_) {
	bool err;

	err = cpu_write_word(addr, (long_ >> 16) & 0xffff);
	assert(err);

	err = cpu_write_word(addr + 2, long_ & 0xffff);
	assert(err);
}
