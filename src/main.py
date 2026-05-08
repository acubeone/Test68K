import argparse
from pathlib import Path
import pprint
import random
import sys
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


INSTRS = [
	[0x4E71],  # NOP
	[0x4AFC],  # ILLEGAL
	[0xA000],  # LINE-A
	[0xF000],  # LINE-F
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
	_ = parser.add_argument(
		"-s",
		"--seed",
		type=int,
		default=0,
		help="Default random seed",
	)
	_ = parser.add_argument(
		"-m",
		"--model",
		type=str,
		default="m68000",
		choices=["m68000", "m68010"],
		help="Select which CPU to emulate: m68000 | m68010",
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
	cpu.set_test_case_name(f"{id} {disasm} {op_words[0]:x}")

	cpu.capture_pre()
	cpu.execute(CYCLES_BUDGET)
	cpu.capture_post()

	return cpu.query_test_case()


def _rand_regs(rng: random.Random) -> list[int]:
	regs = [rng.getrandbits(32) for _ in range(wrapper.REG_COUNT)]
	regs[wrapper.REG_NAMES.index("pc")] = ROM_ADDR[0]
	regs[wrapper.REG_NAMES.index("sp")] = VECTORS[0]
	regs[wrapper.REG_NAMES.index("a7")] = VECTORS[0]
	regs[wrapper.REG_NAMES.index("sr")] = rng.getrandbits(16)
	return regs


def _rand_ram(rng: random.Random, n: int = 256) -> list[int]:
	return [rng.getrandbits(8) for _ in range(n)]


def _generate_batch(outdir: Path, max_tests: int, seed: int) -> None:
	cpu = wrapper.CPU()
	seed = seed or random.randint(0, sys.maxsize)

	for op_words in INSTRS:
		for i in range(max_tests):
			rng = random.Random(seed)

			regs = _rand_regs(rng)
			ram = _rand_ram(rng)

			case = _run_test_case(
				cpu, i, regs, op_words, ram, wrapper.MODEL_M68000, seed
			)
			pprint.pp(case)


def main() -> None:
	args = _parse_args()
	if not args.out.is_dir():
		args.out.mkdir(parents=True, exist_ok=True)

	_generate_batch(args.out, args.max_tests, args.seed)


if __name__ == "__main__":
	main()
