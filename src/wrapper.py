import logging
import os, ctypes
from ctypes import POINTER, byref, c_bool, c_size_t, c_uint8, c_uint16, c_uint32


_lib = ctypes.CDLL(os.environ["T68K_WRAPPER_LIB"])

# bool cpu_init(void)
_lib.cpu_init.argtypes = []
_lib.cpu_init.restype = c_bool

# void cpu_deinit(void)
_lib.cpu_deinit.argtypes = []
_lib.cpu_deinit.restype = None

# bool cpu_load_rom(usize, u8[])
_lib.cpu_load_rom.argtypes = [c_size_t, POINTER(c_uint8)]
_lib.cpu_load_rom.restype = c_bool

# bool cpu_read_byte(u32, u8*)
_lib.cpu_read_byte.argtypes = [c_uint32, POINTER(c_uint8)]
_lib.cpu_read_byte.restype = c_bool

# bool cpu_read_word(u32, u16*)
_lib.cpu_read_word.argtypes = [c_uint32, POINTER(c_uint16)]
_lib.cpu_read_word.restype = c_bool

# bool cpu_write_byte(u32, u8)
_lib.cpu_write_byte.argtypes = [c_uint32, c_uint8]
_lib.cpu_write_byte.restype = c_bool

# bool cpu_write_word(u32, u16)
_lib.cpu_write_word.argtypes = [c_uint32, c_uint16]
_lib.cpu_write_word.restype = c_bool

class CPU:
	def __init__(self):
		res = _lib.cpu_init()
		assert(res )


	def __del__(self):
		_lib.cpu_deinit()


	def load_rom(self, data: bytes | bytearray | memoryview) -> bool:
		raw = bytes(data)
		if len(raw) == 0:
			return False

		buf = (c_uint8 * len(raw)).from_buffer_copy(raw)
		return bool(_lib.cpu_load_rom(len(raw), buf))


	def read_byte(self, addr: int) -> int | None:
		byte = c_uint8()
		res = _lib.cpu_read_byte(addr, byref(byte))
		if not res:
			return None

		return byte.value


	def read_word(self, addr: int) -> int | None:
		word = c_uint16()
		res = _lib.cpu_read_word(addr, byref(word))
		if not res:
			logging.error("Failed to read word!")
			return None

		return word.value


	def write_byte(self, addr: int, byte: int) -> bool:
		return _lib.cpu_write_byte(addr, c_uint8(byte))


	def write_word(self, addr: int, word: int) -> bool:
		res = _lib.cpu_write_word(addr, c_uint16(word))
		if not res:
			logging.error("Failed to write word!")
		return bool(res)
