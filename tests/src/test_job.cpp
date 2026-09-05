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
#include "unity.h"
#include "unity_internals.h"
#include "utils.hpp"

#include <print>

void setUp(void) { }
void tearDown(void) { }

static void test_job_ignores_line_a_and_f_opcodes() {
	tcs::Job job(12345);

	// Run across 0xA000 range (Line-A) and 0xF000 range (Line-F)
	auto entries_a = job.run(moira::Model::M68000, 0xA000, 0xA010);
	TEST_ASSERT_EQUAL_UINT32(0, entries_a.size());

	auto entries_f = job.run(moira::Model::M68000, 0xF000, 0xF010);
	TEST_ASSERT_EQUAL_UINT32(0, entries_f.size());
}

static void test_job_pre_state_sanitization_and_alignment() {
	tcs::Job job(42);

	// Run a simple NOP instruction (0x4E71)
	auto entries = job.run(moira::Model::M68000, 0x4E71, 0x4E72);
	TEST_ASSERT_EQUAL_UINT32(1, entries.size());

	const auto& entry = entries[0];

	// Verify PC and VBR initialization
	TEST_ASSERT_EQUAL_HEX32(0xC000, entry.pre_regs[tcs::REG_PC]);
	TEST_ASSERT_EQUAL_HEX32(0x0000, entry.pre_regs[tcs::REG_VBR]);

	// Verify SR mask (bits 13, 10-8, 4-0 allowed)
	TEST_ASSERT_BITS_HIGH(0x0000, entry.pre_regs[tcs::REG_SR] & ~0xA71F);

	// Verify word alignment for address registers and stack pointers
	for (u8 i = tcs::REG_A0; i <= tcs::REG_A6; ++i) {
		TEST_ASSERT_EQUAL_UINT32(0, entry.pre_regs[i] & 1u);
	}
	TEST_ASSERT_EQUAL_UINT32(0, entry.pre_regs[tcs::REG_USP] & 1u);
	TEST_ASSERT_EQUAL_UINT32(0, entry.pre_regs[tcs::REG_SSP] & 1u);
}

static void test_job_seed_determinism() {
	// Run two separate jobs with identical seeds on the same opcode range
	tcs::Job job1(999);
	auto entries1 = job1.run(moira::Model::M68000, 0x2000, 0x2005);

	tcs::Job job2(999);
	auto entries2 = job2.run(moira::Model::M68000, 0x2000, 0x2005);

	TEST_ASSERT_EQUAL_UINT32(entries1.size(), entries2.size());

	for (usize i = 0; i < entries1.size(); ++i) {
		TEST_ASSERT_EQUAL_HEX32(
			entries1[i].pre_regs[tcs::REG_D0], entries2[i].pre_regs[tcs::REG_D0]
		);
		TEST_ASSERT_EQUAL_HEX32(
			entries1[i].pre_regs[tcs::REG_A0], entries2[i].pre_regs[tcs::REG_A0]
		);
		TEST_ASSERT_EQUAL_HEX16(entries1[i].instr[0], entries2[i].instr[0]);
	}
}

static void test_job_execution_captures_post_state_and_cycles() {
	tcs::Job job(100);

	// Execute NOP (0x4E71)
	auto entries = job.run(moira::Model::M68000, 0x4E71, 0x4E72);
	TEST_ASSERT_EQUAL_UINT32(1, entries.size());

	const auto& entry = entries[0];

	// NOP takes 4 clock cycles on M68000 and advances PC by 2 bytes
	TEST_ASSERT_GREATER_THAN_UINT32(0, entry.cycles);
	TEST_ASSERT_EQUAL_HEX32(0xC002, entry.post_regs[tcs::REG_PC]);
}

int main() {
	UNITY_BEGIN();

	RUN_TEST(test_job_ignores_line_a_and_f_opcodes);
	RUN_TEST(test_job_pre_state_sanitization_and_alignment);
	RUN_TEST(test_job_seed_determinism);
	RUN_TEST(test_job_execution_captures_post_state_and_cycles);

	return UNITY_END();
}
