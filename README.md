> **Experimental 3D fork:** this repository adds an optional 3D renderer to
> Cataclysm: Bright Nights. Gameplay rules remain controlled by BN. Start a BN
> world; CDDA saves and mods are not compatible migration inputs.

## Changes in this fork

- Optional GPU 3D view with camera-relative WASD movement, orbit/pan/zoom controls,
  configurable thin walls and cutaways, and multiple visible storeys.
- Wall faces, window surrounds, mounted boards, fitted shallow lockers, upright
  loose items, natural objects, and vehicles rendered in 3D.
- Optional alpha-silhouette voxel models for loose items; enable them in
  **Options → Graphics** after enabling the 3D renderer. Supported transparent
  sprites keep their pixel silhouette, including corners and notches.
- Vehicle wheels use faceted 3D geometry.
- Authored `.vox` item models can replace sprite-derived volumes when present in
  the renderer's item asset directory. Models and profiles prepare off-thread,
  reload with the tileset, and retain per-file fallbacks for invalid assets.
- Automated comparisons while playing: a private save copy records a 2D reference,
  eight 3D angles, current saves, settings, and diagnostic logs. Reports are split
  into ZIPs below GitHub's attachment limit. The launcher checks executable/shader
  compatibility and reports capture progress and failures.
- Simple fork issue forms: describe the problem and attach a report if available.

See the [3D build, controls, and capture guide](docs/en/dev/guides/renderer_3d.md).
The renderer is experimental. Linux builds and capture reports have been tested;
native macOS and Windows execution has not yet been verified. The UI overhaul,
first-person camera, and guided onboarding are separate feature ports in progress,
not features included in this main branch yet.
Current remaining work, branch integration and deferred ideas are tracked in the
[project backlog](BACKLOG.md); research documents retain their original evidence.

## Local testing and tools

After installing the dependencies in the [CMake build guide](docs/en/dev/guides/building/cmake.md),
the Linux developer checks are:

```sh
cmake --preset linux-full -DLINKER=lld
cmake --build --preset linux-full --target cataclysm-bn-tiles cata_test-tiles -j8
CATA_TEST_COMPUTE_ACCELERATION=cpu ./out/build/linux-full/tests/cata_test-tiles '[r3d]'
python3 -m unittest discover -s tools/r3d_spike -p test_compare_session.py
python3 tools/r3d_spike/compare_session.py --help
```

For headless Linux tests, also set `SDL_VIDEODRIVER=dummy`. The CPU compute setting
avoids requiring an SDL GPU compute device for tests; it does not validate the
rendered GPU image. This fork uses local checks rather than paid GitHub CI.
Run interactive game smoke tests in a virtual window (`xvfb-run -a` on Linux),
with a private `--userdir` and `--configdir`. A software Vulkan installation and
`--gpu-backend software` allow GPU image checks without a physical display.

The renderer tests include 17 seeded geometry snapshots. For an intentional
geometry change, regenerate them with `R3D_UPDATE_GEOMETRY_SNAPSHOT=1` on the test
command, review the differences and name the affected scenes in the commit.
Snapshots cover mesh positions/light, not textures, UVs or shaders. Use the
[comparison guide](tools/r3d_spike/COMPARE.md) for real gameplay captures and ZIP
reports; those are still needed to judge appearance.

Format only touched files with `bash build-scripts/format-cpp.sh <files>`.
The helper uses astyle 3.4.7 for top-level C++ because 3.6.x mis-indents
trailing-return functions; tests and nested C++ use clang-format. Install
[uv](https://docs.astral.sh/uv/) for the pinned astyle fallback, or install
`astyle-py==1.1.0` in your Python environment. A native astyle 3.4.7 also works.
The CMake `format` target uses the same selection. See [AGENTS.md](AGENTS.md).

The separate [Go asset CLI](https://github.com/r-melvin/Cataclysm-BN-3D-Asset-Pipeline)
packs directional sprites, voxelizes directly to `.vox`, validates models and
optionally exports existing Kenney Shape projects through MelonLoader. Its
README covers installation and `--help`. BN consumes the exported models;
Kenney Shape and MelonLoader are authoring tools installed separately.

## LLM disclosure

LLMs have been used to generate and adapt code in this fork and may also be used
to generate or edit some artwork. Generated contributions can contain mistakes
and need human review and testing. This disclosure concerns this fork's additions;
it does not imply that upstream BN code or existing tilesets were LLM-generated.
Existing licenses, credits, and asset attribution continue to apply. Any generated
art added to the project should be identified with its asset-specific provenance.

# Cataclysm: Bright Nights

<header align="center">
  <a><img src="docs/en/contribute/img/readme-title.png" title="screenshots of (clockwise from upper-right: Chaosvolt (x2), ExecutorBill, scarf005"></a>

[![en][icon-en]][en] [![ko][icon-ko]][ko] [![ja][icon-ja]][ja]

</header>

[en]: ./README.md
[icon-en]: https://img.shields.io/badge/lang-en-red?style=flat-square
[ko]: ./README.ko.md
[icon-ko]: https://img.shields.io/badge/lang-ko-orange?style=flat-square
[ja]: ./README.ja.md
[icon-ja]: https://img.shields.io/badge/lang-ja-green?style=flat-square

Cataclysm: Bright Nights is a roguelike with sci-fi elements set in a post-apocalyptic world.

While some have described it as a "zombie game", there is far more to Cataclysm than that. Struggle
to survive in a harsh, persistent, procedurally generated world. Scavenge the remnants of a dead
civilization for food, equipment, or, if you are lucky, a vehicle with a full tank of gas to get you
the hell out of there.

Fight to defeat or escape from a wide variety of powerful monstrosities, from zombies to giant
insects to killer robots and things far stranger and deadlier, and against the others like yourself,
who want what you have.

Find a way to stop the Cataclysm ... or become one of its strongest monsters.

> Cataclysm: Bright Nights is a fork of Cataclysm: Dark Days Ahead.
> [see the differences from its ancestor.](https://docs.cataclysmbn.org/game/changelog/).

## Downloads

### Executables

[![Stable][stable-releases-badge]][stable-releases] [![Flatpak][flathub-badge]][flathub-releases]

[![Recent][all-releases-badge]][all-releases] [![Experiemental][experimental-badge]][experimental-releases]

### Launchers

We do not support any launchers, none of them are maintained by any members of the development team and the vast majority of them do not keep up with our stable release cadence.

### Third Party Mods

Bright Nights uses a [mod registry](https://mods.cataclysmbn.org/) for easier discovery of mods not shipped with the main game.

### Source Code

[![Source Code][source-badge]][source] [![Zip Archive][clone-badge]][clone]

[stable-releases]: https://github.com/cataclysmbn/Cataclysm-BN/releases/latest "Download stable executable"
[stable-releases-badge]: https://img.shields.io/github/v/release/cataclysmbn/Cataclysm-BN?style=for-the-badge&color=success&label=stable
[all-releases]: https://github.com/cataclysmbn/Cataclysm-BN/releases?q=prerelease%3Atrue&expanded=true
[all-releases-badge]: https://img.shields.io/github/v/release/cataclysmbn/Cataclysm-BN?style=for-the-badge&color=important&label=Latest%20Release&include_prereleases&sort=date
[experimental-releases]: https://github.com/cataclysmbn/Cataclysm-BN/releases/tag/experimental
[experimental-badge]: https://img.shields.io/github/v/release/cataclysmbn/Cataclysm-BN?style=for-the-badge&color=salmon&label=Experimental%20Release&include_prereleases&sort=date
[flathub-releases]: https://flathub.org/apps/org.cataclysmbn.CataclysmBN
[flathub-badge]: https://img.shields.io/flathub/v/org.cataclysmbn.CataclysmBN?style=for-the-badge&color=success
[source]: https://github.com/cataclysmbn/Cataclysm-BN/archive/master.zip "The source can be downloaded as a .zip archive"
[source-badge]: https://img.shields.io/badge/Zip%20Archive-black?style=for-the-badge&logo=github
[clone]: https://github.com/cataclysmbn/Cataclysm-BN/ "clone from our GitHub repo"
[clone-badge]: https://img.shields.io/badge/Clone%20From%20Repo-black?style=for-the-badge&logo=github

#### Building from source

Please read the official docs for details:

- [building with cmake](docs/en/dev/guides/building/cmake.md)
- [building with MSYS2](docs/en/dev/guides/building/msys.md)
- [building with vcpkg](docs/en/dev/guides/building/vs_vcpkg.md)

## Contributing

> Cataclysm: Bright Nights developed under Creative Commons Attribution ShareAlike 3.0 license. The
> code and content of the game is free to use, modify, and redistribute for any purpose whatsoever.
> See http://creativecommons.org/licenses/by-sa/3.0/ for details. Some code distributed with the
> project is not part of the project and is released under different software licenses, the files
> covered by different software licenses have their own license notices.

Please check the [official docs](https://docs.cataclysmbn.org/contribute/contributing/) for
details.

## Documentation

Gameplay and developing documentation is available in the [doc](./docs/) directory
in markdown format. You can also

- visit the [official docs](https://docs.cataclysmbn.org/) site
- [build and serve the documentation locally](./docs/en/contribute/docs.md)

## Community

[![Official Docs](https://img.shields.io/badge/Docs-LightGray?style=for-the-badge)][docs]
[![Discussions](https://img.shields.io/badge/Discussions-black?style=for-the-badge&logo=github)][discussion]
[![Discord](https://img.shields.io/discord/830879262763909202?style=for-the-badge&logo=discord)][discord]
[![Discussions](https://img.shields.io/badge/CDDA%20Modding-green?style=for-the-badge&logo=discord)][modding]

[discussion]: https://github.com/cataclysmbn/Cataclysm-BN/discussions
[discord]: https://discord.gg/XW7XhXuZ89
[modding]: https://discord.gg/B5q4XCa "Unofficial DDA modding community discord has a BN channel"
[docs]: https://docs.cataclysmbn.org "Official BN documentation"

## Frequently Asked Questions

#### Is there a tutorial?

Yes, you can find the tutorial in the **Special** menu at the main menu (be aware that due to many
code changes the tutorial may not function). You can also access documentation in-game via the `?`
key.

#### How can I change the key bindings?

Press the `?` key, followed by the `1` key to see the full list of key commands. Press the `+` key
to add a key binding, select which action with the corresponding letter key `a-w`, and then the key
you wish to assign to that action.

#### How can I start a new world?

**World** on the main menu will generate a fresh world for you. Select **Create World**.

#### There is no music (or sound) in the game. How can I add it?

See where to place 3rd party mods, but replace mods/ with sounds/, this is the folder you will put it into

Find a soundpack such as the recommended one [Otopack](https://github.com/NarandBD/Otopack-BN-Mk-2).

Unzip the zip into the noted folder

Then finally select it from the settings, and then restart the game.

#### Where should I put 3rd-party mods?

If not in the List of common directories or it fails to work:

First boot up the game, select help, click 6 ( resolved game directories )

Place mods in the folder after "user mods: "

Otherwise:

For Windows users it is wherever your game folder is ( same level as the EXE ) /mods

For Android users using the non-legacy storage, the user mods folder is `Documents/Cataclysm-BN/mods`

For Linux users using the XDG directories (but NOT the flatpak): The user mods directory should be in `~/.local/share/cataclysm-bn/mods` (`~/.local/share/cataclysm-bn` is the user directory in general)

For flatpak users, the user mods folder is `~/.var/app/org.cataclysmbn.CataclysmBN/data/cataclysm-bn/mods` (the user directory in general is `~/.var/app/org.cataclysmbn.CataclysmBN/data/cataclysm-bn/`)

#### How do I update the game manually?

Assuming you've managed your mods appropriately, the correct update process is to delete the old data folder (alongside the gfx folder if you want to be extra safe) and _then_ overwrite the contents of the old BN folder with the new BN download. Deleting the old data folder is specifically necessary due to the fact that simply overwriting the old folder will **not** account for updates which delete files, as may happen with the obsoletion folder for example.

Don't delete any of the userdata folders!

Userdata folders are found by selecting help from the launch menu and clicking 6 ( resolved game directory )

#### I've found a bug. What should I do?

[Bug report](https://github.com/cataclysmbn/Cataclysm-BN/issues/new?template=bug_report.yml) can
be submitted via debug menu.

Run `Submit a bug report on github` inside the game to submit an issue.

|   1. open Options (ESC) -> Debug Menu (a)   |              2. open Info (i)               |
| :-----------------------------------------: | :-----------------------------------------: |
| ![](docs/en/contribute/img/readme-bug1.png) | ![](docs/en/contribute/img/readme-bug2.png) |
|    3. Submit a bug report on github (U)     |      4. An link to issue is generated       |
| ![](docs/en/contribute/img/readme-bug3.png) | ![](docs/en/contribute/img/readme-bug4.png) |

It will open a bug report on browser with `Version and configuration` filled in.

#### I would like to make a suggestion. What should I do?

- For simple ideas: please visit
  [our Discussions page](https://github.com/cataclysmbn/Cataclysm-BN/discussions/categories/ideas).
  It could be a new feature, a port request, a mod idea, or anything else.
- Please submit an issue on
  [our GitHub page](https://github.com/cataclysmbn/Cataclysm-BN/issues/) using
  [feature request form](https://github.com/cataclysmbn/Cataclysm-BN/issues/new?template=feature_request.yml).
