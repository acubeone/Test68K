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

#pragma once

#include "Bus.hpp"
#include "Moira/Moira.h"
#include "Moira/MoiraTypes.h"
#include "utils.hpp"

#include <array>
#include <random>
#include <span>
#include <string>
#include <vector>

namespace tcs {

constexpr auto _EXT_WORDS_COUNT = 12;

enum CpuModel : u8 {
	CPU_M68000 = 0,
	CPU_M68010 = 1,
};

enum Registers : u8 {
	REG_D0 = 0,
	REG_D1,
	REG_D2,
	REG_D3,
	REG_D4,
	REG_D5,
	REG_D6,
	REG_D7,
	REG_A0,
	REG_A1,
	REG_A2,
	REG_A3,
	REG_A4,
	REG_A5,
	REG_A6,
	REG_A7,

	REG_PC,
	REG_SR,
	REG_USP,
	REG_SSP,
	REG_VBR,
	REG_SFC,
	REG_DFC,
	REGS_COUNT,
};

struct Entry {
	std::array<u16, _EXT_WORDS_COUNT> instr;
	u32 cycles;
	u16 vector;
	std::string name;

	u32 pre_regs[REGS_COUNT];
	std::vector<RamSlice> pre_ram;

	u32 post_regs[REGS_COUNT];
	std::vector<RamSlice> post_ram;

	std::vector<MemOp> mem_ops;
};

class Job : protected moira::Moira {
private:
	mutable Bus m_bus;

	Entry m_current {};
	std::vector<Entry> m_entries;

	std::mt19937 m_mt;
	std::uniform_int_distribution<u32> m_uniform_int;

public:
	Job(u64 seed);

	[[nodiscard]] std::span<Entry> run(moira::Model model, u16 start, u16 len);

private:
	[[nodiscard]] u32 _rand() {
		return m_uniform_int(m_mt);
	}

	template <typename T>
	void _randomize_region(std::span<T> region);

	[[nodiscard]] u32 _decode_brief_extension(u16 ext, u32 base_addr) const;
	void _randomize_addresses(const moira::InstrInfo& info);

	void _capture_pre(u16 op, const moira::InstrInfo& info);
	void _capture_post();

protected:
	u8 read8(u32 addr) const override;
	u16 read16(u32 addr) const override;

	void write8(u32 addr, u8 byte) const override;
	void write16(u32 addr, u16 word) const override;

	void willExecute(moira::M68kException exc, u16 vector) override;
};

} // namespace tcs
