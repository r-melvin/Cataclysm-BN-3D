#!/usr/bin/env python3
"""Play in an isolated user directory, collect paired renderer checkpoints, and share ZIPs."""
import argparse
import contextlib
import html
import hashlib
import json
import os
from pathlib import Path
import shutil
import sqlite3
import subprocess
import struct
import sys
import tempfile
import time
import zipfile
import re

ROOT = Path(__file__).resolve().parents[2]
CONFIG_FILES = ("options.json", "keybindings.json", "auto_pickup.json", "safemode.json",
                "auto_note.json", "colors.json", "base_colors.json", "custom_colors.json",
                "preload.json", "fonts.json", "panel_options.json")
MAX_PAYLOAD = 24_000_000  # GitHub says 25 MB; reserve space for ZIP headers.
CHUNK = 20_000_000


def tile_sprite_index(config_path):
    """Index tileset IDs to their source atlas cell and dimensions."""
    config = json.loads(config_path.read_text())
    result = {}

    def first_sprite(value):
        if isinstance(value, int):
            return value
        if isinstance(value, list):
            for entry in value:
                sprite = entry.get("sprite") if isinstance(entry, dict) else entry
                if isinstance(sprite, int):
                    return sprite
        return None

    for part in config.get("tiles-new", []):
        atlas = config_path.parent / part["file"]
        if not atlas.is_file():
            continue
        for tile in part.get("tiles", []):
            ids = tile.get("id", [])
            ids = ids if isinstance(ids, list) else [ids]
            sprite = first_sprite(tile.get("fg"))
            if sprite is None:
                continue
            for tile_id in ids:
                if isinstance(tile_id, str):
                    result.setdefault(tile_id, dict(atlas=atlas, sprite=sprite,
                                                    width=part.get("sprite_width", config["tile_info"][0]["width"]),
                                                    height=part.get("sprite_height", config["tile_info"][0]["height"])))
            for additional in tile.get("additional_tiles", []):
                extra_ids = additional.get("id", [])
                extra_ids = extra_ids if isinstance(extra_ids, list) else [extra_ids]
                extra_sprite = first_sprite(additional.get("fg"))
                if extra_sprite is not None:
                    for tile_id in ids:
                        if not isinstance(tile_id, str):
                            continue
                        for extra_id in extra_ids:
                            if isinstance(extra_id, str):
                                result.setdefault(f"{tile_id}_{extra_id}", dict(
                                    atlas=atlas, sprite=extra_sprite,
                                    width=part.get("sprite_width", config["tile_info"][0]["width"]),
                                    height=part.get("sprite_height", config["tile_info"][0]["height"])))
    return result


def placement_records(map_path):
    """Read the stable, whitespace-delimited placement section in map.txt."""
    records = []
    in_placements = False
    for line in map_path.read_text(errors="replace").splitlines():
        if line.startswith("placements (category id x y support_x support_y):"):
            in_placements = True
            continue
        if in_placements:
            fields = line.split()
            if len(fields) == 6:
                try:
                    records.append(dict(category=fields[0], id=fields[1], x=int(fields[2]),
                                        y=int(fields[3]), support_x=int(fields[4]), support_y=int(fields[5])))
                except ValueError:
                    continue
    return records


def render_art_gallery(capture, assets_root=None):
    """Create a sprite-to-render gallery and state/anchor reports for one checkpoint."""
    map_path = capture / "view_map.txt"
    if not map_path.is_file():
        return
    records = placement_records(map_path)
    if not records:
        return
    gallery = capture / "art-gallery"
    sprites = gallery / "sprites"
    sprites.mkdir(parents=True, exist_ok=True)
    asset_base = assets_root if assets_root and assets_root.is_dir() else ROOT / "gfx"
    indexes = {name: tile_sprite_index(asset_base / name / "tile_config.json")
               for name in ("HitButton_iso", "UltimateCataclysm")}
    profile_path = (assets_root / "presentation_profiles.json") if assets_root else ROOT / "data/raw/r3d/presentation_profiles.json"
    authored_profiles = {}
    if profile_path.is_file():
        profile_data = json.loads(profile_path.read_text())
        for profile in profile_data.get("profiles", []):
            authored_profiles[(profile.get("category"), profile.get("id"))] = profile
    copied = {}
    cards = []
    unique_records = {}
    for record in records:
        unique_records.setdefault((record["category"], record["id"]), record)
    for record in unique_records.values():
        variants = []
        for tileset, index in indexes.items():
            sprite = index.get(record["id"])
            if not sprite:
                continue
            atlas = sprite["atlas"]
            rel = copied.get(atlas)
            if rel is None:
                rel = f"sprites/{len(copied):03d}-{atlas.name}"
                shutil.copy2(atlas, gallery / rel)
                copied[atlas] = rel
            # Sprite IDs are zero-based cells in their source sheet.
            with atlas.open("rb") as image:
                header = image.read(24)
            if header[:8] != b"\x89PNG\r\n\x1a\n":
                continue
            image_width, image_height = struct.unpack(">II", header[16:24])
            columns = image_width // sprite["width"]
            x = (sprite["sprite"] % columns) * sprite["width"]
            y = (sprite["sprite"] // columns) * sprite["height"]
            if x + sprite["width"] > image_width or y + sprite["height"] > image_height:
                continue
            variants.append((tileset, rel, x, y, sprite["width"], sprite["height"],
                             image_width, image_height))
        if variants:
            cards.append((record, variants))

    # Plot occupied map cells and their declared tile-ground support anchors.
    min_x = min(record["x"] for record in records)
    max_x = max(record["x"] for record in records)
    min_y = min(record["y"] for record in records)
    max_y = max(record["y"] for record in records)
    cell = 22
    svg = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {(max_x-min_x+1)*cell} {(max_y-min_y+1)*cell}">',
           '<style>text{font:8px monospace}.terrain{fill:#718096}.furniture{fill:#f6ad55}.item{fill:#63b3ed}</style>']
    for x in range(min_x, max_x + 1):
        for y in range(min_y, max_y + 1):
            svg.append(f'<path d="M{(x-min_x)*cell} {(y-min_y)*cell}h{cell}v{cell}h-{cell}z" fill="none" stroke="#bbb" stroke-width=".5"/>')
    for record in records:
        px, py = (record["x"]-min_x)*cell, (record["y"]-min_y)*cell
        color = record["category"] if record["category"] in ("terrain", "furniture", "item") else "terrain"
        svg.append(f'<circle class="{color}" cx="{px+cell/2+4}" cy="{py+cell/2-4}" r="4"><title>{html.escape(record["category"]+": "+record["id"])}</title></circle>')
        if record["category"] != "terrain":
            sx, sy = (record["support_x"]-min_x)*cell+cell/2, (record["support_y"]-min_y)*cell+cell/2
            svg.append(f'<path d="M{px+cell/2+4} {py+cell/2-4}L{sx} {sy}" stroke="#e53e3e" stroke-width="1"/><rect x="{sx-3}" y="{sy-3}" width="6" height="6" fill="none" stroke="#e53e3e"/>')
    svg.append("</svg>")
    (gallery / "support-map.svg").write_text("\n".join(svg))

    # State sprite presence is derived from the packaged tilesets, and observed
    # state IDs are marked separately from variants that need a gameplay capture.
    state_groups = {}
    for index in indexes.values():
        for tile_id in index:
            match = re.fullmatch(r"(t_(?:door|window)(?:_.+)?)(?:_(c|o|b|open|broken|closed))", tile_id)
            if match:
                family, state = match.groups()
                label = {"c": "closed", "o": "open", "b": "broken", "open": "open", "broken": "broken", "closed": "closed"}[state]
                state_groups.setdefault(family, set()).add(label)
            for suffix, state in (("_open", "open"), ("_broken", "broken")):
                if tile_id.startswith("t_window_") and tile_id.endswith(suffix):
                    family = tile_id[:-len(suffix)]
                    state_groups.setdefault(family, set()).add(state)
                    if family in index:
                        state_groups[family].add("closed")
    observed_states = {}
    observed_ids = {record["id"] for record in records}
    for record in records:
        match = re.fullmatch(r"(t_(?:door|window)(?:_.+)?)(?:_(c|o|b|open|broken|closed))", record["id"])
        if match:
            family, state = match.groups()
            state = {"c": "closed", "o": "open", "b": "broken"}.get(state, state)
            observed_states.setdefault(family, set()).add(state)
    for index in indexes.values():
        for tile_id in index:
            if tile_id.startswith("t_window_") and tile_id in observed_ids:
                observed_states.setdefault(tile_id, set()).add("closed")
    state_rows = []
    for family, available in sorted(state_groups.items()):
        states = [state for state in ("closed", "open", "broken") if state in available]
        if states:
            state_rows.append((family, states, observed_states.get(family, set())))

    sections = []
    for record, variants in cards:
        pictures = []
        for tileset, source, x, y, width, height, atlas_width, atlas_height in variants:
            scale = 4
            pictures.append(f'<figure><figcaption>{html.escape(tileset)}</figcaption><div class="sprite" style="width:{width*scale}px;height:{height*scale}px;background-size:{atlas_width*scale}px {atlas_height*scale}px;background-image:url(\'{html.escape(source)}\');background-position:-{x*scale}px -{y*scale}px"></div></figure>')
        placements = sum(1 for candidate in records
                         if candidate["category"] == record["category"] and candidate["id"] == record["id"])
        profile_category = {"item": "item", "furniture": "furniture", "terrain": "terrain"}.get(record["category"], record["category"])
        profile = authored_profiles.get((profile_category, record["id"]))
        profile_details = ""
        if profile:
            size = profile.get("size", [0, 0, 0])
            anchor = profile.get("support_anchor", [0.5, 0.5, 0])
            regions = profile.get("material_regions", [])
            region_list = "".join(f'<li>{html.escape(region.get("name", "region"))}: {html.escape(region.get("role", "unassigned"))} (palette {html.escape(", ".join(map(str, region.get("palette_indices", []))))})</li>' for region in regions)
            face_list = "".join(
                f'<li>{html.escape(str(source.get("face", "face")))}: '
                f'{html.escape(str(source.get("tileset", "")))}/'
                f'{html.escape(str(source.get("sprite", "")))}; '
                f'rotation {html.escape(str(source.get("rotation", 0)))}; '
                f'crop {html.escape(str(source.get("crop", [0, 0, 1, 1])))}</li>'
                for source in profile.get("face_sources", []))
            face_details = f'<strong>Face sources</strong><ul>{face_list}</ul>' if face_list else ""
            forward, right, up = (value if value > 0 else 1.0 for value in size)
            scale = 72 / max(forward, right, up)
            footprint_width, footprint_depth, elevation_width, elevation_height = (
                forward * scale, right * scale, forward * scale, up * scale)
            footprint_x = 46 - footprint_width / 2
            footprint_y = 58 - footprint_depth / 2
            support_x = footprint_x + footprint_width * anchor[0]
            support_y = footprint_y + footprint_depth * (1 - anchor[1])
            elevation_x = 152
            elevation_bottom = 94
            elevation_support_x = elevation_x + elevation_width * anchor[0]
            elevation_support_y = elevation_bottom - elevation_height * anchor[2]
            diagram = (f'<svg class="profile-diagram" viewBox="0 0 250 116" role="img" aria-label="Top footprint and side elevation with support anchors">'
                       f'<text x="8" y="12">TOP · F {forward:g} × R {right:g}</text>'
                       f'<rect x="{footprint_x:g}" y="{footprint_y:g}" width="{footprint_width:g}" height="{footprint_depth:g}" fill="#30465b" stroke="#9cf"/>'
                       f'<path d="M{support_x - 5:g} {support_y:g}h10m-5 -5v10" stroke="#ff6868" stroke-width="2"/>'
                       f'<text x="8" y="108">SIDE · F {forward:g} × U {up:g}</text>'
                       f'<rect x="{elevation_x}" y="{elevation_bottom - elevation_height:g}" width="{elevation_width:g}" height="{elevation_height:g}" fill="#30465b" stroke="#9cf"/>'
                       f'<path d="M{elevation_support_x - 5:g} {elevation_support_y:g}h10m-5 -5v10" stroke="#ff6868" stroke-width="2"/>'
                       '</svg>')
            profile_details = (f'<section class="profile"><strong>Presentation profile</strong><p>Size F/R/U: {html.escape(", ".join(map(str, size)))} tiles; support anchor: {html.escape(", ".join(map(str, anchor)))}; sprite depth: {html.escape(str(profile.get("depth_fraction", "inferred")))}</p>{diagram}<p class="anchor-legend">Red crosses mark support anchors; F = forward, R = right, U = up.</p><ul>{region_list}</ul>{face_details}</section>')
        views = [f'<a href="../view_3d_yaw_{angle}.png"><img loading="lazy" width="240" src="../view_3d_yaw_{angle}.png">3D yaw {angle*45-45}°</a>' for angle in range(8)]
        views.append('<a href="../view_2d.png"><img loading="lazy" width="240" src="../view_2d.png">2D checkpoint</a>')
        sections.append(f'<article><h3>{html.escape(record["category"])}: <code>{html.escape(record["id"])}</code></h3><p>{placements} placement(s); example tile ({record["x"]}, {record["y"]}); support anchor ({record["support_x"]}, {record["support_y"]}).</p><div class="sprites">{"".join(pictures)}</div>{profile_details}<div class="views">' + " ".join(views) + '</div></article>')
    state_table = "".join(f'<tr><td><code>{html.escape(family)}</code></td><td>{", ".join(states)}</td><td>{", ".join(state for state in states if state in observed) or "none"}</td></tr>' for family, states, observed in state_rows)
    page = ('<!doctype html><meta charset="utf-8"><title>Renderer art gallery</title>'
            '<style>body{font:14px system-ui;background:#161616;color:#eee;margin:2rem}a{color:#9cf}article{border-top:1px solid #555;padding:1rem 0}.sprites{display:flex;gap:1rem;min-height:120px;align-items:start}.sprite{image-rendering:pixelated;background-repeat:no-repeat}figure{margin:0}figcaption{margin-bottom:.4rem}.profile{border-left:3px solid #9cf;padding:.1rem .8rem;margin:.8rem 0}.profile p{margin:.4rem 0}.profile-diagram{width:min(100%,500px);background:#20242a;border:1px solid #555}.profile-diagram text{fill:#ddd;font:9px system-ui}.anchor-legend{color:#f88}.profile ul{display:flex;gap:1.5rem;flex-wrap:wrap;margin:.3rem 0}.views{display:flex;gap:.7rem;flex-wrap:wrap}.views a{display:grid;gap:.3rem}.views img{height:auto;border:1px solid #555}table{border-collapse:collapse}td,th{border:1px solid #555;padding:.4rem;text-align:left}</style>'
            '<h1>Sprite-to-view gallery</h1><p>Source atlas sprites are paired with the same-state 2D image and eight 3D camera poses. Red lines mark the renderer placement origin and its tile-ground support anchor; the grid is in map coordinates.</p>'
            '<p>Anchor map legend: grey terrain, amber furniture, blue loose items; red square is the tile-ground support anchor.</p>'
            '<img src="support-map.svg" style="max-width:100%;image-rendering:pixelated"><h2>Open / closed / broken sprite coverage</h2><table><tr><th>Family</th><th>States in shipped tileset</th><th>States seen in checkpoint</th></tr>' + state_table + '</table><h2>Visible placements</h2>' + "\n".join(sections))
    (gallery / "index.html").write_text(page)
RESTORE = r'''#!/usr/bin/env python3
"""Run after extracting ALL numbered ZIPs into the same empty directory."""
import hashlib, json
from pathlib import Path, PurePosixPath, PureWindowsPath
root = Path(__file__).resolve().parent

def checked_path(value):
    if not isinstance(value, str) or not value or '\\' in value:
        raise ValueError('Invalid archive path')
    relative = PurePosixPath(value)
    if relative.is_absolute() or PureWindowsPath(value).drive or '..' in relative.parts or not relative.parts:
        raise ValueError('Unsafe archive path: ' + value)
    path = root
    for component in relative.parts:
        path = path / component
        if path.is_symlink():
            raise ValueError('Symlink in archive path: ' + value)
    return path

jobs = []
targets = set()
for entry in json.loads((root / 'large-files.json').read_text()):
    target = checked_path(entry['path'])
    if target.exists() or target in targets:
        raise ValueError('Refusing to overwrite existing or duplicate file: ' + str(target))
    targets.add(target)
    parts = [checked_path(part) for part in entry['parts']]
    if not parts or not all(part.is_file() for part in parts):
        raise ValueError('Missing archive chunks')
    jobs.append((target, parts, entry['sha256']))
for target, parts, expected in jobs:
    target.parent.mkdir(parents=True, exist_ok=True)
    digest = hashlib.sha256()
    out = target.open('xb')
    try:
        with out:
            for part in parts:
                with part.open('rb') as source:
                    while data := source.read(1024 * 1024):
                        digest.update(data)
                        out.write(data)
        if digest.hexdigest() != expected:
            raise ValueError('Archive checksum mismatch: ' + str(target))
    except BaseException:
        target.unlink(missing_ok=True)
        raise
print('Save files restored. See README.txt for loading instructions.')
'''


def copy_tree(source, target):
    """Copy regular files only; never traverse links out of the selected world."""
    if source.is_symlink():
        raise ValueError(f"Refusing symlink: {source}")
    for directory, dirs, files in os.walk(source, followlinks=False):
        relative = Path(directory).relative_to(source)
        (target / relative).mkdir(parents=True, exist_ok=True)
        for name in dirs + files:
            if (Path(directory) / name).is_symlink():
                raise ValueError(f"Refusing symlink: {Path(directory) / name}")
        for name in files:
            src = Path(directory) / name
            if src.is_file():
                destination = target / relative / name
                if src.suffix == ".sqlite3":
                    with contextlib.closing(sqlite3.connect(src.as_uri() + "?mode=ro", uri=True)) as database:
                        with contextlib.closing(sqlite3.connect(destination)) as snapshot:
                            database.backup(snapshot)
                            snapshot.execute("PRAGMA journal_mode=DELETE")
                elif not name.endswith(("-wal", "-shm", "-journal")):
                    shutil.copy2(src, destination)


def preflight(binary, env, platform=sys.platform):
    """Fail before play when the selected executable/assets cannot capture."""
    version = subprocess.run([str(binary), "--version"], cwd=ROOT, env=env,
                             capture_output=True, text=True, timeout=30)
    if version.returncode or "Renderer capture protocol: 1" not in version.stdout.splitlines():
        raise ValueError(f"{binary} does not support this capture launcher. Rebuild the tiles "
                         "executable from this checkout, or select the rebuilt executable with --binary.")
    backend = env.get("SDL_GPU_DRIVER", "").lower()
    suffix = {"vulkan": "spv", "direct3d12": "dxil", "metal": "msl"}.get(backend)
    if suffix is None:
        suffix = "msl" if platform == "darwin" else "dxil" if platform == "win32" else "spv"
    missing = [f"{name}.{suffix}" for name in ("r3d_block_vertex", "r3d_block_fragment",
                                                "r3d_silhouette_fragment")
               if not (ROOT / "data" / "shaders" / f"{name}.{suffix}").is_file()]
    if missing:
        raise ValueError("Missing compiled 3D shaders: " + ", ".join(missing) +
                         f". Rebuild with SHADER_TARGETS={suffix}; see tools/r3d_spike/COMPARE.md.")
    return version.stdout.strip()


def failure_details(session):
    lines = []
    status = session / "captures" / "session-status.txt"
    if status.is_file():
        lines.append("Last capture status: " + status.read_text(errors="replace").strip())
    else:
        lines.append("The game never initialized capture support. Check startup errors and the selected binary.")
    failed = sorted((session / "captures").glob("capture-*/FAILED.txt"))
    for marker in failed[:3]:
        lines.append(f"{marker.parent.name}: {marker.read_text(errors='replace').strip()}")
    if not failed:
        lines.append("Load or create a character and reach normal gameplay input; menus and idle waiting do not capture.")
    log = session / "user" / "config" / "debug.log"
    if log.is_file():
        errors = [line for line in log.read_text(errors="replace").splitlines() if "ERROR" in line]
        capture_errors = [line for line in errors if "r3d:" in line or "r3d compare:" in line]
        lines.extend((capture_errors or errors)[-8:])
    lines.append(f"Private diagnostics: {session / 'stdout.log'} and {log}")
    return "\n".join(lines)


def package(session):
    """Only publish completed checkpoints and explicitly selected diagnostics."""
    captures = session / "captures"
    completed = [p for p in sorted(captures.glob("capture-*"))
                 if (p / "COMPLETE.txt").is_file() and not (p / "FAILED.txt").exists()]
    if not completed:
        raise ValueError("No complete checkpoints. No archive made.\n" + failure_details(session))
    with tempfile.TemporaryDirectory(prefix="r3d-package-") as temporary:
        staging = Path(temporary)
        for capture in completed:
            render_art_gallery(capture, session / "art-assets")
            copy_tree(capture, staging / "captures" / capture.name)
        diagnostics = staging / "diagnostics"
        diagnostics.mkdir()
        for name in ("debug.log", "debug.log.prev", "crash.log"):
            src = session / "user" / "config" / name
            if src.is_file() and not src.is_symlink():
                shutil.copy2(src, diagnostics / name)
        for name in ("session.json", "stdout.log"):
            src = session / name
            if src.is_file() and not src.is_symlink():
                shutil.copy2(src, diagnostics / name)
        # Checkpoints carry their own rendering configuration. Keep the final options
        # alongside the logs; never sweep other private configuration or worlds.
        src = session / "user" / "config" / "options.json"
        if src.is_file() and not src.is_symlink():
            shutil.copy2(src, diagnostics / "options.json")
        (staging / "README.txt").write_text(
            "Renderer comparison report\n\n"
            "Extract ALL numbered ZIPs into the same empty directory, then run python3 restore.py.\n"
            "Each captures/capture-NNNN is an independent same-state checkpoint. Copy its save/\n"
            "AND config/ directories into a NEW user directory, then launch the recorded build with\n"
            "--userdir that-directory/ --configdir that-directory/config/ --world WORLD.\n"
            "Both directory arguments are required for XDG builds. Do not overwrite your normal saves. Custom mods\n"
            "and tileset assets are not included; use the versions listed in game-report.txt.\n"
            "rng.txt records the gameplay RNG for investigation; normal loading initializes RNG\n"
            "normally. Saves reproduce the scene, not a deterministic future action sequence.\n\n"
            "view_2d.png is the fixed HitButton_iso reference for every 3D pose in this checkpoint.\n"
            "view_3d_yaw_N.png uses yaw -45 + 45*N degrees, pitch 30, orthographic projection.\n"
            "view_before.png and view_resumed.png show the player view before/after the batch.\n"
            "art-gallery/index.html pairs source sprites from both shipped tilesets with the checkpoint poses.\n"
            "Its support-map.svg marks each placement origin and tile-ground anchor. The state table\n"
            "lists open/closed/broken door and window sprites available in the tilesets and states\n"
            "actually visible in this checkpoint.\n"
            "No turn or input is processed between screenshots. Capture briefly pauses play.\n\n"
            "For a useful issue, describe the bug, reproduction steps and expected behavior.\n"
            "Attach these ZIPs; game-report.txt records OS/build/graphics/language/mods.\n"
            "Inspect before sharing: saves/logs can contain character names, notes and paths.\n"
            "This bundle intentionally includes only completed snapshots of the selected world,\n"
            "the game report, options, and session debug/crash/stdout logs.\n"
            "Each art-gallery contains source atlas images from the checkout's shipped tilesets;\n"
            "custom tileset assets are not bundled.\n\n"
            "GitHub supports ZIP attachments up to 25 MB (images separately: 10 MB), checked\n"
            "2026-10-18: https://docs.github.com/en/get-started/writing-on-github/"
            "working-with-advanced-formatting/attaching-files\n"
        )
        large = []
        for file in sorted(staging.rglob("*")):
            if not file.is_file() or file.stat().st_size <= CHUNK:
                continue
            relative = file.relative_to(staging).as_posix()
            parts = []
            digest = hashlib.sha256()
            with file.open("rb") as source:
                while data := source.read(CHUNK):
                    digest.update(data)
                    name = f"chunks/{len(large):04d}-{len(parts):04d}.bin"
                    target = staging / name
                    target.parent.mkdir(exist_ok=True)
                    target.write_bytes(data)
                    parts.append(name)
            large.append(dict(path=relative, parts=parts, sha256=digest.hexdigest()))
            file.unlink()
        (staging / "large-files.json").write_text(json.dumps(large, indent=2))
        (staging / "restore.py").write_text(RESTORE)
        archives = []
        archive = None
        payload = 0
        try:
            for file in sorted(p for p in staging.rglob("*") if p.is_file()):
                size = file.stat().st_size + 2 * len(str(file.relative_to(staging)).encode()) + 512
                if archive is None or payload + size > MAX_PAYLOAD:
                    if archive:
                        archive.close()
                    target = session / f"comparison-{len(archives) + 1:03d}.zip"
                    # Refuse overwriting a prior report accidentally.
                    archive = zipfile.ZipFile(target, "x", compression=zipfile.ZIP_DEFLATED)
                    archives.append(target)
                    payload = 0
                archive.write(file, file.relative_to(staging))
                payload += size
        finally:
            if archive:
                archive.close()
        for archive in archives:
            if archive.stat().st_size >= 25_000_000:
                raise ValueError(f"Archive exceeds GitHub limit: {archive}")
            print(f"Share {archive} ({archive.stat().st_size / 1_000_000:.1f} MB)")
        return archives


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("world", nargs="?", help="saved world to clone (omit to create a new world in an empty private session)")
    parser.add_argument("--userdir", type=Path, default=ROOT, help="source user directory")
    parser.add_argument("--configdir", type=Path, help="source configuration directory (default: USERDIR/config)")
    parser.add_argument("--output", type=Path, help="new private session directory (must not exist)")
    parser.add_argument("--binary", type=Path, default=ROOT / "cataclysm-bn-tiles")
    parser.add_argument("--interval", type=int, default=30, help="minimum seconds between action-boundary captures")
    parser.add_argument("--count", type=int, default=5, help="maximum checkpoints")
    parser.add_argument("--package", type=Path, help="package an existing session without starting the game")
    args = parser.parse_args()
    if args.package:
        package(args.package.resolve())
        return
    if not 1 <= args.count <= 100 or not 1 <= args.interval <= 86400:
        parser.error("count must be 1..100; interval must be 1..86400")
    if args.world and (Path(args.world).name != args.world or args.world in (".", "..")):
        parser.error("world must be a single directory name")
    if args.configdir is not None and not args.configdir.is_dir():
        parser.error(f"configuration directory does not exist: {args.configdir}")
    binary = args.binary.resolve()
    if not binary.is_file():
        parser.error(f"build the tiles executable first: {binary}")
    for tileset in ("HitButton_iso", "UltimateCataclysm"):
        if not (ROOT / "gfx" / tileset / "tile_config.json").is_file():
            parser.error(f"missing {tileset}; restore the shipped BN gfx/{tileset} directory")
    env = os.environ.copy()
    # A comparison requires SDL_GPU even when the normal game uses a fallback renderer.
    env["SDL_RENDER_DRIVER"] = "gpu"
    for name in ("CATA_R3D_COMPARE", "CATA_R3D_QUIT_AFTER_SHOT", "CATA_R3D_SCREENSHOT",
                 "CATA_R3D_COMPARE_SESSION"):
        env.pop(name, None)
    version = preflight(binary, env)
    session = args.output.resolve() if args.output else Path(tempfile.mkdtemp(prefix="r3d-session-"))
    if args.output:
        session.mkdir(parents=True, exist_ok=False)
    user = session / "user"
    (user / "config").mkdir(parents=True)
    (session / "captures").mkdir()
    asset_snapshot = session / "art-assets"
    asset_snapshot.mkdir()
    for tileset in ("HitButton_iso", "UltimateCataclysm"):
        copy_tree(ROOT / "gfx" / tileset, asset_snapshot / tileset)
    shutil.copy2(ROOT / "data/raw/r3d/presentation_profiles.json", asset_snapshot / "presentation_profiles.json")
    source_config = args.configdir if args.configdir is not None else args.userdir / "config"
    for name in CONFIG_FILES:
        src = source_config / name
        if src.is_file() and not src.is_symlink():
            shutil.copy2(src, user / "config" / name)
    # The map hook is only drawn with tiles enabled. Change the isolated copy only.
    options_path = user / "config" / "options.json"
    options = json.loads(options_path.read_text()) if options_path.exists() else []
    options = [option for option in options if option.get("name") not in ("USE_TILES", "RENDERER")]
    options.extend([dict(name="USE_TILES", value="true"), dict(name="RENDERER", value="gpu")])
    options_path.write_text(json.dumps(options, indent=2))
    if args.world:
        source = args.userdir / "save" / args.world
        if not source.is_dir():
            parser.error(f"world does not exist: {source}")
        copy_tree(source, user / "save" / args.world)
    command = [str(binary), "--userdir", str(user) + os.sep,
               "--configdir", str(user / "config") + os.sep]
    if args.world:
        command += ["--world", args.world]
    env.update(CATA_R3D_COMPARE_SESSION=str(session / "captures"),
               CATA_R3D_COMPARE_INTERVAL=str(args.interval), CATA_R3D_COMPARE_COUNT=str(args.count),
               CATA_R3D_CAMERA="-45,30,20,1", CATA_R3D_COMPARE_TILESET="HitButton_iso",
               CATA_R3D_COMPARE_3D_TILESET="UltimateCataclysm")
    (session / "session.json").write_text(json.dumps(dict(
        command=command, interval=args.interval, count=args.count,
        source_world=args.world, private_userdir=str(user), build=version,
        render_driver=env["SDL_RENDER_DRIVER"], gpu_driver=env.get("SDL_GPU_DRIVER", "auto")), indent=2))
    print(f"Play normally; captures briefly pause at normal input boundaries.\n"
          f"Your session and saves persist at {session}\n"
          "Load or create a character to start captures. Quit normally to package the report.", flush=True)
    if not args.world:
        print("This private session starts without existing worlds. To use an existing save, "
              "pass its world name and --userdir if needed.", flush=True)
    previous = None
    with (session / "stdout.log").open("w") as log:
        with subprocess.Popen(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT) as game:
            while game.poll() is None:
                status_file = session / "captures" / "session-status.txt"
                status = status_file.read_text(errors="replace").strip() if status_file.exists() else ""
                if status and status != previous:
                    print(status, flush=True)
                    previous = status
                time.sleep(0.5)
    print(f"Game exited with status {game.returncode}. Private saves remain at {user}")
    package(session)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        sys.exit(str(error))
