#pragma once
#ifndef CATA_SRC_R3D_H
#define CATA_SRC_R3D_H

// Public entry points of the 3D renderer. No SDL types here so game code can include it in
// every build; without TILES everything is an inline no-op.

#if defined(TILES)
#include <optional>

#include  <string>
#include <vector>

#include "coordinates.h"

struct SDL_Renderer;
#endif

namespace r3d
{

#if defined(TILES)

/// A line of text the 3D view wants drawn over its last frame, in the map font.
struct hud_text {
    float x = 0.0f; // logical pixels, in the space of the rectangle given to draw()
    float y = 0.0f; // top of the text
    std::string text;
    int color = 7;  // curses colour index (0-15), as for overlay strings
    int align = 0;  // -1: starts at x, 0: centred on x, 1: ends at x
    bool boxed = false; // drawn on a dark box, for text over a busy picture
};

/// Texts to draw over the 3D frame the last draw() returned true for.
auto hud_texts() -> const std::vector<hud_text> &; // *NOPAD*
/// Size in logical pixels of a character of the font hud texts are drawn in.
auto set_text_cell( int width, int height ) -> void ;

/// Draws the map around center into the pixel rectangle (x, y, w, h) of the renderer's current
/// target, composited like any other 2D draw. Returns false when the 3D view cannot be drawn this
/// frame (option off, no GPU renderer, ...), in which case the caller draws the flat view instead.
/// render_epoch changes whenever the renderer or its textures were rebuilt.


/// Draws the map around center into the pixel rectangle (x, y, w, h) of the renderer's current
/// target, composited like any other 2D draw. Returns false when the 3D view cannot be drawn this
/// frame (option off, no GPU renderer, ...), in which case the caller draws the flat view instead.
/// render_epoch changes whenever the renderer or its textures were rebuilt.
struct draw_options {
    SDL_Renderer *renderer;
    float x;
    float y;
    float w;
    float h;
    const tripoint_bub_ms &center;
    unsigned int render_epoch;
};

auto draw( const draw_options &options ) -> bool;

/// Releases all GPU objects; call before the renderer goes away.
auto shutdown() -> void;

/// True when the option is on, the GPU renderer is available and the 3D scene is drawn.
auto active() -> bool;

/// The map tile under the point (x, y) of the renderer, in the same logical pixels as the
/// rectangle given to draw(), as seen in the last drawn 3D frame. Like the flat view's picks, the
/// result is on the map's own level (get_abs_sub().z()): something seen through a hole on a lower
/// level yields the tile where the mouse ray crossed the viewed level. Nothing when the point is
/// outside that frame, points at the sky, or the 3D view is not active.
auto pick( float x, float y ) -> std::optional<tripoint_bub_ms>;

/// 1/8 turns clockwise to apply to map directions so that "north" walks away from the 3D camera
/// (see camera::forward_eighths); 0 when the 3D view is not shown.
auto view_eighths() -> int;
/// True when the main game takes WASD for walking and the arrow keys for the 3D camera.
auto wasd_controls() -> bool;

/// Debug capture for CATA_R3D_COMPARE (see r3d.cpp): saves the 3D view and the flat isometric
/// view of the same turn once. CATA_R3D_COMPARE_SESSION enables bounded periodic checkpoints
/// with eight 3D yaw poses and current saves. Sessions require gameplay_boundary=true;
/// the generic input hook can still perform the legacy one-shot without saving.
auto compare_capture( bool gameplay_boundary ) -> void;

/// Flips the RENDERER_3D option.
auto toggle() -> void;
/// Orbit the camera by one step; dir < 0 is left, dir > 0 right.
auto rotate( int dir ) -> void;
/// Tilt the camera by one step; dir > 0 raises the view towards top-down.
auto pitch( int dir ) -> void;
/// Move the camera closer (dir > 0) or away (dir < 0).
auto zoom( int dir ) -> void;
/// Restore the default camera, centred on the view again.
auto reset() -> void;
/// True when (x, y), in the logical pixels given to draw(), lies on the last drawn 3D frame.
auto contains( float x, float y ) -> bool;
/// Free camera from a mouse drag of (dx, dy) pixels: orbit and tilt, or slide over the ground.
auto drag_orbit( float dx, float dy ) -> void;
auto drag_pan( float dx, float dy ) -> void;

#else

inline auto active() -> bool { return false; }
inline auto view_eighths() -> int { return 0; }
inline auto wasd_controls() -> bool { return false; }
inline auto compare_capture( bool /*gameplay_boundary*/ ) -> void {}
inline auto toggle() -> void {}
inline auto rotate( int /*direction*/ ) -> void {}
inline auto pitch( int /*direction*/ ) -> void {}
inline auto zoom( int /*direction*/ ) -> void {}
inline auto reset() -> void {}
inline auto contains( float /*x*/, float /*y*/ ) -> bool { return false; }
inline auto drag_orbit( float /*x*/, float /*y*/ ) -> void {}
inline auto drag_pan( float /*x*/, float /*y*/ ) -> void {}

#endif

} // namespace r3d

#endif // CATA_SRC_R3D_H
