#include "Bus.hpp"
#include "unity.h"
#include "unity_internals.h"

#include <vector>

static tcs::Bus *bus = nullptr;

void setUp(void) {
	bus = new tcs::Bus();
}

void tearDown(void) {
	delete bus;
	bus = nullptr;
}

static void test_gen_diff_read_only_in_execution() {
	std::vector<tcs::RamSlice> pre;
	std::vector<tcs::RamSlice> post;

	// 1. capture phase: setup initial opcode/RAM state
	bus->clear_ram();
	bus->capture_enabled = true;
	bus->write_byte(0x1000, 0x12, 1);
	bus->write_byte(0x1001, 0x34, 1);

	// 2. Execution phase: instruction reads opcode
	bus->capture_enabled = false;
	bus->read_word(0x1000, 1);

	bus->gen_diff(pre, post);

	// Pre state captures initial setup bytes
	TEST_ASSERT_EQUAL_UINT32(1, pre.size());
	TEST_ASSERT_EQUAL_UINT32(0x1000, pre[0].addr);
	TEST_ASSERT_EQUAL_UINT32(2, pre[0].data.size());
	TEST_ASSERT_EQUAL_UINT8(0x12, pre[0].data[0]);
	TEST_ASSERT_EQUAL_UINT8(0x34, pre[0].data[1]);

	// Read operations produce no post-execution diffs
	TEST_ASSERT_EQUAL_UINT32(0, post.size());
}

static void test_gen_diff_single_byte_mutation() {
	std::vector<tcs::RamSlice> pre;
	std::vector<tcs::RamSlice> post;

	// 1. capture phase: pre-populate address
	bus->clear_ram();
	bus->capture_enabled = true;
	bus->write_byte(0x2000, 0x00, 1);

	// 2. Execution phase: mutate memory
	bus->capture_enabled = false;
	bus->write_byte(0x2000, 0xAB, 1);

	bus->gen_diff(pre, post);

	TEST_ASSERT_EQUAL_UINT32(1, pre.size());
	TEST_ASSERT_EQUAL_UINT32(0x2000, pre[0].addr);
	TEST_ASSERT_EQUAL_UINT8(0x00, pre[0].data[0]);

	TEST_ASSERT_EQUAL_UINT32(1, post.size());
	TEST_ASSERT_EQUAL_UINT32(0x2000, post[0].addr);
	TEST_ASSERT_EQUAL_UINT8(0xAB, post[0].data[0]);
}

static void test_gen_diff_contiguous_and_gapped_slices() {
	std::vector<tcs::RamSlice> pre;
	std::vector<tcs::RamSlice> post;

	// 1. capture phase: setup initial memory regions
	bus->clear_ram();
	bus->capture_enabled = true;
	bus->write_byte(0x1000, 0x00, 1);
	bus->write_byte(0x1001, 0x00, 1);
	bus->write_byte(0x1002, 0x00, 1);
	bus->write_byte(0x1005, 0x00, 1);

	// 2. Execution phase: perform writes with a gap
	bus->capture_enabled = false;
	bus->write_byte(0x1000, 0x11, 1);
	bus->write_byte(0x1001, 0x22, 1);
	bus->write_byte(0x1002, 0x33, 1);
	bus->write_byte(0x1005, 0x99, 1);

	bus->gen_diff(pre, post);

	// Verify pre slices (0x1000..0x1002 and 0x1005)
	TEST_ASSERT_EQUAL_UINT32(2, pre.size());
	TEST_ASSERT_EQUAL_UINT32(0x1000, pre[0].addr);
	TEST_ASSERT_EQUAL_UINT32(3, pre[0].data.size());
	TEST_ASSERT_EQUAL_UINT32(0x1005, pre[1].addr);
	TEST_ASSERT_EQUAL_UINT32(1, pre[1].data.size());

	// Verify post slices
	TEST_ASSERT_EQUAL_UINT32(2, post.size());
	TEST_ASSERT_EQUAL_UINT32(0x1000, post[0].addr);
	TEST_ASSERT_EQUAL_UINT32(3, post[0].data.size());
	TEST_ASSERT_EQUAL_UINT8(0x11, post[0].data[0]);
	TEST_ASSERT_EQUAL_UINT8(0x22, post[0].data[1]);
	TEST_ASSERT_EQUAL_UINT8(0x33, post[0].data[2]);

	TEST_ASSERT_EQUAL_UINT32(0x1005, post[1].addr);
	TEST_ASSERT_EQUAL_UINT8(0x99, post[1].data[0]);
}

static void test_gen_diff_write_same_value_ignored_in_post() {
	std::vector<tcs::RamSlice> pre;
	std::vector<tcs::RamSlice> post;

	// 1. capture phase: populate value 0x55
	bus->clear_ram();
	bus->capture_enabled = true;
	bus->write_byte(0x3000, 0x55, 1);

	// 2. Execution phase: write exact same value back
	bus->capture_enabled = false;
	bus->write_byte(0x3000, 0x55, 1);

	bus->gen_diff(pre, post);

	TEST_ASSERT_EQUAL_UINT32(1, pre.size());
	TEST_ASSERT_EQUAL_UINT8(0x55, pre[0].data[0]);

	// Unchanged byte ignored in post vector
	TEST_ASSERT_EQUAL_UINT32(0, post.size());
}

int main() {
	UNITY_BEGIN();
	RUN_TEST(test_gen_diff_read_only_in_execution);
	RUN_TEST(test_gen_diff_single_byte_mutation);
	RUN_TEST(test_gen_diff_contiguous_and_gapped_slices);
	RUN_TEST(test_gen_diff_write_same_value_ignored_in_post);
	return UNITY_END();
}
