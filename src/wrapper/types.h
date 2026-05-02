#pragma once

#include <stddef.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef size_t usize;

enum {
	T68K_MAX_RAM = 1 << 22, // Max flat RAM: 4MB
};
