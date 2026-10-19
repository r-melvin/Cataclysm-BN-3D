# Renderer comparison sessions

Build `cataclysm-bn-tiles`. BN ships the `HitButton_iso` and
`UltimateCataclysm` reference tilesets. Then run:

```sh
python3 tools/r3d_spike/compare_session.py 'My world' --output /tmp/my-renderer-report
# For saves outside this checkout, add --userdir /path/to/user-directory.
# If settings are elsewhere (e.g. XDG), also add --configdir /path/to/source-config.
# Omit the world to choose/create a new world in the private session.
```

Play normally and quit normally. The launcher does not synthesize keys, control
windows, or change desktop focus. It clones the chosen world and a small list of
configuration files into `/tmp/my-renderer-report/user`; your original saves are
untouched. **All progress during this session remains in that private user
directory.** Keep it if you want to continue that character. Launch the same
executable with `--userdir /tmp/my-renderer-report/user/ --configdir /tmp/my-renderer-report/user/config/`
to resume it. The launcher always passes both private paths, including on XDG builds.
It preserves allowlisted startup/backend, font and panel settings in the private
session and each checkpoint, alongside that checkpoint's current options.

The default is five checkpoints at least 30 seconds apart (`--count`, `--interval`).
A checkpoint happens when gameplay next asks for a normal player action. Menus,
activities and auto-movement do not trigger capture. Idle waiting does not cause
new checkpoints; take a normal action after the interval. Each batch briefly
pauses play while it saves, reloads tilesets, and draws screenshots. No game turn
or input is processed inside that batch; queued keys remain for gameplay. The
previous view, tileset, zoom, camera, pan and gameplay RNG are restored afterwards.

Each completed checkpoint includes a **fresh save of the captured state**, a map
dump, build/OS/graphics/language/mod diagnostics and the options active at capture time, one fixed isometric 2D reference,
and eight orthographic 3D poses at 45-degree yaw intervals and 30-degree pitch.
Isometric 2D has a fixed orientation; its reference is shared by all poses rather
than pretending that 2D can rotate. `view_before.png` and `view_resumed.png` record the player view before and after
the batch for restoration checks.
The packaged `art-gallery/index.html` pairs visible placements with their source
sprites from both shipped tilesets and links to the checkpoint's eight 3D poses
and 2D reference. Its `support-map.svg` shows placement origins and tile-ground
anchors in map coordinates. The door/window matrix lists open, closed and broken
sprites available in the shipped tilesets and marks the states actually present
in the checkpoint. Items with entries in
`data/raw/r3d/presentation_profiles.json` also show a scaled top footprint and
side elevation with their support point, plus material-region names and VOX
palette slots. The profile file is bundled with the tileset sources so gallery
values match the renderer build. Separate gameplay checkpoints are needed to compare rendered
geometry for states absent from this scene.
No complete marker is written if saving or either renderer fails. A genuine disk-write
failure may show the normal game save error dialog; dismiss it normally. The
checkpoint is aborted and its older disk save is never packaged as current state.
A game blocked from saving after a failed load, or an unavailable checkpoint
destination, fails without packaging an older save. Capture saves skip Lua
`on_game_save` hooks so recording does not trigger mod gameplay effects.
Committed SQLite databases are snapshotted through SQLite backup, including WAL data.

The save reproduces the scene on the recorded build with the same mods and assets.
Custom mod/tileset assets are not bundled. `rng.txt` records the pre-capture random
engine for investigation; normal game loading does not restore that engine, so
future action sequences are not promised to replay deterministically. Animated
sprite phases can differ. Screenshots include the whole game window, including UI.

After quitting, numbered `comparison-001.zip`, etc. are created in the session
directory. Each is a normal ZIP below GitHub's 25 MB attachment limit. Upload all
of them to the issue. To reproduce, extract all ZIPs into one empty directory and
run `python3 restore.py` (reassembles any oversized files), then follow README.txt
to load a checkpoint in a new user directory (copy both its `save/` and `config/`,
and pass both `--userdir` and `--configdir` when launching). No saves are silently trimmed.
If the game crashes, keep the private directory and package completed checkpoints:

```sh
python3 tools/r3d_spike/compare_session.py --package /tmp/my-renderer-report
```

Packaging refuses to overwrite earlier ZIPs; move those aside before repackaging.
Checkpoints with failures are retained privately for diagnosis and excluded from
shareable archives. Inspect reports before sharing: the selected world's saves
include characters, notes, and explored map data, and logs may include local paths.
The package includes only completed checkpoint directories, session logs,
`options.json`, and reports; it does not sweep the checkout or other worlds.

BN's upstream bug template requests versions/configuration/mods and crash/debug
logs plus zipped saves. The fork's simple template accepts a plain description
and optional attachments. The generated report
provides the technical attachments; add your description, reproduction steps and
expected behavior. [GitHub's official attachment documentation](https://docs.github.com/en/get-started/writing-on-github/working-with-advanced-formatting/attaching-files)
(checked 2026-10-18) accepts `.zip`, `.gz`, `.tgz`, limits non-image files to 25 MB,
and separately limits images to 10 MB.

The older `compare_iso.sh` remains a one-shot capture-and-exit helper. For normal
play use `compare_session.py`. Direct `CATA_R3D_COMPARE_SESSION` environment use
writes saves to the active user directory; the launcher is the supported way to
keep originals isolated.

Capture sessions reject stale executables and missing compiled shaders before launch,
select GPU tiles in the private session, and print checkpoint progress. Rebuild
with the required `SHADER_TARGETS` (`spirv` for Vulkan, `msl` for Metal, `dxil`
for Direct3D 12). BN uses its native shadercross pipeline. Omitting the world
starts an empty private session; create a character and reach gameplay input.
Pass a world name and `--userdir` to clone an existing BN save. Failed sessions
print stage details and private diagnostic paths.
