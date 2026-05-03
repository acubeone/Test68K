import argparse
from pathlib import Path
from typing import Any
import wrapper

CYCLES_BUDGET: int = 200
ROM_ADDR: tuple[int, int] = (0x00_1000, 0x00_FFFF)
RAM_ADDR: tuple[int, int] = (0x01_0000, 0x03_FFFF)

VECTORS: list[int] = [
	0x0002_FFF0,  # Initial SSP
	0x0000_1000,  # Initial PC
	*[0x00_0800 + (vec * 0x10) for vec in range(2, 256)],
]


def _parse_args() -> argparse.Namespace:
	parser = argparse.ArgumentParser(description="Generate M68K instruction tests")
	_ = parser.add_argument(
		"-o",
		"--out",
		type=Path,
		required=True,
		help="Output directory for test files",
	)
	_ = parser.add_argument(
		"--max-tests",
		type=int,
		default=256,
		help="How many generated tests for each instruction",
	)

	return parser.parse_args()


def _run_test_case(
	cpu: wrapper.CPU,
	id: int,
	regs: list[int],
	op_words: list[int],
	ram: list[int],
	model: int,
	seed: int,
) -> dict[str, Any]:
	cpu.clear_ram()
	cpu.begin_test_case(model, seed)
	cpu.set_op_words(op_words)

	for vec, long in enumerate(VECTORS):
		base = vec * 4
		cpu.write_word(base + 0, (long >> 16) & 0xFFFF)
		cpu.write_word(base + 2, (long) & 0xFFFF)

	for i, word in enumerate(op_words):
		cpu.write_word(ROM_ADDR[0] + (i * 2), word)

	for i, byte in enumerate(ram):
		cpu.write_byte(RAM_ADDR[0] + i, byte & 0xFF)

	cpu.reset()
	for i, reg in enumerate(regs):
		cpu.set_reg(wrapper.REG_NAMES[i], reg)

	disasm = cpu.disasm_at(ROM_ADDR[0])
	cpu.set_test_case_name(f"{id} {disasm} {hex(op_words[0])}")

	cpu.capture_pre()
	cpu.execute(CYCLES_BUDGET)
	cpu.capture_post()

	return cpu.query_test_case()


def main() -> None:
	args = _parse_args()
	if not args.out.is_dir():
		args.out.mkdir(parents=True, exist_ok=True)
	outpath = args.out
	max_tests = args.max_tests

	cpu = wrapper.CPU()
