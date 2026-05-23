#ifndef _TEST68K_MOIRA_WRAPPER_H_
#define _TEST68K_MOIRA_WRAPPER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef struct cpu_t cpu_t;

typedef u8 (*cpu_read8_t)(void *user, u32 addr);
typedef u16 (*cpu_read16_t)(void *user, u32 addr);
typedef void (*cpu_write8_t)(void *user, u32 addr, u8 byte);
typedef void (*cpu_write16_t)(void *user, u32 addr, u16 word);

enum cpu_model_t : i32 {
	CPU_MODEL_M68000 = 0,
	CPU_MODEL_M68010,
};

enum cpu_registers_t {
	CPU_REG_D0 = 0,
	CPU_REG_D1,
	CPU_REG_D2,
	CPU_REG_D3,
	CPU_REG_D4,
	CPU_REG_D5,
	CPU_REG_D6,
	CPU_REG_D7,
	CPU_REG_A0,
	CPU_REG_A1,
	CPU_REG_A2,
	CPU_REG_A3,
	CPU_REG_A4,
	CPU_REG_A5,
	CPU_REG_A6,
	CPU_REG_A7,

	CPU_REG_PC,
	CPU_REG_SR,
	CPU_REG_SP,
	CPU_REG_USP,
	CPU_REG_ISP,
	CPU_REG_VBR,
	CPU_REG_SFC,
	CPU_REG_DFC,
	CPU_REG_COUNT,
};

enum cpu_exception_kind_t : i32 {
	CPU_EXC_OK = 0,
	CPU_EXC_BUS_ERR,
	CPU_EXC_ADDR_ERR,
	CPU_EXC_ILLEGAL,
	CPU_EXC_DIVBYZERO,
	CPU_EXC_CHK,
	CPU_EXC_TRAPV,
	CPU_EXC_PRIVILEGE,
	CPU_EXC_TRACE,
	CPU_EXC_LINEA,
	CPU_EXC_LINEF,
	CPU_EXC_FORMAT_ERR,
	CPU_EXC_IRQ_UNINIT,
	CPU_EXC_IRQ_SPURIOUS,
	CPU_EXC_TRAP,
	CPU_EXC_BKPT,
};

cpu_t *cpu_create(cpu_read8_t r8, cpu_read16_t r16, cpu_write8_t w8, cpu_write16_t w16);
void cpu_destroy(cpu_t *cpu);

void cpu_set_userdata(cpu_t *cpu, void *userdata);

void cpu_reset(cpu_t *cpu, enum cpu_model_t cpu_model);
void cpu_execute(cpu_t *cpu, i64 cycles);

bool cpu_is_instruction_valid(const cpu_t *cpu, u16 opcode, u16 ext);

i32 cpu_disassemble(const cpu_t *cpu, char *str, u32 addr);

void cpu_get_registers(const cpu_t *cpu, u32 *out);
void cpu_set_registers(cpu_t *cpu, u32 *regs);

i64 cpu_get_clock(const cpu_t *cpu);
void cpu_set_clock(cpu_t *cpu, i64 cycles);

u32 cpu_get_pc0(const cpu_t *cpu);
u8 cpu_read_fc(const cpu_t *cpu);
u8 cpu_get_ipl(const cpu_t *cpu);

u16 cpu_get_exception_vector(const cpu_t *cpu);
u16 cpu_get_exception_kind(const cpu_t *cpu);

#ifdef __cplusplus
}
#endif

#endif /* _TEST68K_MOIRA_WRAPPER_H_ */
