"""Format top-level C++ with astyle 3.4.7, preserving trailing-return indentation."""
import argparse
from pathlib import Path
import re
import subprocess


CPP_TOKENS = re.compile(
    r'//(?:\\\r?\n|[^\n])*|/\*.*?\*/|'
    r'(?P<raw>(?:u8|u|U|L)?R"(?P<delimiter>[^\s()\\]{0,16})\(.*?\)(?P=delimiter)")|'
    r'"(?:\\.|[^"\\])*"|'
    r"(?<![\w'])(?:u8|u|U|L)?'(?:\\.|[^'\\\r\n])*'",
    re.DOTALL,
)


def format_preserving_raw_literals(source, formatter):
    """Keep embedded help, JSON, Lua and artwork byte-for-byte inside raw tokens."""
    prefix = "__BN_RAW_LITERAL_"
    while prefix in source:
        prefix += "_"
    protected = {}

    def mask(match):
        if match.group("raw") is None:
            return match.group()
        token = f'"{prefix}{len(protected)}__"'
        protected[token] = match.group()
        return token

    formatted = formatter(CPP_TOKENS.sub(mask, source))
    for token, literal in protected.items():
        if formatted.count(token) != 1:
            raise ValueError("formatter changed a protected raw-string marker")
        formatted = formatted.replace(token, literal, 1)
    return formatted


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", type=Path, help="native astyle 3.4.7 executable")
    parser.add_argument("files", nargs="+", type=Path)
    args = parser.parse_args()
    try:
        config = Path(__file__).resolve().parents[2] / ".astylerc"
        if args.native:
            version = subprocess.check_output([str(args.native), "--version"],
                                              encoding="utf-8", stderr=subprocess.STDOUT).strip()
            if version != "Artistic Style Version 3.4.7":
                raise ValueError(f"expected native astyle 3.4.7, found {version}")

            def formatter(source):
                return subprocess.check_output(
                    [str(args.native), "--mode=c", f"--options={config}"], input=source,
                    encoding="utf-8", stderr=subprocess.PIPE)
        else:
            try:
                from astyle_py.astyle_wrapper import Astyle
            except ImportError:
                parser.exit(1, "error: install astyle-py==1.1.0 in this Python environment, or run "
                            "uvx --from astyle-py==1.1.0 python tools/r3d_spike/astyle_347.py <files>\n")
            opts = " ".join(line.strip() for line in config.read_text(encoding="utf-8").splitlines()
                            if line.strip() and not line.lstrip().startswith("#"))
            astyle = Astyle(version="3.4.7")
            astyle.set_options(opts)
            formatter = astyle.format
        for path in args.files:
            src = path.read_text(encoding="utf-8")
            out = format_preserving_raw_literals(src, formatter)
            if out != src:
                path.write_text(out, encoding="utf-8", newline="\n")
                print(f"formatted {path}")
    except Exception as error:
        parser.exit(1, f"error: astyle 3.4.7 formatting failed: {error}\n"
                    "Check the input files, package installation and writable formatter cache.\n")


if __name__ == "__main__":
    main()
