import struct
from typing import Any

from wrapper import REG_COUNT

HEADER_MAGIC = b"T68K"
HEADER_VERSION = 1
UNKNOWN_ENTRY_COUNT = 0xFFFF_FFFF

ENTRY_MAGIC = b"TCS1"


def _to_u16(value: int) -> int:
	return value & 0xFFFF


def _to_u32(value: int) -> int:
	return value & 0xFFFF_FFFF


def _to_u64(value: int) -> int:
	return value & 0xFFFF_FFFF_FFFF_FFFF


def _align_to_4bytes(data: bytearray) -> None:
	pad = (4 - (len(data) % 4)) % 4
	if pad > 0:
		data.extend(b"\x00" * pad)


# header: MAGIC:u8[4], VERSION:u16, PADDING:u8[2], ENTRY_COUNT:u32, RESERVED:u8[4]
def serialize_header(entry_count: int = UNKNOWN_ENTRY_COUNT) -> bytes:
	return struct.pack("<4sH2xI4x", HEADER_MAGIC, HEADER_VERSION, _to_u32(entry_count))


def serialize_entry(test: dict[str, Any]) -> bytes:
	# blob
	name = str(test.get("name", "")).encode("ascii")
	model = _to_u32(test.get("model", 0))
	seed = _to_u64(test.get("seed", 0))

	op_words = [int(word) & 0xFFFF for word in test.get("op_words", [])]

	pre = test.get("pre", {})
	pre_regs = [_to_u32(reg) for reg in pre.get("regs", [])][:REG_COUNT]
	pre_regs.extend([0] * (REG_COUNT - len(pre_regs)))  # Zero regs not present
	pre_ram = pre.get("ram", [])

	post = test.get("post", {})
	post_regs = [_to_u32(reg) for reg in post.get("regs", [])][:REG_COUNT]
	post_regs.extend([0] * (REG_COUNT - len(post_regs)))  # Zero regs not present
	post_ram = post.get("ram", [])

	mem_ops = test.get("mem_ops", [])

	ov = test.get("overflow", {})
	flags = (
		((1 if ov.get("mem_ops", False) else 0) << 0)
		| ((1 if ov.get("ram_diff", False) else 0) << 1)
		| ((1 if ov.get("touched_list", False) else 0) << 2)
	)

	blob = bytearray()

	# seed:u64, model:u32, name_len:u32, name:u8[N], padding:u8[]
	blob.extend(struct.pack("<QII", seed, model, len(name)))
	blob.extend(name)
	_align_to_4bytes(blob)

	# op_word_count:u32, op_words:u16[N], padding:u8[]
	blob.extend(struct.pack("<I", len(op_words)))
	if op_words:
		blob.extend(struct.pack(f"<{len(op_words)}H", *op_words))
	_align_to_4bytes(blob)

	# pre_regs:u32[19], pre_ram_len:u32, pre_ram:ram[N]
	blob.extend(struct.pack(f"<{REG_COUNT}I", *pre_regs))
	blob.extend(struct.pack("<I", len(pre_ram)))
	for ram_byte in pre_ram:
		blob.extend(
			struct.pack(
				"<IB3x",
				_to_u32(ram_byte.get("addr", 0)),
				int(ram_byte.get("byte", 0)) & 0xFF,
			)
		)

	# post_regs:u32[19], post_ram_len:u32, post_ram:ram[N]
	blob.extend(struct.pack(f"<{REG_COUNT}I", *post_regs))
	blob.extend(struct.pack("<I", len(post_ram)))
	for ram_byte in post_ram:
		blob.extend(
			struct.pack(
				"<IB3x",
				_to_u32(ram_byte.get("addr", 0)),
				int(ram_byte.get("byte", 0)) & 0xFF,
			)
		)

	# mem_ops:
	#   addr:u32, data:u32, kind:u8, size:u8, fc:u8, padding:u8
	blob.extend(struct.pack("<I", len(mem_ops)))
	for op in mem_ops:
		addr = _to_u32(op.get("addr", 0))
		data = _to_u32(op.get("data", 0))
		kind = 1 if int(op.get("kind", 0)) != 0 else 0  # 0=read, 1=write
		size = 1 if bool(op.get("is_word", False)) else 0  # 0=byte, 1=word
		fc = int(op.get("fc", 0)) & 0xFF
		blob.extend(struct.pack("<IIBBBx", addr, data, kind, size, fc))

	# cycles:u32, exception_vector:u16, exec_result:u16, flags:u32
	blob.extend(
		struct.pack(
			"<IHHI",
			_to_u32(test.get("cycles", 0)),
			_to_u16(test.get("exception_vector", 0)),
			_to_u16(test.get("exec_result", 0)),
			_to_u32(flags),
		)
	)

	# MAGIC:u8[4], ENTRY_LEN:u32, blob
	return struct.pack("<4sI", ENTRY_MAGIC, len(blob)) + blob


def serialize(tests: list[dict[str, Any]]) -> bytes:
	data = bytearray()
	for test in tests:
		data.extend(serialize_entry(test))

	return serialize_header(len(tests)) + data
