# Text scaling, high-DPI, and UI layout for SDL3 games

Research note. Sources were fetched or searched in this session. Anything not backed by a fetched or searched source is marked **UNVERIFIED** or listed under "Could not verify".

## 1. SDL3 high-DPI model

| Item | Verified fact | Source |
|---|---|---|
| High-DPI is always on in SDL3 | "high DPI support is always enabled" (the high-DPI disable hint was removed) | https://wiki.libsdl.org/SDL3/README-migration |
| `SDL_GetWindowDisplayScale` | `float SDL_GetWindowDisplayScale(SDL_Window *window);`. Described as "a combination of the window pixel density and the display content scale", i.e. the scale at which content should be drawn. Follows the OS scale setting and updates when the window moves to a display with a different setting. Returns 0.0f on failure. Main thread only. Since 3.2.0. | https://wiki.libsdl.org/SDL3/SDL_GetWindowDisplayScale |
| `SDL_GetWindowPixelDensity` | `float SDL_GetWindowPixelDensity(SDL_Window *window);`. "This is a ratio of pixel size to window size." Example: 1920x1080 window with a 3840x2160 back buffer gives 2.0. Returns 0.0f on failure. | https://wiki.libsdl.org/SDL3/SDL_GetWindowPixelDensity |
| `SDL_WINDOW_HIGH_PIXEL_DENSITY` | "window uses high pixel density back buffer if possible". The page does not say how it relates to the two functions above. **UNVERIFIED**: from memory, without this flag the back buffer is window-sized and pixel density is 1.0. Check before relying on it. | https://wiki.libsdl.org/SDL3/SDL_WindowFlags |
| Pixel size query | `SDL_GetWindowSizeInPixels()` replaces `SDL_Metal_GetDrawableSize()`. `SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED` tracks back-buffer size changes; `SDL_EVENT_WINDOW_SIZE_CHANGED` was removed. | https://wiki.libsdl.org/SDL3/README-migration |
| Mouse coordinates | Mouse events and several query/warp functions use floats for sub-pixel precision. The renderer no longer rewrites mouse/touch coordinates or scales them automatically. | https://wiki.libsdl.org/SDL3/README-migration |
| Event to render coordinates | `bool SDL_ConvertEventToRenderCoordinates(SDL_Renderer *renderer, SDL_Event *event);`. "Convert the coordinates in an event to render coordinates." | https://wiki.libsdl.org/SDL3/SDL_ConvertEventToRenderCoordinates |
| Safe area | `bool SDL_GetWindowSafeArea(SDL_Window *window, SDL_Rect *rect);`. Area "which is safe to have interactable content". Obscured areas come from on-screen controls, curved edges, notches, and TV overscan (the examples are mine, not the page's). Keep rendering across the whole window, but keep important or interactive content inside this rect. | https://wiki.libsdl.org/SDL3/SDL_GetWindowSafeArea |

### Logical presentation

`bool SDL_SetRenderLogicalPresentation(SDL_Renderer *renderer, int w, int h, SDL_RendererLogicalPresentation mode);`. Verified: "You can disable logical coordinates by setting the mode to SDL_LOGICAL_PRESENTATION_DISABLED, and in that case you get the full pixel resolution of the render target." Related: `SDL_GetRenderLogicalPresentation`, `SDL_GetRenderLogicalPresentationRect`.

| Mode | Documented behaviour | Trade-off for a text UI |
|---|---|---|
| `SDL_LOGICAL_PRESENTATION_DISABLED` | "There is no logical size in effect" | Full pixel resolution. Text is laid out in real pixels. Best fidelity for font rendering. |
| `SDL_LOGICAL_PRESENTATION_STRETCH` | "The rendered content is stretched to the output resolution" | Distorts glyphs at any aspect mismatch. Poor for text. |
| `SDL_LOGICAL_PRESENTATION_LETTERBOX` | "fit to the largest dimension and the other dimension is letterboxed with the clear color" | Keeps aspect. Costs screen area on 21:9 and 4:3. Text stays undistorted but is resampled unless the scale is integral. |
| `SDL_LOGICAL_PRESENTATION_OVERSCAN` | "fit to the smallest dimension and the other dimension extends beyond the output bounds" | Fills the screen. Content at the edges is cut off, so the UI must respect the safe area. |
| `SDL_LOGICAL_PRESENTATION_INTEGER_SCALE` | "scaled up by integer multiples to fit the output resolution" | Crisp, but steps are large (2x, 3x), and the output can be much smaller than the window. |

Sources: https://wiki.libsdl.org/SDL3/SDL_RendererLogicalPresentation and https://wiki.libsdl.org/SDL3/SDL_SetRenderLogicalPresentation.

Implication for a custom UI: a logical size (for example 1280x800) lets you write layouts once, but it makes font sizes scale with the window. A UI that needs a fixed text scale should use `DISABLED` and compute the layout in device pixels, scaling by `SDL_GetWindowDisplayScale` itself.

## 2. SDL_ttf 3

| Item | Verified fact | Source |
|---|---|---|
| Renderer text engine | `TTF_TextEngine * TTF_CreateRendererTextEngine(SDL_Renderer *renderer);`. "Create a text engine for drawing text on an SDL renderer." | https://wiki.libsdl.org/SDL3_ttf/TTF_CreateRendererTextEngine |
| GPU text engine | `TTF_TextEngine * TTF_CreateGPUTextEngine(SDL_GPUDevice *device);`. Should be called on the thread that created the device. Since 3.0.0. A `...WithProperties` variant takes an atlas texture size. `TTF_GetGPUTextDrawData` returns vertex data for your own GPU drawing. | https://wiki.libsdl.org/SDL3_ttf/TTF_CreateGPUTextEngine, https://www.mankier.com/3/TTF_CreateGPUTextEngineWithProperties (search result, not fetched) |
| SDF | `bool TTF_SetFontSDF(TTF_Font *font, bool enabled);`. "SDF is a technique that helps fonts look sharp even when scaling and rotating," and "requires special shader support for display." Works with Blended APIs; raw signed distance values go in the alpha channel. Changing it clears the glyph cache. The page does not name the shaders or engines that support it. | https://wiki.libsdl.org/SDL3_ttf/TTF_SetFontSDF |
| Hinting | `void TTF_SetFontHinting(TTF_Font *font, TTF_HintingFlags hinting);`. Values: `TTF_HINTING_NORMAL`, `TTF_HINTING_LIGHT`, `TTF_HINTING_MONO`, `TTF_HINTING_NONE`, `TTF_HINTING_LIGHT_SUBPIXEL` (3.0.0+). The page does not describe each value. | https://wiki.libsdl.org/SDL3_ttf/TTF_SetFontHinting |
| DPI-aware size | `bool TTF_SetFontSizeDPI(TTF_Font *font, float ptsize, int hdpi, int vdpi);`. Changing it clears the glyph cache. The page does not explain how DPI changes the rasterised size. | https://wiki.libsdl.org/SDL3_ttf/TTF_SetFontSizeDPI |
| Fallback fonts | `bool TTF_AddFallbackFont(TTF_Font *font, TTF_Font *fallback);`. Supplies glyphs the primary lacks. Fallbacks "used in the order added". Both fonts should share size and style. | https://wiki.libsdl.org/SDL3_ttf/TTF_AddFallbackFont |
| Emoji / CJK | Fallback chaining is the documented mechanism. **UNVERIFIED**: whether SDL_ttf renders colour emoji, and which CJK fonts to ship. Not searched. | - |

Implication: SDF is a shader-dependent option. With the renderer engine, scale glyphs at the target size instead, and use SDF only if you own a shader path. CJK support needs a fallback font with CJK coverage added via `TTF_AddFallbackFont`.

## 3. Game UI-scale ranges

| Game | Range / control | Verified? | Source |
|---|---|---|---|
| World of Warcraft | Checkbox plus slider. Minimum 0.64. The wiki says 1.0 scales widgets "as if the screen were 768 units tall". Default when unchecked is chosen by the game (0.64 to 1.00 Retail, 0.90 to 1.00 Classic). Menu location differs between sources. | Yes (wiki), upper bound implied | https://warcraft.wiki.gg/wiki/UI_Scale |
| Final Fantasy XIV | Forum user reports only 100% or 150% UI scale, which is too small or too large on 1440p. | Anecdotal (forum) | https://forum.square-enix.com/ffxiv/showthread.php?p=5950592 (search result) |
| Factorio | Fixed 25% steps. Typed values are rounded to 75 or 100 per a forum post. | Anecdotal | https://forums.factorio.com/viewtopic.php?p=476644 (search result) |
| Minecraft | Fixed presets (100%, 167% and so on). A feedback post asks for a continuous slider. | Anecdotal | https://feedback.minecraft.net/hc/en-us/community/posts/47926274708109-Custom-UI-Scale-Slider-in-Video-Settings (search result) |
| Caves of Qud | "Modern and Overlay Control Scale" dropdown with no upper limit (2020 Steam thread). Play-area scale: Fit, Cover, Pixel Perfect (2022 devlog); default changed from Fit to Cover. Tiles are 16x24. Text font is Source Code Pro. | Yes (dev/community), dated | https://steamcommunity.com/app/333640/discussions/0/2997669713998413653, https://freeholdgames.itch.io/cavesofqud/devlog/450719/feature-friday-november-11-2022, https://wiki.cavesofqud.com/wiki/Visual_Style |
| Dwarf Fortress (Steam, v50) | Interface-scale toggle that fits menus to a grid (default 170x64 cells). A percentage sets cell growth when grid-fit is off. Tilesets are 8x12 and 10x12 cells. Users report the interface stays small on 4K. | Yes (community wiki), v50 details unconfirmed | https://dwarffortresswiki.org/Settings, https://dwarffortresswiki.org/Tileset_repository |
| God of War (PC), Diablo IV | **Could not verify.** A HUD-scale control was not found for either. The God of War search found only a render-scale option (50-100%), which is not HUD scale. | No | - |

Pattern: a percentage slider with a floor near 0.6x to 0.7x and a ceiling near 1.5x to 2x is common (WoW shows 0.64 as the floor). Games with fixed steps (FFXIV, Factorio, Minecraft) get the most complaints.

## 4. Minimum sizes and distance rules

| Rule | Value | Source |
|---|---|---|
| Steam Deck: readable at distance | "Interface text must be easily readable at a distance of 12 inches/30 cm from the screen." | https://partner.steamgames.com/doc/steamdeck/compat |
| Steam Deck: absolute minimum | "the smallest on-screen font character should never fall below 9 pixels in height at 1280x800." 9px is the approval minimum. | same |
| Steam Deck: target | "we recommend aiming for 12px whenever possible." | same |
| Steam Deck: settings | "We recommend supporting user-configurable text size and, when possible, contrast." | same |
| Steam Machine | Display tests are not run because "resolution and text size could vary depending on the size and type of display being used." | same |
| 1080p / 1440p / 4K | **Could not verify** an official minimum. The Steam Deck figure is the only official number found. Convert it to a physical size instead (see derived note below). | - |
| TV / 10-foot distance | Microsoft gives no number. Its guidance says the ten-foot viewing distance makes small fonts hard to read and that text should be minimized. | https://msdn.microsoft.com/library/jj883727, https://learn.microsoft.com/pl-pl/windows/apps/design/devices/designing-for-tv |
| TV: third-party figure | "never be smaller than 24 pts" and "18px is probably the smallest readable size, and only for nonessential labels". Third-party, not official. | https://www.toptal.com/designers/ui/tv-ui-design |
| TV: viewing distance | Roblox: players "typically sitting 8–10 feet away". Recommends relative sizes over fixed pixels. | https://create.roblox.com/docs/en-us/production/publishing/console-guidelines |
| Amazon Fire TV | Elements large and spaced for reading from a distance; minimise text. | https://developer.amazon.com/docs/fire-tv/design-and-user-experience-guidelines.html |
| BDA (dyslexia) body text | Sans-serif at 12 pt or larger; some versions say 12-14 pt. Secondary sources only. | search results, not the BDA PDF |

Derived (my arithmetic, not sourced): the Deck's 9px at 800 px tall is about 1.1% of screen height. At 30 cm and a 7-inch 1280x800 panel, 12px is about 2.3 mm tall. A 1080p monitor at arm's length needs a different pixel count for the same visual size. Use a physical-size target (mm or degrees of visual angle) rather than a pixel count across resolutions.

## 5. Scaling approaches for text-grid and custom UI games

- **Curses grids (CBN, Dwarf Fortress, Caves of Qud text mode):** The grid is the layout. Scaling means changing the cell size or the grid-fit rule. DF's grid-fit (170x64 default) and Qud's Fit/Cover/Pixel Perfect are both documented examples of the trade-off between readability and fitting the screen.
- **Tile sets with fixed art:** Scaling by integer multiples keeps the art crisp (Qud's Pixel Perfect). Aspect mismatch shows up as stretched UI (DF wiki: a tileset with a different aspect ratio stretches UI elements).
- **Immediate-mode GUI (ImGui):** See section 6.

## 6. ImGui font scaling

- Verified (GitHub issue, SDL3 backend): the SDL3 backend reports FontScaleDpi=2.0 on a high-DPI display and gives crisp text. The SDL2 backend reported 2.881 and blurry text. Source: search result citing https://upd.dev/ocornut/imgui/issues/8756 (SDL2 scale detection). Treat as a user report.
- Verified (issue #797 summary): text turned blurry under the built-in font scaling, even at 2x. Maintainers' guidance: a bitmap font stretched past its rendered size looks soft. Source: https://upd.dev/ocornut/imgui/issues/797 (search summary).
- Verified (ReShade forum): scaling only the overlay text leaves the shared atlas unchanged. Rebuilding the atlas at the target global font size fixes it. Source: https://www.reshade.me/forum/troubleshooting/8085-force-reshade-imgui-to-rebuild-font-atlas-for-given-size (search result).
- Conflicting advice: some guides point to `io.DisplayFramebufferScale`, others to `io.FontScaleDpi`. **UNVERIFIED** which is right for your ImGui version. Check the imgui.h and imgui_impl_sdl3 source.
- **Could not verify:** CDDA-specific ImGui blurry-font complaints. Searches turned up only an old CDDA forum thread on readability, with no ImGui mention. The likely cause is the same atlas-stretch problem above, but that is inference.

## 7. Layout approaches (C++ custom-drawn UI)

| Approach | Description | Pros | Cons |
|---|---|---|---|
| Anchor-based (Unity RectTransform) | Elements anchored to parent points (min/max anchors), with offsets. **UNVERIFIED from memory:** the docs fetch was redirected and not read. | Simple; scales with the parent; familiar to Unity users. | Anchors are hard to reason about for lists and wrapping text; needs a retained node tree. |
| Flexbox-style (Yoga) | Meta's embeddable C++ flexbox subset. Layout only, no drawing. MIT. Used by React Native, Qt Quick Layouts, ComponentKit. | Mature. Handles wrapping, alignment, and percentages. Fits a retained tree. | Pulls a dependency. Only a subset of CSS flexbox. Its C++ standard requirement differs between sources, so check the repo. |
| Flexbox-style (Clay) | Single-header C library by Nic Barker. Flexbox-like, immediate mode (rebuilt each frame), arena memory, no malloc. Emits render commands; you draw them. | Tiny; no dependencies; immediate mode is simple to host in an existing loop; scrolling and text wrapping included. | Young library. Its API is macro-based. You write the renderer. |
| Constraint-based | Elements placed by constraints (e.g. Cassowary). **UNVERIFIED:** not researched. | Expressive for resizing. | Solver cost and complexity. Usually more than a terminal-style game needs. |
| 9-slice panels | Corners fixed, edges and centre stretched. **UNVERIFIED:** not researched beyond general knowledge. | Keeps frame borders crisp at any panel size. | Needs art prepared for it. Does not solve text layout. |
| Safe area | `SDL_GetWindowSafeArea` (section 1). | Correct for TVs, notches, and overscan. | Only for the outer frame. Interior layout still needed. |
| Aspect-ratio handling | Logical presentation (section 1), or direct layout with pixel-based breakpoints. **UNVERIFIED:** 21:9 and 4:3 specifics not researched. | Simple with logical size. | Letterboxing wastes space on ultrawide. Overscan clips. |

Recommendation (my view, not sourced): for a custom C++ UI, a flexbox-style layer (Clay, if you want no dependencies and immediate mode; Yoga if you want a retained tree and a proven engine) on top of a text-measurement function is a reasonable start. Keep the font scale as a single user setting that feeds the measurement step. This avoids logical presentation for text.

## 8. Font choices

### OpenDyslexic

- Wery & Diliberto (2016), Annals of Dyslexia. Single-case alternating-treatment design, about a dozen children aged 9-12. Compared OpenDyslexic with Arial and Times New Roman. "no improvement in reading rate or accuracy for individual students with dyslexia, as well as the group as a whole." No student preferred it. Source: https://link.springer.com/article/10.1007/s11881-016-0127-1 (search result; the abstract was not fetched). The sample is too small to show no effect, only that no effect was found.
- An eye-tracking study (48 dyslexic readers, aged 11-50, per search summary): "OpenDyslexic did not lead to better or worse readability compared to standard fonts, and participants significantly preferred Verdana and Helvetica over OpenDyslexic." Same search summary: sans serif, monospaced, and roman styles beat serif, proportional, and italic. Source: https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5629233/ is a different OpenDyslexic study cited in search results; the 48-reader study's URL was not confirmed. Treat the figures as search-summary claims.
- A review found no evidence that special dyslexia fonts give statistically significant speed gains (search summary of a literature review; not located).
- Verdict: no good evidence that OpenDyslexic improves reading speed or accuracy. Comprehension results are mixed and the studies are small.

### Lexend

- A widely cited study reported a 19.8% reading-speed gain over Times New Roman. Google-financed. Commentators note it rests on one study of about 20 children. No peer-reviewed replication was found. Sources: https://teleprompter.com/blog/effectiveness-of-lexend-and-opendyslexic-fonts and https://typedrawers.com/discussion/comment/43203 (search results, not primary).
- Verdict: unconfirmed. Do not claim it as evidence-backed without the original paper.

### Atkinson Hyperlegible (Next)

- Designed by the Braille Institute for low-vision readers. Distinguishes similar glyphs (l, I, 1) by design. Developed with low-vision client feedback and legibility research. Source: https://en.wikipedia.org/wiki/Atkinson_Hyperlegible (search result).
- A 2024 CSUN session described a quantitative study with low-vision readers, but the results were not found. **Unverified** beyond design rationale.
- Atkinson Hyperlegible Next (2025): variable weight, wider character set, and a monospaced variant. Source: search summary of the Braille Institute release.

### Sans-serif guidance for body text

- BDA guidance (secondary sources): sans-serif such as Arial, Verdana, Tahoma, Century Gothic, Trebuchet, Calibri, Open Sans; 12 pt or larger. Not from the BDA PDF.

Recommendation: pick a sans-serif with clear glyph distinction (Atkinson Hyperlegible or Verdana/Helvetica-class) for body text. Do not rely on OpenDyslexic for legibility claims. Test with your own readers.

## 9. Could not verify

- SDL_WINDOW_HIGH_PIXEL_DENSITY's exact effect on back buffer size and its link to SDL_GetWindowPixelDensity. The fetched pages do not state it.
- Logical-presentation behaviour for 21:9 and 4:3 beyond the documented mode descriptions.
- SDL_ttf emoji (colour) support and CJK coverage specifics.
- SDF shader requirements and which engines support them.
- God of War and Diablo IV UI-scale ranges.
- Minimum sizes at 1080p, 1440p and 4K. Only the Steam Deck rule is official.
- Unity RectTransform anchor documentation (redirect not followed to content).
- Constraint-based layout and 9-slice details.
- CDDA's ImGui blurry-font complaints. Not found.
- Atkinson Hyperlegible low-vision study results (CSUN 2024).
- Lexend replication studies.
- BDA style guide primary text (font sizes differ between secondary sources: 12 pt vs 12-14 pt).
- Dwarf Fortress v50 interface-scale details beyond the community wiki.
- Caves of Qud's current menu names (sources are 2020-2022).
