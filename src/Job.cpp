// Test68K - Test Suite generator for the M68000 and M68010
//
// Copyright (c) 2026 acubeone
// Email: acube_one@disroot.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#include "Job.hpp"

#include "Moira/MoiraTypes.h"
#include "utils.hpp"

#include <array>
#include <cstring>
#include <span>

static constexpr u16 _RESET_PC = 0xc000;

// clang-format off
static constexpr std::array<u16, 256> _vector_table {
	0x0000, 0x8ffe, // Initial SP: 0x00008ffe
	0x0000, 0xc000, // Reset PC: 0x0000c000
};
// clang-format on

namespace tcs {

Job::Job(u64 seed) {
	m_mt.seed(seed);
	m_current.name.resize(128);
}

std::span<Entry> Job::run(moira::Model model, u16 start, u16 len) {
	setModel(model);

	// Reset entries length to reuse capacity
	m_entries.clear();

	for (u16 op = start; op < len; op += 1) {
		if ((op & 0xf000) == 0xa000 || (op & 0xf000) == 0xf000)
			continue; // Ignore line-a and line-f

		auto info = getInstrInfo(op);
		if (!isAvailable(model, info.I, info.M, info.S))
			continue;

		_capture_pre(op, info);
		reset();

		// moira::reset clears registers, so we set them here again
		for (u8 i = 0; i < 8; i += 1) {
			setD(i, m_current.pre_regs[REG_D0 + i]);
			setA(i, m_current.pre_regs[REG_A0 + i]);
		}
		setSR(m_current.pre_regs[REG_SR]);
		setUSP(m_current.pre_regs[REG_USP]);
		setISP(m_current.pre_regs[REG_SSP]);
		setVBR(m_current.pre_regs[REG_VBR]);
		setSFC(m_current.pre_regs[REG_SFC]);
		setDFC(m_current.pre_regs[REG_DFC]);

		execute();
		_capture_post();

		// Entry is complete
		m_entries.push_back(m_current);
		m_current.vector = 0;
		m_current.instr.fill(0);
		m_current.mem_ops.clear();
	}

	return m_entries;
}

template <typename T>
void Job::_randomize_region(std::span<T> region) {
	for (auto& el : region) {
		el = static_cast<T>(_rand());
	}
}

u32 Job::_decode_brief_extension(u16 ext, u32 base_addr) const {
	bool is_addr = (ext >> 15) & 1u;
	bool is_long = (ext >> 11) & 1u;
	i32 disp = static_cast<i32>(static_cast<i8>(ext & 0xff));
	u8 reg_num = (ext >> 12) & 0x07;

	u32 ureg = is_addr ? m_current.pre_regs[REG_A0 + reg_num]
					   : m_current.pre_regs[REG_D0 + reg_num];
	i32 xreg = is_long ? static_cast<i32>(ureg)
					   : static_cast<i32>(static_cast<i16>(ureg));

	return base_addr + disp + xreg;
}

void Job::_randomize_addresses(const moira::InstrInfo& info) {
	for (u8 i = REG_A0; i <= REG_A7; i += 1) {
		u32 areg = m_current.pre_regs[i];

		// (An) / (An)+
		m_bus.write_long(areg, _rand(), 0b001);

		// -(An)
		if (i == REG_A7 && info.S == moira::Byte)
			m_bus.write_long(areg - 2, _rand(), 0b001);
		else
			m_bus.write_long(areg - info.S, _rand(), 0b001);

		// (d16, An)
		// We don't know where the displacement is, so do all possible words
		for (u8 j = 1; j < _EXT_WORDS_COUNT; j += 1) {
			i32 disp = static_cast<i32>(static_cast<i16>(m_current.instr[j]));
			m_bus.write_long(areg + disp, _rand(), 0b001);
		}

		// (d16, An, Xi)
		for (u8 j = 1; j <= 2; j += 1) {
			u16 ext = m_current.instr[j];

			u32 addr = _decode_brief_extension(ext, areg);
			m_bus.write_long(addr, _rand(), 0b001);
		}
	}

	// (d16, PC)
	i32 disp = static_cast<i32>(static_cast<i16>(m_current.instr[1]));
	m_bus.write_long(_RESET_PC + 2 + disp, _rand(), 0b001);

	// (d16, PC, Xi)
	u16 ext = m_current.instr[2];
	u32 addr = _decode_brief_extension(ext, _RESET_PC + 2);
	m_bus.write_word(addr, _rand(), 0b001);

	// (xxx).w
	u32 src_abs_w = static_cast<i32>(static_cast<i16>(m_current.instr[1]));
	u32 dst_abs_w = static_cast<i32>(static_cast<i16>(m_current.instr[2]));
	u32 rand = _rand();
	m_bus.write_word(src_abs_w, (rand >> 16) & 0xffff, 0b001);
	m_bus.write_word(dst_abs_w, rand & 0xffff, 0b001);

	// (xxx).l
	u32 src_abs_l = static_cast<u32>(m_current.instr[1]) << 16 | m_current.instr[2];
	u32 dst_abs_l = static_cast<u32>(m_current.instr[3]) << 16 | m_current.instr[4];
	m_bus.write_long(src_abs_l, _rand(), 0b001);
	m_bus.write_long(dst_abs_l, _rand(), 0b001);
}

void Job::_capture_pre(u16 op, const moira::InstrInfo& info) {
	// Build pre-state
	_randomize_region<u16>(m_current.instr);
	_randomize_region<u32>(m_current.pre_regs);

	// Fix status register
	m_current.pre_regs[REG_SR] &= 0xa71f;

	// Fix misaligned address registers
	for (u8 i = REG_A0; i <= REG_A6; i++) {
		m_current.pre_regs[i] &= ~1u;
	}
	m_current.pre_regs[REG_USP] &= ~1u;
	m_current.pre_regs[REG_SSP] &= ~1u;

	bool is_supervisor = (m_current.pre_regs[REG_SR] >> 13) & 1u;
	m_current.pre_regs[REG_A7] = is_supervisor ? m_current.pre_regs[REG_SSP]
											   : m_current.pre_regs[REG_USP];
	m_current.pre_regs[REG_PC] = _RESET_PC;
	m_current.pre_regs[REG_VBR] = 0;

	// First entry is always the instruction opcode
	m_current.instr[0] = op;

	// We don't know if extension-words are treated as indexed, so we need
	// to make them valid to prevent 'Illegal Exceptions'
	for (u8 i = 1; i < _EXT_WORDS_COUNT; i += 1)
		m_current.instr[i] &= 0xf8ff;

	m_bus.clear();
	m_bus.capture_enabled = false;

	m_bus.write_block(0x0000, _vector_table, 0b110);
	m_bus.write_block(_RESET_PC, m_current.instr, 0b010);

	disassemble(m_current.name.data(), _RESET_PC);
	m_current.name.resize(std::strlen(m_current.name.data()));
	_randomize_addresses(info);

	m_bus.capture_enabled = true;
}

void Job::_capture_post() {
	m_current.cycles = getClock();

	for (u8 i = 0; i < 8; i += 1) {
		m_current.post_regs[REG_D0 + i] = getD(i);
		m_current.post_regs[REG_A0 + i] = getA(i);
	}
	m_current.post_regs[REG_PC] = getPC();
	m_current.post_regs[REG_SR] = getSR();
	m_current.post_regs[REG_USP] = getUSP();
	m_current.post_regs[REG_SSP] = getISP();
	m_current.post_regs[REG_VBR] = getVBR();
	m_current.post_regs[REG_SFC] = getSFC();
	m_current.post_regs[REG_DFC] = getDFC();

	bool is_supervisor = (m_current.post_regs[REG_SR] >> 13) & 1u;
	m_current.post_regs[REG_A7] = is_supervisor ? m_current.post_regs[REG_SSP]
												: m_current.post_regs[REG_USP];

	// Get RAM operations and diffs
	m_bus.gen_diff(m_current.pre_ram, m_current.post_ram);
	auto mem_ops = m_bus.query_operations();
	m_current.mem_ops.assign(mem_ops.begin(), mem_ops.end());
}

u8 Job::read8(u32 addr) const {
	return m_bus.read_byte(addr, readFC());
}

u16 Job::read16(u32 addr) const {
	return m_bus.read_word(addr, readFC());
}

void Job::write8(u32 addr, u8 byte) const {
	m_bus.write_byte(addr, byte, readFC());
}

void Job::write16(u32 addr, u16 word) const {
	m_bus.write_word(addr, word, readFC());
}

void Job::willExecute(moira::M68kException exc, u16 vector) {
	(void)exc;
	m_current.vector = vector;
}

} // namespace tcs
