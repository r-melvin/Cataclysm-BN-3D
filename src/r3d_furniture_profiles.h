#pragma once
#ifndef CATA_SRC_R3D_FURNITURE_PROFILES_H
#define CATA_SRC_R3D_FURNITURE_PROFILES_H

#include <algorithm>
#include <array>

#include "r3d_shapes.h"

namespace r3d
{

/// Local width, depth and height, with a normalized contact point. Zero sizes retain inference.
struct furniture_proportions {
    std::array<float, 3> size{};
    std::array<float, 3> anchor{ 0.5f, 0.5f, 0.0f };
};

struct furniture_profile_bounds_options {
    box_layout fitted;
    const furniture_proportions &profile;
    int facing = 0;
    bool flip = false;
    uint8_t wall_contacts = 0;
};

/// Override selected axes after native row/depth fitting. Authored spans stay exact; wall contact
/// translates them instead of stretching. On opposite contacts, prefer the furniture's back.
inline auto furniture_profile_bounds( const furniture_profile_bounds_options &options ) ->
box_layout
{
    auto bounds = options.fitted;
    auto size = options.profile.size;
    auto anchor = options.profile.anchor;
    if( options.flip ) { anchor[0] = 1.0f - anchor[0]; }
    for( auto turn = 0; turn < ( options.facing & 3 ); ++turn ) {
        std::swap( size[0], size[1] );
        const auto x = anchor[0];
        anchor[0] = 1.0f - anchor[1];
        anchor[1] = x;
    }
    const auto along_x = ( options.facing & 1 ) != 0;
    const auto back_positive = ( options.facing & 3 ) == 1 || ( options.facing & 3 ) == 2;
    const auto fit_axis = [&]( float & low, float & high, const size_t axis ) {
        if( size[axis] == 0.0f && anchor[axis] == 0.5f ) { return; }
        const auto span = size[axis] > 0.0f ? size[axis] : high - low;
        low = 0.5f - anchor[axis] * span;
        high = low + span;
        const auto negative = axis == 0 ? face_west : face_north;
        const auto positive = axis == 0 ? face_east : face_south;
        const auto touches_negative = ( options.wall_contacts & negative ) != 0;
        const auto touches_positive = ( options.wall_contacts & positive ) != 0;
        if( touches_negative || touches_positive ) {
            constexpr auto gap = 0.5f - wall_half_thickness;
            const auto use_positive = touches_positive &&
                                      ( !touches_negative || ( ( axis == 0 ) == along_x && back_positive ) );
            low = use_positive ? 1.0f + gap - span : -gap;
            high = low + span;
        }
    };
    fit_axis( bounds.x0, bounds.x1, 0 );
    fit_axis( bounds.y0, bounds.y1, 1 );
    return bounds;
}

struct furniture_profile_vertical {
    float base_offset;
    float top;
};

/// Keep the authored contact point on the floor, including when inferred height is retained.
inline auto furniture_profile_height( const furniture_proportions &profile,
                                      const float inferred_top ) -> furniture_profile_vertical
{
    const auto height = profile.size[2] > 0.0f ? profile.size[2] : inferred_top - floor_height;
    const auto base = -profile.anchor[2] * height;
    return { .base_offset = base, .top = floor_height + base + height };
}

} // namespace r3d
#endif
