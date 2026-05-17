// clang-format Language: C
#ifndef _TEST68K_MOIRA_WRAPPER_H_
#define _TEST68K_MOIRA_WRAPPER_H_

#ifdef __cplusplus
extern "C" {
#endif

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

cpu_t *cpu_create(cpu_read8_t r8, cpu_read16_t r16, cpu_write8_t w8, cpu_write16_t w16);
void cpu_destroy(cpu_t *cpu);

void cpu_set_userdata(cpu_t *cpu, void *userdata);

#ifdef __cplusplus
}
#endif

#endif /* _TEST68K_MOIRA_WRAPPER_H_ */
