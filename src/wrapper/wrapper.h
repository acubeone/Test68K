#pragma once

#include "types.h"

bool cpu_init();
void cpu_deinit();

bool cpu_load_rom(usize size, u8 bytes[static size]);

bool cpu_read_byte(u32 addr, u8 *byte);
bool cpu_read_word(u32 addr, u16 *word);

bool cpu_write_byte(u32 addr, u8 byte);
bool cpu_write_word(u32 addr, u16 word);
