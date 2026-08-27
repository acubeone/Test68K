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

#include "basic_types.hpp"

#include <random>
#include <string>
#include <vector>

namespace tcs {

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

struct RamSlice {
	u32 addr;
	std::vector<u8> data;
};

struct State {
	u32 regs[REGS_COUNT];
	std::vector<RamSlice> ram;
};

struct MemOp {
	u32 addr;
	u16 data;
	bool is_write;
	bool is_word;
	u8 fc;
};

struct Entry {
	std::string name;
	u32 cycles;
	u16 vector;

	std::vector<u16> op_words;
	std::vector<MemOp> mem_ops;

	State pre;
	State post;
};

class Runner {
private:
	CpuModel m_model;
	u64 m_seed;
	std::vector<Entry> m_entries;

	std::mt19937 m_mt;
	std::uniform_int_distribution<u32> m_rand;

public:
	Runner(CpuModel model, u64 seed);

private:
	[[nodiscard]] u64 urand() {
		return m_rand(m_mt);
	}
};

} // namespace tcs
