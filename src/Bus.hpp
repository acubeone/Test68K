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

#include "utils.hpp"

#include <span>
#include <unordered_set>
#include <vector>

namespace tcs {

struct RamSlice {
	u32 addr;
	std::vector<u8> data;
};

struct MemOp {
	u32 addr;
	u16 data;
	bool is_write;
	bool is_word;
	u8 fc;
};

class Bus {
public:
	bool capture_enabled = false; // Only enabled on CPU execution

private:
	std::vector<u8> m_ram;
	std::vector<MemOp> m_mem_ops;

	std::unordered_set<u32> m_pre_set;
	std::vector<u8> m_pre_ram;

	std::unordered_set<u32> m_touched_set;
	std::vector<u32> m_touched_list;

public:
	Bus();

	void clear();

	u8 read_byte(u32 addr, u8 fc);
	u16 read_word(u32 addr, u8 fc);

	void write_byte(u32 addr, u8 byte, u8 fc);
	void write_word(u32 addr, u16 word, u8 fc);
	void write_long(u32 addr, u32 long_, u8 fc);

	void write_block(u32 addr, std::span<const u16> words, u8 fc);

	void gen_diff(std::vector<RamSlice>& pre, std::vector<RamSlice>& post);

	[[nodiscard]] std::span<const MemOp> query_operations() {
		return m_mem_ops;
	}

private:
	void _register_access(u32 addr, u16 data, bool is_word, bool is_write, u8 fc);
	void _track_touched(u32 addr);
	void _track_pre(u32 addr, u8 byte);
};

} // namespace tcs
