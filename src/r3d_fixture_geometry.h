#pragma once
#ifndef CATA_SRC_R3D_FIXTURE_GEOMETRY_H
#define CATA_SRC_R3D_FIXTURE_GEOMETRY_H

#include <vector>
#include "r3d_prop_geometry.h"
#include "r3d_shapes.h"

struct SDL_Renderer;

namespace r3d
{
struct sprite_ref;
class quad_sink;
class gpu;
enum class fixture_finish { enamel, steel, dark, glass, cabinet, red, white };
enum class fixture_detail { body, brace, leg };
struct fixture_piece {
    prop_cuboid box;
    fixture_finish finish = fixture_finish::steel;
    fixture_detail detail = fixture_detail::body;
};
/// One continuous braced support, including a tapered crown for standalone markers.
auto radio_tower_pieces( float height ) -> std::vector<fixture_piece>;

#if defined(TILES)
/// Owned neutral palette for model surfaces, independent of a sprite's baked perspective.
auto make_fixture_palette( SDL_Renderer *renderer, gpu *device = nullptr ) -> sprite_ref;
struct emit_fixture_options {
    quad_sink &sink;
    const sprite_ref &palette;
    const std::vector<fixture_piece> &pieces;
    box_layout bounds;
    int x;
    int y;
    float base;
    float height = 1.0f;
    int facing = 0;
};
auto emit_fixture( const emit_fixture_options &options ) -> void;
#endif
} // namespace r3d
#endif
