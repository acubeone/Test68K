#pragma once

#include <stddef.h>
#include <stdint.h>

typedef int32_t i32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;

enum {
	CPU_MAX_RAM = 1 << 22, // Max flat RAM: 4MB
	CPU_MAX_NAME_LEN = 128,
	CPU_MAX_RAM_DIFF = 4096,
	CPU_MAX_MEM_OPS = 4096,
	CPU_MAX_OP_WORDS = 16,

	CPU_REG_COUNT = 19,
};

typedef enum CPU_Model {
	CPU_MODEL_M68000 = 0,
	CPU_MODEL_M68010,
} CPU_Model;

typedef enum CPU_ExecResult {
	CPU_EXEC_OK = 0,
	CPU_EXEC_ILLEGAL, // Exception: Illegal
	CPU_EXEC_LINEA,	  // Exception: Line A
	CPU_EXEC_LINEF,	  // Exception: Line F
	CPU_EXEC_ADDR,	  // Exception: Address Error
	CPU_EXEC_TRAP,	  // Exception: TRAP #N
	CPU_EXEC_TAS,	  // Instruction: TAS
	CPU_EXEC_OTHER,	  // Other exception
} CPU_ExecResult;

typedef enum CPU_MemOpKind {
	CPU_MEM_READ = 0,
	CPU_MEM_WRITE,
} CPU_MemOpKind;

typedef struct CPU_RamByte {
	u32 addr;
	u8 byte;
} CPU_RamByte;

typedef struct CPU_State {
	u32 regs[CPU_REG_COUNT];
	CPU_RamByte ram[CPU_MAX_RAM_DIFF];
	u32 ram_len;
} CPU_State;

typedef struct CPU_MemOp {
	CPU_MemOpKind kind;
	u32 addr; // Access address
	u16 data; // Data accessed
	u8 fc;	  // Function code
	bool is_word;
} CPU_MemOp;

typedef struct CPU_TestCase {
	char name[CPU_MAX_NAME_LEN];
	CPU_Model model;
	u64 seed;

	u16 op_words[CPU_MAX_OP_WORDS];
	u8 op_word_count;

	CPU_State pre;
	CPU_State post;

	CPU_MemOp mem_ops[CPU_MAX_MEM_OPS];
	u32 mem_op_len;

	u32 cycles;
	CPU_ExecResult exec_result;
	u16 exception_vector; // 0 if none/unknown

	bool mem_ops_overflow;
	bool ram_diff_overflow;
	bool touched_list_overflow;
} CPU_TestCase;

bool cpu_init();
void cpu_deinit();

void cpu_begin_test_case(const char *name, CPU_Model model, u64 seed);
void cpu_query_test_case(CPU_TestCase *test);

void cpu_capture_pre();
void cpu_capture_post();

void cpu_clear_ram();
void cpu_reset();
void cpu_set_op_words(const u16 *words, u8 count);
u32 cpu_execute(u32 cycles); // Return used cycles

u32 cpu_get_reg(u8 reg);
void cpu_set_reg(u8 reg, u32 data);

u8 cpu_read_byte(u32 addr);
u16 cpu_read_word(u32 addr);
void cpu_write_byte(u32 addr, u8 byte);
void cpu_write_word(u32 addr, u16 word);

// Musashi callbacks
void cpu_set_fc(u8 fc);
i32 cpu_tas();
