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

typedef u8 (*cpu_read8)(u32);
typedef u16 (*cpu_read16)(u32);

typedef void (*cpu_write8)(u32, u8);
typedef void (*cpu_write16)(u32, u16);

void cpu_init();
void cpu_deinit();

void cpu_set_read8_callback(cpu_read8 func);
void cpu_set_read16_callback(cpu_read16 func);

void cpu_set_write8_callback(cpu_write8 func);
void cpu_set_write16_callback(cpu_write16 func);

#ifdef __cplusplus
}
#endif

#endif /* _TEST68K_MOIRA_WRAPPER_H_ */
