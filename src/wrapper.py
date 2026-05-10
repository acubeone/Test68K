import os
import ctypes as C
from typing import Any

_lib = C.CDLL(os.environ["T68K_WRAPPER_LIB"])

MAX_RAM = 1 << 22
MAX_NAME_LEN = 128
MAX_RAM_DIFF = 4096
MAX_MEM_OPS = 4096
MAX_OP_WORDS = 16

REG_COUNT = 25
VEC_COUNT = 256

# CPU_Model
MODEL_M68000 = 0
MODEL_M68010 = 1

# CPU_ExecResult
EXEC_OK = 0
EXEC_ILLEGAL = 1  # Exception: Illegal
EXEC_ADDR = 2  # Exception: Address Error
EXEC_TRAP = 3  # Exception: TRAP #N
EXEC_TAS = 4  # Exception: TAS
EXEC_OTHER = 5  # Other exception

# CPU_MemOpKind
MEM_READ = 0
MEM_WRITE = 1


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

	def to_list(self) -> dict[str, Any]:
		pre_ram = [
			{
				"addr": int(self.pre.ram[i].addr),
				"byte": int(self.pre.ram[i].byte),
			}
			for i in range(int(self.pre.ram_len))
		]
		post_ram = [
			{
				"addr": int(self.post.ram[i].addr),
				"byte": int(self.post.ram[i].byte),
			}
			for i in range(int(self.post.ram_len))
		]

		mem_ops = [
			{
				"kind": int(self.mem_ops[i].kind),
				"addr": int(self.mem_ops[i].addr),
				"data": int(self.mem_ops[i].data),
				"fc": int(self.mem_ops[i].fc),
				"is_word": bool(self.mem_ops[i].is_word),
			}
			for i in range(self.mem_op_len)
		]

		return {
			"name": _decode_ctr(self.name),
			"model": int(self.model),
			"seed": int(self.seed),
			"op_words": [int(self.op_words[i]) for i in range(int(self.op_word_count))],
			"pre": {
				"regs": [int(self.pre.regs[i]) for i in range(REG_COUNT)],
				"ram": pre_ram,
			},
			"post": {
				"regs": [int(self.post.regs[i]) for i in range(REG_COUNT)],
				"ram": post_ram,
			},
			"mem_ops": mem_ops,
			"cycles": int(self.cycles),
			"exec_result": int(self.exec_result),
			"exception_vector": int(self.exception_vector),
			"overflow": {
				"mem_ops": bool(self.mem_ops_overflow),
				"ram_diff": bool(self.ram_diff_overflow),
				"touched_list": bool(self.touched_list_overflow),
			},
		}


class BatchRequest(C.Structure):
	_fields_ = [
		("count", C.c_uint32),
		("seed", C.c_uint64),
		("model", C.c_uint32),
		("regs", C.c_uint32 * REG_COUNT),
		("cycles_budget", C.c_uint32),
		("step_budget", C.c_uint32),
		("vectors", C.c_uint32 * VEC_COUNT),
		("op_words", C.c_uint16 * MAX_OP_WORDS),
		("ram", C.POINTER(C.c_uint8)),
		("ram_size", C.c_uint32),
	]


# bool cpu_init(void)
_lib.cpu_init.argtypes = []
_lib.cpu_init.restype = C.c_bool

# void cpu_deinit(void)
_lib.cpu_deinit.argtypes = []
_lib.cpu_deinit.restype = None

# void cpu_request_batch(const CPU_BatchRequest*, CPU_TestCase *[])
_lib.cpu_request_batch.argtypes = [
	C.POINTER(BatchRequest),
	C.POINTER(C.POINTER(TestCase)),
]
_lib.cpu_request_batch.restype = None

# void cpu_begin_test_case(CPU_Model, u64)
_lib.cpu_begin_test_case.argtypes = [C.c_uint32, C.c_uint64]
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

# void cpu_disasm_at(u32, char*, u32)
_lib.cpu_disasm_at.argtypes = [C.c_uint32, C.c_char_p, C.c_uint32]
_lib.cpu_disasm_at.restype = None

# void cpu_is_instruction_valid(u16)
_lib.cpu_is_instruction_valid.argtypes = [C.c_uint16, C.c_uint32]
_lib.cpu_is_instruction_valid.restype = C.c_bool

# void cpu_set_op_words(const u16*, u8)
_lib.cpu_set_op_words.argtypes = [C.POINTER(C.c_uint16), C.c_uint8]
_lib.cpu_set_op_words.restype = None

# void cpu_set_test_case_name(const char*)
_lib.cpu_set_test_case_name.argtypes = [C.c_char_p]
_lib.cpu_set_test_case_name.restype = None

# void cpu_clear_ram()
_lib.cpu_clear_ram.argtypes = []
_lib.cpu_clear_ram.restype = None

# void cpu_reset()
_lib.cpu_reset.argtypes = []
_lib.cpu_reset.restype = None

# u32 cpu_execute(u32, u32)
_lib.cpu_execute.argtypes = [C.c_uint32, C.c_uint32]
_lib.cpu_execute.restype = C.c_uint32

# u32 cpu_get_reg(u8)
_lib.cpu_get_reg.argtypes = [C.c_uint8]
_lib.cpu_get_reg.restype = C.c_uint32

# void cpu_set_reg(u8, u32)
_lib.cpu_set_reg.argtypes = [C.c_uint8, C.c_uint32]
_lib.cpu_set_reg.restype = None

# void cpu_set_regs(u32*)
_lib.cpu_set_regs.argtypes = [C.POINTER(C.c_uint32)]
_lib.cpu_set_regs.restype = None

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

# void cpu_write_block(u32, u8*, u32)
_lib.cpu_write_block.argtypes = [C.c_uint32, C.POINTER(C.c_uint8), C.c_uint32]
_lib.cpu_write_block.restype = None

REG_NAMES = [
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

_test_buf = TestCase()


def _decode_ctr(buf: bytes) -> str:
	return buf.split(b"\x00", 1)[0].decode("utf-8", errors="replace")


class CPU:
	def __init__(self):
		res = bool(_lib.cpu_init())
		assert res

	def __del__(self):
		_lib.cpu_deinit()

	def request_batch(self, request: dict[str, Any]) -> list[dict[str, Any]]:
		request_count = request.get("count", 0)

		req = BatchRequest()
		req.count = C.c_uint32(request_count)
		req.seed = C.c_uint64(request.get("seed", 0))
		req.model = C.c_uint32(request.get("model", MODEL_M68000))
		req.cycles_budget = C.c_uint32(request.get("cycles_budget", 0))
		req.step_budget = C.c_uint32(request.get("step_budget", 4))

		req.regs = (C.c_uint32 * REG_COUNT)()
		for i, reg in enumerate(request.get("regs", [])[:REG_COUNT]):
			req.regs[i] = reg

		req.vectors = (C.c_uint32 * VEC_COUNT)()
		for i, vec in enumerate(request.get("vectors", [])[:VEC_COUNT]):
			req.vectors[i] = vec

		req.op_words = (C.c_uint16 * MAX_OP_WORDS)()
		for i, words in enumerate(request.get("op_words", [])[:MAX_OP_WORDS]):
			req.op_words[i] = words

		ram = request.get("ram", [])
		ram_arr = (C.c_uint8 * len(ram))(*ram)

		req.ram_size = C.c_uint32(len(ram))
		req.ram = C.cast(ram_arr, C.POINTER(C.c_uint8))

		cases = (TestCase * request_count)()
		case_ptrs = (C.POINTER(TestCase) * request_count)(
			*[C.pointer(cases[i]) for i in range(request_count)]
		)

		_lib.cpu_request_batch(C.byref(req), case_ptrs)
		return [cases[i].to_list() for i in range(request_count)]

	def begin_test_case(self, model: int, seed: int) -> None:
		_lib.cpu_begin_test_case(C.c_uint32(model), C.c_uint64(seed))

	def query_test_case(self) -> dict[str, Any]:
		_lib.cpu_query_test_case(C.byref(_test_buf))
		return _test_buf.to_list()

	def capture_pre(self) -> None:
		_lib.cpu_capture_pre()

	def capture_post(self) -> None:
		_lib.cpu_capture_post()

	def clear_ram(self) -> None:
		_lib.cpu_clear_ram()

	def reset(self) -> None:
		_lib.cpu_reset()

	def disasm_at(self, pc: int) -> str:
		buf = C.create_string_buffer(MAX_NAME_LEN)
		_lib.cpu_disasm_at(C.c_uint32(pc), buf, C.c_uint32(len(buf)))
		return buf.value.decode("utf-8", errors="replace")

	def is_instruction_valid(self, opcode: int, model: int) -> bool:
		res = _lib.cpu_is_instruction_valid(C.c_uint16(opcode), C.c_uint32(model))
		return bool(res)

	def set_op_words(self, words: list[int]) -> None:
		count = min(len(words), MAX_OP_WORDS)
		arr = (C.c_uint16 * count)(*[(word & 0xFFFF) for word in words[:count]])
		_lib.cpu_set_op_words(arr, count)

	def set_test_case_name(self, name: str) -> None:
		_lib.cpu_set_test_case_name(name.encode())

	def execute(self, total_budget: int, step_budget: int = 4) -> int:
		return int(_lib.cpu_execute(total_budget, step_budget))

	def get_reg(self, reg: str) -> int:
		return int(_lib.cpu_get_reg(REG_NAMES.index(reg)))

	def set_reg(self, reg: str, data: int) -> None:
		_lib.cpu_set_reg(REG_NAMES.index(reg), C.c_uint32(data))

	def set_regs(self, regs: list[int]) -> None:
		arr = (C.c_uint32 * len(regs))(*regs)
		_lib.cpu_set_regs(arr)

	def read_byte(self, addr: int) -> int:
		return int(_lib.cpu_read_byte(addr))

	def read_word(self, addr: int) -> int:
		return int(_lib.cpu_read_word(addr))

	def write_byte(self, addr: int, byte: int) -> None:
		_lib.cpu_write_byte(addr, C.c_uint8(byte))

	def write_word(self, addr: int, word: int) -> None:
		_lib.cpu_write_word(addr, C.c_uint16(word))

	def write_block(self, addr: int, data: list[int]) -> None:
		arr = (C.c_uint8 * len(data))(*data)
		_lib.cpu_write_block(C.c_uint32(addr), arr, C.c_uint32(len(data)))
