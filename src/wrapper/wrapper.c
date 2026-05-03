#include "wrapper.h"

#include "m68k.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static u8 *_ram = nullptr;

static u8 *_pre_ram = nullptr;
static bool *_pre_set = nullptr;
static bool *_touched_set = nullptr;
static u32 _touched_list[CPU_MAX_RAM_DIFF * 8] = {};
static u32 _touched_list_len = 0;

static CPU_TestCase _test_case = {};
static u8 _current_fc = 0;
static bool _trace_enabled = false;

#define FATAL_EXIT(...) _fatal_exit(__func__, __LINE__, __VA_ARGS__)

[[noreturn]]
static void _fatal_exit(const char *func, u32 line, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "\n[FATAL](%s:%d): ", func, line);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);

	exit(EXIT_FAILURE);
}

static void _register_ram_access(u32 addr, u16 data, bool is_word, bool is_write) {
	if (!_trace_enabled)
		return;

	if (_test_case.mem_op_len >= CPU_MAX_MEM_OPS) {
		_test_case.mem_ops_overflow = true;
		return;
	}

	CPU_MemOp op = {};
	op.kind = is_write ? CPU_MEM_WRITE : CPU_MEM_READ;
	op.addr = addr;
	op.fc = _current_fc;
	op.is_word = is_word;

	if (is_word)
		op.data = data & 0xffff;
	else
		op.data = data & 0x00ff;

	_test_case.mem_ops[_test_case.mem_op_len] = op;
	_test_case.mem_op_len += 1;
}

static void _ram_track_touched(u32 addr) {
	addr &= 0x003f'ffff;
	if (_touched_set[addr])
		return;

	_touched_set[addr] = true;
	if (_touched_list_len >= CPU_MAX_RAM_DIFF * 8) {
		_test_case.touched_list_overflow = true;
		return;
	}

	_touched_list[_touched_list_len] = addr;
	_touched_list_len += 1;
}

static void _ram_track_pre(u32 addr, u8 byte) {
	addr &= 0x003f'ffff;
	if (_pre_set[addr])
		return;

	_pre_set[addr] = true;
	_pre_ram[addr] = byte;
}

static void _ram_append(CPU_State *state, u32 addr, u8 byte) {
	addr &= 0x003f'ffff;
	if (state->ram_len >= CPU_MAX_RAM_DIFF) {
		_test_case.ram_diff_overflow = true;
		return;
	}

	state->ram[state->ram_len].addr = addr;
	state->ram[state->ram_len].byte = byte;
	state->ram_len += 1;
}

static void _ram_diff(CPU_State *pre, CPU_State *post) {
	assert(pre);
	assert(post);

	pre->ram_len = 0;
	post->ram_len = 0;
	for (u32 i = 0; i < _touched_list_len; i += 1) {
		u32 addr = _touched_list[i];
		u8 now = _ram[addr];

		bool has_pre = _pre_set[addr];
		u8 prev = has_pre ? _pre_ram[addr] : now;

		if (has_pre)
			_ram_append(pre, addr, prev);

		if (now != prev)
			_ram_append(post, addr, now);
	}
}

bool cpu_init() {
	_ram = malloc(CPU_MAX_RAM);
	if (!_ram)
		FATAL_EXIT("Failed to allocate RAM");
	memset(_ram, 0, CPU_MAX_RAM);

	_pre_ram = malloc(CPU_MAX_RAM);
	_pre_set = malloc(CPU_MAX_RAM);
	_touched_set = malloc(CPU_MAX_RAM);
	if (!_pre_ram || !_pre_set || !_touched_set)
		FATAL_EXIT("Failed to allocate resources");

	m68k_init();
	return true;
}

void cpu_deinit() {
	if (_ram)
		free(_ram);

	if (_pre_ram)
		free(_pre_ram);
	if (_pre_set)
		free(_pre_set);
	if (_touched_set)
		free(_touched_set);
}

void cpu_begin_test_case(const char *name, CPU_Model model, u64 seed) {
	assert(name);

	if (model == CPU_MODEL_M68010)
		m68k_set_cpu_type(M68K_CPU_TYPE_68010);
	else
		m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	memset(&_test_case, 0, sizeof(CPU_TestCase));
	cpu_clear_ram();

	strncpy(_test_case.name, name, CPU_MAX_NAME_LEN);
	_test_case.name[CPU_MAX_NAME_LEN - 1] = '\0';

	_test_case.model = model;
	_test_case.seed = seed;
	_test_case.exec_result = CPU_EXEC_OK;
	_test_case.exception_vector = 0;

	memset(_pre_set, 0, CPU_MAX_RAM);
	memset(_touched_set, 0, CPU_MAX_RAM);
	_touched_list_len = 0;
}

void cpu_query_test_case(CPU_TestCase *test) {
	assert(test);
	memcpy(test, &_test_case, sizeof(CPU_TestCase));
}

void cpu_capture_pre() {
	for (u32 i = 0; i < CPU_REG_COUNT; i += 1) {
		_test_case.pre.regs[i] = m68k_get_reg(nullptr, i);
	}

	_trace_enabled = true;
}

void cpu_capture_post() {
	for (int i = 0; i < CPU_REG_COUNT; i += 1) {
		_test_case.post.regs[i] = m68k_get_reg(nullptr, i);
	}

	_ram_diff(&_test_case.pre, &_test_case.post);
	_trace_enabled = false;
}

void cpu_clear_ram() {
	assert(_ram);
	memset(_ram, 0, CPU_MAX_RAM);
}

void cpu_reset() {
	m68k_pulse_reset();
}

void cpu_set_op_words(const u16 *words, u8 count) {
	assert(words);

	if (count >= CPU_MAX_OP_WORDS)
		count = CPU_MAX_OP_WORDS;

	_test_case.op_word_count = count;
	for (u8 i = 0; i < count; i += 1) {
		_test_case.op_words[i] = words[i];
	}
}

u32 cpu_execute(u32 cycles) {
	u32 used_cycles = m68k_execute(cycles);
	_test_case.cycles += used_cycles;
	return used_cycles;
}

u32 cpu_get_reg(u8 reg) {
	return m68k_get_reg(nullptr, reg);
}

void cpu_set_reg(u8 reg, u32 data) {
	m68k_set_reg(reg, data);
}

u8 cpu_read_byte(u32 addr) {
	assert(_ram);
	addr &= 0x003f'ffff;

	_ram_track_touched(addr);

	u8 byte = _ram[addr];
	_register_ram_access(addr, (u16)byte, false, false);
	return byte;
}

u16 cpu_read_word(u32 addr) {
	assert(_ram);
	addr &= 0x003f'ffff;

	// prevent wraparound in word access
	if ((addr + 1) >= CPU_MAX_RAM) {
		_test_case.exec_result = CPU_EXEC_ADDR;
		_test_case.exception_vector = 0x03;
		return 0;
	}

	_ram_track_touched(addr + 0);
	_ram_track_touched(addr + 1);

	u16 hi = _ram[addr + 0];
	u16 lo = _ram[addr + 1];
	u16 word = (hi << 8) | lo;

	_register_ram_access(addr, word, true, false);
	return word;
}

void cpu_write_byte(u32 addr, u8 byte) {
	assert(_ram);
	addr &= 0x003f'ffff;

	if (_trace_enabled)
		_ram_track_pre(addr, _ram[addr]);
	_ram_track_touched(addr);

	_ram[addr] = byte;
	_register_ram_access(addr, (u16)byte, false, true);
}

void cpu_write_word(u32 addr, u16 word) {
	assert(_ram);
	addr &= 0x003f'ffff;

	// prevent wraparound in word access
	if ((addr + 1) >= CPU_MAX_RAM) {
		_test_case.exec_result = CPU_EXEC_ADDR;
		_test_case.exception_vector = 0x03;
		return;
	}

	if (_trace_enabled) {
		_ram_track_pre(addr + 0, _ram[addr + 0]);
		_ram_track_pre(addr + 1, _ram[addr + 1]);
	}

	_ram_track_touched(addr + 0);
	_ram_track_touched(addr + 1);

	_ram[addr + 0] = (u8)((word >> 8) & 0x00ff);
	_ram[addr + 1] = (u8)(word & 0x00ff);
	_register_ram_access(addr, word, true, true);
}

void cpu_set_fc(u8 fc) {
	_current_fc = fc;
}

i32 cpu_tas() {
	_test_case.exec_result = CPU_EXEC_TAS;
	_test_case.exception_vector = 0;
	return 1;
}

i32 cpu_exception_illegal(u16 opcode) {
	u8 class = (opcode & 0xf000) >> 12;
	if (class == 0x0a) {
		_test_case.exec_result = CPU_EXEC_LINEA;
		_test_case.exception_vector = 0x0a;
		return 0;
	}

	if (class == 0x0f) {
		_test_case.exec_result = CPU_EXEC_LINEF;
		_test_case.exception_vector = 0x0b;
		return 0;
	}

	_test_case.exec_result = CPU_EXEC_ILLEGAL;
	_test_case.exception_vector = 0x04;
	return 0;
}

i32 cpu_exception_trap(u8 trap) {
	_test_case.exec_result = CPU_EXEC_TRAP;
	_test_case.exception_vector = 0x20 + trap;
	return 0;
}

extern u32 m68k_read_memory_8(u32 addr) {
	return cpu_read_byte(addr);
}

extern u32 m68k_read_memory_16(u32 addr) {
	return cpu_read_word(addr);
}

extern u32 m68k_read_memory_32(u32 addr) {
	return (cpu_read_word(addr) << 16) | cpu_read_word(addr + 2);
}

extern void m68k_write_memory_8(u32 addr, u32 value) {
	cpu_write_byte(addr, value);
}

extern void m68k_write_memory_16(u32 addr, u32 value) {
	cpu_write_word(addr, value);
}

extern void m68k_write_memory_32(u32 addr, u32 value) {
	cpu_write_word(addr, (value >> 16) & 0xffff);
	cpu_write_word(addr + 2, value & 0xffff);
}
