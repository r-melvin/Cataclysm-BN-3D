"""Format top-level C++ with astyle 3.4.7, preserving trailing-return indentation."""
import argparse
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", nargs="+", type=Path)
    args = parser.parse_args()
    try:
        from astyle_py.astyle_wrapper import Astyle
    except ImportError:
        parser.exit(1, "error: install astyle-py==1.1.0 in this Python environment, or run "
                    "uvx --from astyle-py==1.1.0 python tools/r3d_spike/astyle_347.py <files>\n")

    try:
        config = Path(__file__).resolve().parents[2] / ".astylerc"
        opts = " ".join(line.strip() for line in config.read_text(encoding="utf-8").splitlines()
                        if line.strip() and not line.lstrip().startswith("#"))
        astyle = Astyle(version="3.4.7")
        astyle.set_options(opts)
        for path in args.files:
            src = path.read_text(encoding="utf-8")
            out = astyle.format(src)
            if out != src:
                path.write_text(out, encoding="utf-8", newline="\n")
                print(f"formatted {path}")
    except Exception as error:
        parser.exit(1, f"error: astyle 3.4.7 formatting failed: {error}\n"
                    "Check the input files, package installation and writable formatter cache.\n")


if __name__ == "__main__":
    main()
