"""Format files with astyle 3.4.7 (local astyle 3.6.19 mis-indents trailing-return functions)."""
import sys
from pathlib import Path

from astyle_py.astyle_wrapper import Astyle

opts = " ".join(
    line.strip() for line in Path(".astylerc").read_text().splitlines()
    if line.strip() and not line.startswith("#")
)
astyle = Astyle(version="3.4.7")
astyle.set_options(opts)
for name in sys.argv[1:]:
    path = Path(name)
    src = path.read_text()
    out = astyle.format(src)
    if out != src:
        path.write_text(out)
        print(f"formatted {name}")
