import argparse
from pathlib import Path
from wrapper import T68K

def _parse_args() -> argparse.Namespace:
	parser = argparse.ArgumentParser(
		description="Generate M68K instruction tests"
	)
	_ = parser.add_argument(
		"-o",
		"--out",
		type=Path,
		required=True,
		help="Output directory for test files"
	)

	return parser.parse_args()


def main() -> None:
	args = _parse_args()
	if not args.out.is_dir():
		return
	outpath = args.out

	t68k = T68K()


if __name__ == "__main__":
	main()
