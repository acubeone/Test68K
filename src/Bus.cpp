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

#include "Bus.hpp"

#include "utils.hpp"

#include <algorithm>
#include <span>
#include <vector>

static constexpr auto _DEFAULT_RAM_SIZE = 64 * 1024; // Default ram size: 64KB

namespace tcs {

Bus::Bus() {
	m_ram.resize(_DEFAULT_RAM_SIZE);
	m_pre_ram.resize(_DEFAULT_RAM_SIZE);
}

void Bus::clear() {
	m_pre_set.clear();
	m_touched_set.clear();
	m_touched_list.clear();
	m_mem_ops.clear();
}

u8 Bus::read_byte(u32 addr, u8 fc) {
	addr &= 0x0000'ffff;
	_track_touched(addr);

	u8 byte = m_ram[addr];
	_register_access(addr, (u16)byte, false, false, fc);
	return byte;
}

u16 Bus::read_word(u32 addr, u8 fc) {
	u32 hi_addr = (addr + 0) & 0xffff;
	u32 lo_addr = (addr + 1) & 0xffff;

	_track_touched(hi_addr);
	_track_touched(lo_addr);

	u16 word = (m_ram[hi_addr] << 8) | m_ram[lo_addr];
	_register_access(hi_addr, word, true, false, fc);
	return word;
}

void Bus::write_byte(u32 addr, u8 byte, u8 fc) {
	addr &= 0x0000'ffff;

	if (!capture_enabled)
		_track_pre(addr, m_ram[addr]);
	_track_touched(addr);

	m_ram[addr] = byte;
	_register_access(addr, (u16)byte, false, true, fc);
}

void Bus::write_word(u32 addr, u16 word, u8 fc) {
	u32 hi_addr = (addr + 0) & 0xffff;
	u32 lo_addr = (addr + 1) & 0xffff;

	if (!capture_enabled) {
		_track_pre(hi_addr, m_ram[hi_addr]);
		_track_pre(lo_addr, m_ram[lo_addr]);
	}
	_track_touched(hi_addr);
	_track_touched(lo_addr);

	m_ram[hi_addr] = (word >> 8) & 0xff;
	m_ram[lo_addr] = word & 0xff;
	_register_access(addr, word, true, true, fc);
}

void Bus::write_long(u32 addr, u32 long_, u8 fc) {
	write_word(addr + 0, static_cast<u16>(long_ >> 16), fc);
	write_word(addr + 2, static_cast<u16>(long_ & 0xffff), fc);
}

void Bus::write_block(u32 addr, std::span<const u16> words, u8 fc) {
	for (u16 word : words) {
		u32 hi_addr = (addr + 0) & 0xffff;
		u32 lo_addr = (addr + 1) & 0xffff;

		if (!capture_enabled) {
			_track_pre(hi_addr, m_ram[hi_addr]);
			_track_pre(lo_addr, m_ram[lo_addr]);
		}
		_track_touched(hi_addr);
		_track_touched(lo_addr);

		m_ram[hi_addr] = (word >> 8) & 0xff;
		m_ram[lo_addr] = word & 0xff;
		_register_access(addr, word, true, true, fc);

		addr += 2;
	}
}

void Bus::gen_diff(std::vector<RamSlice>& pre, std::vector<RamSlice>& post) {
	// Make sure vectors starts empty
	pre.clear();
	post.clear();

	// Starts at lowest touched address
	std::ranges::sort(m_touched_list);

	RamSlice pre_slice {};
	RamSlice post_slice {};

	for (auto addr : m_touched_list) {
		// if address exists in pre_set, it means it was written to in
		// capture-mode, if not, it was only read while accessing
		u8 pre_byte = m_pre_set.contains(addr) ? m_pre_ram[addr] : m_ram[addr];

		if (pre_slice.data.empty())
			pre_slice.addr = addr;

		if (addr != pre_slice.addr + pre_slice.data.size()) {
			pre.push_back(pre_slice);

			pre_slice.addr = addr;
			pre_slice.data.clear();
		}
		pre_slice.data.push_back(pre_byte);

		if (m_ram[addr] == pre_byte)
			continue; // Ignore non-mutated and read-only bytes

		if (post_slice.data.empty())
			post_slice.addr = addr;

		if (addr != post_slice.addr + post_slice.data.size()) {
			post.push_back(post_slice);

			post_slice.addr = addr;
			post_slice.data.clear();
		}

		post_slice.data.push_back(m_ram[addr]);
	}

	// Store remaining uncompleted slices
	if (!pre_slice.data.empty())
		pre.push_back(pre_slice);
	if (!post_slice.data.empty())
		post.push_back(post_slice);
}

void Bus::_register_access(u32 addr, u16 data, bool is_word, bool is_write, u8 fc) {
	if (!capture_enabled)
		return; // Ignore accesses on non-capturing mode

	MemOp op {
		.addr = addr,
		.data = static_cast<u16>(is_word ? data : (data & 0x00ff)),
		.is_write = is_write,
		.is_word = is_word,
		.fc = fc,
	};
	m_mem_ops.push_back(op);
}

void Bus::_track_touched(u32 addr) {
	if (m_touched_set.contains(addr))
		return;

	m_touched_set.insert(addr);
	m_touched_list.push_back(addr);
}

void Bus::_track_pre(u32 addr, u8 byte) {
	if (m_pre_set.contains(addr))
		return;

	m_pre_set.insert(addr);
	m_pre_ram[addr] = byte;
}

} // namespace tcs
