import os
import ctypes as C

_lib = C.CDLL(os.environ["T68K_WRAPPER_LIB"])

MAX_RAM = 1 << 22
MAX_NAME_LEN = 128
MAX_RAM_DIFF = 4096
MAX_MEM_OPS = 4096
MAX_OP_WORDS = 16

REG_COUNT = 19

# CPU_Model
MODEL_M68000 = 0
MODEL_M68010 = 1

# CPU_ExecResult
CPU_EXEC_OK = 0
CPU_EXEC_ILLEGAL = 1  # Exception: Illegal
CPU_EXEC_LINEA = 2  # Exception: Line-A
CPU_EXEC_LINEF = 3  # Exception: Line-F
CPU_EXEC_ADDR = 4  # Exception: Address Error
CPU_EXEC_TRAP = 5  # Exception: TRAP #N
CPU_EXEC_TAS = 6  # Exception: TAS
CPU_EXEC_OTHER = 7  # Other exception

# CPU_MemOpKind
CPU_MEM_READ = 0
CPU_MEM_WRITE = 1


class RamByte(C.Structure):
    _fields_ = [
        ("addr", C.c_uint32),
        ("byte", C.c_uint8),
    ]


class State(C.Structure):
    _fields_ = [
        ("regs", C.c_uint32 * REG_COUNT),
        ("ram", RamByte * MAX_RAM_DIFF),
        ("ram_len", C.c_uint32),
    ]


class MemOp(C.Structure):
    _fields_ = [
        ("kind", C.c_uint32),  # CPU_MemOpKind
        ("addr", C.c_uint32),
        ("data", C.c_uint16),
        ("fc", C.c_uint8),
        ("is_word", C.c_bool),
    ]


class TestCase(C.Structure):
    _fields_ = [
        ("name", C.c_char * MAX_NAME_LEN),
        ("model", C.c_uint32),  # CPU_Model
        ("seed", C.c_uint64),
        ("op_words", C.c_uint16 * MAX_OP_WORDS),
        ("op_word_count", C.c_uint8),
        ("pre", State),
        ("post", State),
        ("mem_ops", MemOp * MAX_MEM_OPS),
        ("mem_op_len", C.c_uint32),
        ("cycles", C.c_uint32),
        ("exec_result", C.c_uint32),  # CPU_ExecResult
        ("exception_vector", C.c_uint16),
        ("mem_ops_overflow", C.c_bool),
        ("ram_diff_overflow", C.c_bool),
        ("touched_list_overflow", C.c_bool),
    ]


# bool cpu_init(void)
_lib.cpu_init.argtypes = []
_lib.cpu_init.restype = C.c_bool

# void cpu_deinit(void)
_lib.cpu_deinit.argtypes = []
_lib.cpu_deinit.restype = None

# void cpu_begin_test_case(const char*, CPU_Model, u64)
_lib.cpu_begin_test_case.argtypes = [C.c_char_p, C.c_uint32, C.c_uint64]
_lib.cpu_begin_test_case.restype = None

# void cpu_query_test_case(CPU_TestCase*)
_lib.cpu_query_test_case.argtypes = [C.POINTER(TestCase)]
_lib.cpu_query_test_case.restype = None

# void cpu_capture_pre()
_lib.cpu_capture_pre.argtypes = []
_lib.cpu_capture_pre.restype = None

# void cpu_capture_post()
_lib.cpu_capture_post.argtypes = []
_lib.cpu_capture_post.restype = None

# void cpu_clear_ram()
_lib.cpu_clear_ram.argtypes = []
_lib.cpu_clear_ram.restype = None

# void cpu_reset()
_lib.cpu_reset.argtypes = []
_lib.cpu_reset.restype = None

# void cpu_set_op_words(const u16*, u8)
_lib.cpu_set_op_words.argtypes = [C.POINTER(C.c_uint16), C.c_uint8]
_lib.cpu_set_op_words.restype = None

# u32 cpu_execute(u32);
_lib.cpu_execute.argtypes = [C.c_uint32]
_lib.cpu_execute.restype = C.c_uint32

# u32 cpu_get_reg(u8)
_lib.cpu_get_reg.argtypes = [C.c_uint8]
_lib.cpu_get_reg.restype = C.c_uint32

# void cpu_set_reg(u8, u32)
_lib.cpu_set_reg.argtypes = [C.c_uint8, C.c_uint32]
_lib.cpu_set_reg.restype = None

# u8 cpu_read_byte(u32)
_lib.cpu_read_byte.argtypes = [C.c_uint32]
_lib.cpu_read_byte.restype = C.c_uint8

# u16 cpu_read_word(u32)
_lib.cpu_read_word.argtypes = [C.c_uint32]
_lib.cpu_read_word.restype = C.c_uint16

# void cpu_write_byte(u32, u8)
_lib.cpu_write_byte.argtypes = [C.c_uint32, C.c_uint8]
_lib.cpu_write_byte.restype = None

# void cpu_write_word(u32, u16)
_lib.cpu_write_word.argtypes = [C.c_uint32, C.c_uint16]
_lib.cpu_write_word.restype = None

_reg_names = [
    "d0",
    "d1",
    "d2",
    "d3",
    "d4",
    "d5",
    "d6",
    "d7",
    "a0",
    "a1",
    "a2",
    "a3",
    "a4",
    "a5",
    "a6",
    "a7",
    "pc",
    "sr",
    "sp",
    "usp",
    "isp",
    "msp",
    "sfc",
    "dfc",
    "vbr",
]


class CPU:
    def __init__(self):
        res = _lib.cpu_init()
        assert res

    def __del__(self):
        _lib.cpu_deinit()

    def clear_ram(self) -> None:
        _lib.cpu_clear_ram()

    def reset(self) -> None:
        _lib.cpu_reset()

    def begin_test_case(self, name: str, model: int, seed: int) -> None:
        _lib.cpu_begin_test_case(name.encode(), C.c_uint32(model), C.c_uint64(seed))

    def query_test_case(self) -> TestCase:
        test = TestCase()
        _lib.cpu_query_test_case(C.byref(test))
        return test

    def capture_pre(self) -> None:
        _lib.cpu_capture_pre()

    def capture_post(self) -> None:
        _lib.cpu_capture_post()

    def set_op_words(self, words: list[int]) -> None:
        count = min(len(words), MAX_OP_WORDS)
        arr = (C.c_uint16 * count)(*[(word & 0xFFFF) for word in words[:count]])
        _lib.cpu_set_op_words(arr, count)

    def execute(self, cycles: int) -> int:
        return _lib.cpu_execute(cycles)

    def get_reg(self, reg: str) -> int:
        return _lib.cpu_get_reg(_reg_names.index(reg))

    def set_reg(self, reg: str, data: int) -> None:
        _lib.cpu_set_reg(_reg_names.index(reg), C.c_uint32(data))

    def read_byte(self, addr: int) -> int:
        return _lib.cpu_read_byte(addr)

    def read_word(self, addr: int) -> int:
        return _lib.cpu_read_word(addr)

    def write_byte(self, addr: int, byte: int) -> None:
        _lib.cpu_write_byte(addr, C.c_uint8(byte))

    def write_word(self, addr: int, word: int) -> None:
        _lib.cpu_write_word(addr, C.c_uint16(word))
