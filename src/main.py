import argparse
from pathlib import Path
import random
import re
import sys
import time
from typing import Any
import tcs
import wrapper

CYCLES_BUDGET: int = 200
ROM_ADDR: tuple[int, int] = (0x00_1000, 0x00_FFFF)
RAM_ADDR: tuple[int, int] = (0x01_0000, 0x03_FFFF)

VECTORS: list[int] = [
	0x0002_FFF0,  # Initial SSP
	0x0000_1000,  # Initial PC
	*[0x00_0800 + (vec * 0x10) for vec in range(2, 256)],
]

_RE_NAME = re.compile(
	r"""
	^\s*\d+\s+
	(?P<mnem>[A-Za-z][A-Za-z0-9_.]*)
	(?:\s+.*)?$
	""",
	re.VERBOSE,
)

# fmt: off
_VALID_MNEMONICS = {
	"abcd", "add", "adda", "addi", "addq", "addx",
	"and", "andi",
	"asl", "asr",
	"bcc", "bcs", "beq", "bge", "bgt", "bhi", "ble", "bls",
	"blt", "bmi", "bne", "bpl", "bra", "bsr", "bvc", "bvs",
	"bchg", "bclr", "bset", "btst",
	"bkpt", "chk", "clr",
	"cmp", "cmpa", "cmpi", "cmpm",
	"dbcc", "dbcs", "dbeq", "dbf",  "dbge", "dbgt", "dbhi", "dble", "dbls",
	"dblt", "dbmi", "dbne", "dbpl", "dbra", "dbt",  "dbvc", "dbvs",
	"divs", "divu",
	"eor", "eori",
	"exg", "ext", "illegal", "jmp", "jsr",
	"lea", "link",
	"lsl", "lsr",
	"move", "movea", "movem", "movep", "moveq", "moves", "movec",
	"muls", "mulu",
	"nbcd", "neg", "negx",
	"nop",
	"not", "or", "ori",
	"pea", "reset",
	"rol", "ror", "roxl", "roxr",
	"rte", "rtr", "rts", "rtd",
	"sbcd",
	"scc", "scs", "seq", "sf",  "sge", "sgt", "shi", "sle",
	"sls", "slt", "smi", "sne", "spl", "st",  "svc", "svs",
	"sub", "suba", "subi", "subq", "subx",
	"swap", "tas", "trap", "trapv", "tst", "unlk",
}
# fmt:on


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
		default=16,
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

	cpu.write_block(ROM_ADDR[0], op_words)
	cpu.write_block(RAM_ADDR[0], ram)

	cpu.reset()
	cpu.set_regs(regs)

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


def _mnemonic_base(name: str) -> str:
	m = _RE_NAME.match(name)
	if not m:
		return ""

	token = m.group("mnem").lower()
	base = token.split(".", 1)[0]

	if base == "dbra":
		base = "dbf"
	return base if base in _VALID_MNEMONICS else ""


def _get_valid_opcodes(cpu: wrapper.CPU, model: int) -> list[int]:
	opcodes: list[int] = []

	for opcode in range(0, 0x1_0000):
		opclass = opcode & 0xF000
		if opclass == 0xF000 or opclass == 0xA000:
			continue

		if opcode != 0x4AFC and not cpu.is_instruction_valid(opcode, model):
			continue

		opcodes.append(opcode)

	return opcodes


def _generate_batch(
	cpu: wrapper.CPU, opcodes: list[int], max_tests: int, seed: int, model: int
) -> dict[str, list[Any]]:
	seed = seed or random.randint(0, sys.maxsize)
	rng = random.Random(seed)

	total = len(opcodes) * max_tests
	done = 0
	last = 0.0

	batch: dict[str, list[Any]] = {}
	for opcode in opcodes:
		# Update progress bar
		done += 1
		now = time.monotonic()
		if now - last >= 0.1 or done == total:
			sys.stderr.write(f"\rGenerating... {hex(opcode)} [{done}/{total}]")
			if done >= total:
				sys.stderr.write("\n")
			sys.stderr.flush()
			last = now

		op_words = [opcode]
		for i in range(max_tests):
			operands = [rng.getrandbits(32) for _ in range(wrapper.MAX_OP_WORDS)]
			op_words.extend(operands)

			regs = _rand_regs(rng)
			ram = _rand_ram(rng)

			test = _run_test_case(cpu, i, regs, op_words, ram, model, seed)

			mnemonic = ""
			exec_result = test.get("exec_result", wrapper.EXEC_OK)
			if exec_result == wrapper.EXEC_ILLEGAL:
				mnemonic = "illegal"
			else:
				test_name = str(test.get("name", ""))
				if not test_name:
					continue
				mnemonic = _mnemonic_base(test_name)

			if not mnemonic:
				continue
			batch.setdefault(mnemonic, []).append(test)

	return batch


def _spit_file(out_dir: Path, name: str, tests: list[dict[str, Any]]) -> None:
	filename = out_dir / Path(name).with_suffix(".bin")
	print(f"Writting to {filename}!")

	data = tcs.serialize(tests)
	filename.write_bytes(data)


def main() -> None:
	args = _parse_args()
	if not args.out.is_dir():
		args.out.mkdir(parents=True, exist_ok=True)

	cpu = wrapper.CPU()
	model = wrapper.MODEL_M68010 if args.model == "m68010" else wrapper.MODEL_M68000

	opcodes = _get_valid_opcodes(cpu, model)
	batch = _generate_batch(cpu, opcodes, args.max_tests, args.seed, model)

	for name, test in batch.items():
		_spit_file(args.out, name, test)


if __name__ == "__main__":
	main()
