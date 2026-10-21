#!/usr/bin/env python3
"""Export the UI redesign documents (Markdown + SVG diagrams) as standalone HTML.

Usage: python3 tools/ui_redesign_docs/build_html.py [docs/en/dev/research/ui_redesign]

Converts README.md and every 0*.md in the folder, plus research/*.md, to sibling .html
files. `{{svg:name}}` in a Markdown file inlines `diagrams/name.svg` as a figure. Links to
`*.md` become links to `*.html`. The Markdown subset handled: ATX headings, paragraphs,
fenced code, tables, nested bullet and numbered lists, block quotes, horizontal rules,
inline code, bold, italic, links and bare URLs. No third-party packages are needed.
"""
from __future__ import annotations

import html
import re
import sys
from pathlib import Path

NAV = [
    ("README.html", "Summary"),
    ("01_pain_points.html", "01 Pain points"),
    ("02_patterns.html", "02 Patterns"),
    ("03_design.html", "03 Design"),
    ("04_architecture.html", "04 Architecture"),
    ("05_plan.html", "05 Plan"),
    ("06_frontend_options.html", "06 Frontends"),
]

CSS = """
:root {
  color-scheme: light dark;
  --bg: #fcfcfb; --fg: #0b0b0b; --fg-2: #52514e; --fg-3: #8a8985;
  --line: #d9d8d3; --panel: #f1f0ec; --code-bg: #f1f0ec; --link: #1f5fb0;
  --s1: #2a78d6; --s2: #eb6834; --s3: #1baf7a; --s4: #eda100; --s7: #6250d6; --s8: #e34948;
  --good: #006300; --warn: #8a5a00; --serious: #b43a00; --critical: #a1151d;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    --bg: #1a1a19; --fg: #f0efec; --fg-2: #c3c2b7; --fg-3: #8f8e86;
    --line: #3a3a37; --panel: #242422; --code-bg: #242422; --link: #7fb2f0;
    --s1: #3987e5; --s2: #d95926; --s3: #199e70; --s4: #c98500; --s7: #9085e9; --s8: #e66767;
    --good: #4cbb4c; --warn: #e0a84a; --serious: #ef7d4a; --critical: #ef6b6b;
  }
}
:root[data-theme="dark"] {
  --bg: #1a1a19; --fg: #f0efec; --fg-2: #c3c2b7; --fg-3: #8f8e86;
  --line: #3a3a37; --panel: #242422; --code-bg: #242422; --link: #7fb2f0;
  --s1: #3987e5; --s2: #d95926; --s3: #199e70; --s4: #c98500; --s7: #9085e9; --s8: #e66767;
  --good: #4cbb4c; --warn: #e0a84a; --serious: #ef7d4a; --critical: #ef6b6b;
}
html { background: var(--bg); }
body { margin: 0; background: var(--bg); color: var(--fg);
  font: 16px/1.55 system-ui, -apple-system, "Segoe UI", Roboto, sans-serif; }
nav.top { position: sticky; top: 0; background: var(--bg); border-bottom: 1px solid var(--line);
  padding: 8px 16px; display: flex; flex-wrap: wrap; gap: 4px 16px; font-size: 14px; z-index: 2; }
nav.top a { color: var(--fg-2); text-decoration: none; }
nav.top a.current { color: var(--fg); font-weight: 600; border-bottom: 2px solid var(--s1); }
main { max-width: 960px; margin: 0 auto; padding: 16px 16px 64px; }
h1 { font-size: 1.9em; line-height: 1.2; margin: 0.6em 0 0.4em; }
h2 { font-size: 1.4em; margin: 1.6em 0 0.5em; border-bottom: 1px solid var(--line); padding-bottom: 4px; }
h3 { font-size: 1.15em; margin: 1.3em 0 0.4em; }
h4 { font-size: 1em; margin: 1.1em 0 0.3em; color: var(--fg-2); }
a { color: var(--link); }
code { font: 0.92em/1.4 ui-monospace, "Cascadia Mono", Menlo, Consolas, monospace;
  background: var(--code-bg); padding: 1px 4px; border-radius: 3px; }
pre { background: var(--code-bg); border: 1px solid var(--line); border-radius: 6px;
  padding: 12px; overflow-x: auto; font-size: 13px; line-height: 1.35; }
pre code { background: none; padding: 0; font-size: inherit; }
blockquote { margin: 0.8em 0; padding: 4px 14px; border-left: 3px solid var(--s1);
  background: var(--panel); color: var(--fg-2); }
blockquote p { margin: 0.3em 0; }
table { border-collapse: collapse; width: 100%; margin: 0.8em 0; font-size: 14px; display: block; overflow-x: auto; }
th, td { border: 1px solid var(--line); padding: 5px 8px; vertical-align: top; text-align: left; }
th { background: var(--panel); }
hr { border: 0; border-top: 1px solid var(--line); margin: 1.5em 0; }
figure.diagram { margin: 1.2em 0; padding: 12px; border: 1px solid var(--line); border-radius: 8px; background: var(--panel); }
figure.diagram svg { width: 100%; height: auto; display: block; font: 13px/1.3 system-ui, sans-serif; }
figure.diagram figcaption { color: var(--fg-2); font-size: 13px; margin-top: 8px; }
.tag { font-size: 12px; color: var(--fg-3); }
/* diagram tokens */
.d-box { fill: var(--bg); stroke: var(--fg-2); stroke-width: 1.2; }
.d-box-soft { fill: var(--panel); stroke: var(--line); stroke-width: 1; }
.d-text { fill: var(--fg); }
.d-text-2 { fill: var(--fg-2); }
.d-text-3 { fill: var(--fg-3); font-size: 11px; }
.d-line { stroke: var(--fg-2); stroke-width: 1.4; fill: none; }
.d-line-soft { stroke: var(--line); stroke-width: 1; fill: none; }
.d-arrow { fill: var(--fg-2); }
.d-s1 { fill: var(--s1); } .d-s2 { fill: var(--s2); } .d-s3 { fill: var(--s3); }
.d-s4 { fill: var(--s4); } .d-s7 { fill: var(--s7); } .d-s8 { fill: var(--s8); }
.d-stroke-s1 { stroke: var(--s1); } .d-stroke-s2 { stroke: var(--s2); } .d-stroke-s3 { stroke: var(--s3); }
.d-stroke-s8 { stroke: var(--s8); }
.d-good { fill: var(--good); } .d-warn { fill: var(--warn); } .d-critical { fill: var(--critical); }
.d-hatch { fill: url(#hatch); }
@media (max-width: 600px) { body { font-size: 15px; } main { padding: 12px 16px 48px; } }
"""


def inline(text: str, md_dir: Path) -> str:
    """Inline Markdown to HTML. Code spans are protected first."""
    out: list[str] = []
    parts = re.split(r"(`[^`]*`)", text)
    for part in parts:
        if part.startswith("`") and part.endswith("`") and len(part) >= 2:
            out.append("<code>" + html.escape(part[1:-1]) + "</code>")
            continue
        s = html.escape(part, quote=False)
        s = re.sub(r"\[([^\]]+)\]\(([^)\s]+)\)", lambda m: link(m.group(1), m.group(2)), s)
        s = re.sub(r"(?<![\"'>=])(https?://[^\s<)]+)", r'<a href="\1">\1</a>', s)
        s = re.sub(r"\*\*(.+?)\*\*", r"<strong>\1</strong>", s)
        s = re.sub(r"(?<![\w*])\*(?!\s)(.+?)(?<!\s)\*(?![\w*])", r"<em>\1</em>", s)
        s = s.replace("\\*", "*")
        out.append(s)
    return "".join(out)


def link(text: str, href: str) -> str:
    if not href.startswith(("http://", "https://", "#", "mailto:")):
        href = re.sub(r"\.md(?=(#|$))", ".html", href)
    return f'<a href="{href}">{text}</a>'


def table(rows: list[str], md_dir: Path) -> str:
    cells = [[c.strip() for c in r.strip().strip("|").split("|")] for r in rows]
    head, body = cells[0], cells[2:]
    h = "<table><thead><tr>" + "".join(f"<th>{inline(c, md_dir)}</th>" for c in head) + "</tr></thead><tbody>"
    for r in body:
        h += "<tr>" + "".join(f"<td>{inline(c, md_dir)}</td>" for c in r) + "</tr>"
    return h + "</tbody></table>"


def figure(name: str, md_dir: Path) -> str:
    path = md_dir / "diagrams" / f"{name}.svg"
    if not path.exists():
        return f'<p class="tag">[diagram {name} missing]</p>'
    svg = path.read_text(encoding="utf-8")
    caption = ""
    m = re.search(r"<title>(.*?)</title>", svg, re.S)
    if m:
        caption = html.escape(m.group(1).strip())
    return f'<figure class="diagram">{svg}<figcaption>{caption}</figcaption></figure>'


def convert(md: str, md_dir: Path) -> tuple[str, str]:
    lines = md.splitlines()
    out: list[str] = []
    title = ""
    i = 0
    list_stack: list[tuple[int, str]] = []  # (indent, tag)

    def close_lists(to_indent: int = -1) -> None:
        while list_stack and list_stack[-1][0] > to_indent:
            out.append(f"</li></{list_stack.pop()[1]}>")

    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        if stripped.startswith("```"):
            close_lists()
            j = i + 1
            code: list[str] = []
            while j < len(lines) and not lines[j].strip().startswith("```"):
                code.append(lines[j])
                j += 1
            out.append("<pre><code>" + html.escape("\n".join(code)) + "</code></pre>")
            i = j + 1
            continue
        m = re.match(r"^(#{1,6})\s+(.*)$", line)
        if m:
            close_lists()
            level = len(m.group(1))
            text = m.group(2).strip()
            if level == 1 and not title:
                title = text
            anchor = re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")
            out.append(f'<h{level} id="{anchor}">{inline(text, md_dir)}</h{level}>')
            i += 1
            continue
        if re.match(r"^\s*\{\{svg:([a-z0-9_]+)\}\}\s*$", stripped):
            close_lists()
            out.append(figure(re.match(r"\{\{svg:([a-z0-9_]+)\}\}", stripped).group(1), md_dir))
            i += 1
            continue
        if stripped.startswith("|") and i + 1 < len(lines) and re.match(r"^\s*\|?\s*:?-{2,}", lines[i + 1]):
            close_lists()
            rows = []
            while i < len(lines) and lines[i].strip().startswith("|"):
                rows.append(lines[i])
                i += 1
            out.append(table(rows, md_dir))
            continue
        if re.match(r"^\s*(-{3,}|\*{3,})\s*$", line):
            close_lists()
            out.append("<hr>")
            i += 1
            continue
        if stripped.startswith(">"):
            close_lists()
            quote: list[str] = []
            while i < len(lines) and lines[i].strip().startswith(">"):
                quote.append(lines[i].strip()[1:].strip())
                i += 1
            paras = "\n".join(quote).split("\n\n")
            out.append("<blockquote>" + "".join(f"<p>{inline(p.replace(chr(10), ' '), md_dir)}</p>" for p in paras) + "</blockquote>")
            continue
        m = re.match(r"^(\s*)([-*]|\d+\.)\s+(.*)$", line)
        if m:
            indent = len(m.group(1).expandtabs(4))
            tag = "ol" if m.group(2)[0].isdigit() else "ul"
            text = m.group(3)
            # continuation lines of the same item
            j = i + 1
            while j < len(lines) and lines[j].strip() and not re.match(r"^(\s*)([-*]|\d+\.)\s+", lines[j]) \
                    and len(lines[j]) - len(lines[j].lstrip()) > indent and not lines[j].strip().startswith(("|", "```", "{{svg")):
                text += " " + lines[j].strip()
                j += 1
            if list_stack and indent > list_stack[-1][0]:
                out.append(f"<{tag}>")
                list_stack.append((indent, tag))
            else:
                while list_stack and list_stack[-1][0] > indent:
                    out.append(f"</li></{list_stack.pop()[1]}>")
                if list_stack and list_stack[-1][0] == indent:
                    out.append("</li>")
                else:
                    out.append(f"<{tag}>")
                    list_stack.append((indent, tag))
            start = ""
            if tag == "ol" and (not out or out[-1] == "<ol>"):
                n = int(m.group(2)[:-1])
                if n != 1:
                    start = f' value="{n}"'
            out.append(f"<li{start}>{inline(text, md_dir)}")
            i = j
            continue
        if not stripped:
            if list_stack and i + 1 < len(lines) and re.match(r"^\s+([-*]|\d+\.)\s+", lines[i + 1]):
                i += 1
                continue
            close_lists()
            i += 1
            continue
        # paragraph
        close_lists()
        para = [stripped]
        j = i + 1
        while j < len(lines) and lines[j].strip() and not re.match(r"^(#{1,6}\s|\s*([-*]|\d+\.)\s|\s*>|\s*\||```|\s*\{\{svg)", lines[j]):
            para.append(lines[j].strip())
            j += 1
        out.append(f"<p>{inline(' '.join(para), md_dir)}</p>")
        i = j
    close_lists()
    return title, "\n".join(out)


def page(title: str, body: str, current: str, depth: int) -> str:
    prefix = "../" * depth
    nav = "".join(
        f'<a href="{prefix}{href}"{" class=\"current\"" if href == current else ""}>{label}</a>'
        for href, label in NAV
    )
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(title)}</title>
<style>{CSS}</style>
</head>
<body>
<nav class="top">{nav}</nav>
<main>
{body}
</main>
</body>
</html>
"""


def build(root: Path) -> list[Path]:
    written: list[Path] = []
    targets = [root / "README.md"] + sorted(root.glob("0*.md"))
    research = sorted((root / "research").glob("*.md")) if (root / "research").exists() else []
    for md_path in targets + research:
        depth = 1 if md_path.parent.name == "research" else 0
        title, body = convert(md_path.read_text(encoding="utf-8"), root)
        out = md_path.with_suffix(".html")
        out.write_text(page(title or md_path.stem, body, out.name, depth), encoding="utf-8")
        written.append(out)
    return written


if __name__ == "__main__":
    folder = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("docs/en/dev/research/ui_redesign")
    for p in build(folder):
        print(p)
