#pragma once
#ifndef CATA_SRC_R3D_WALL_FURNITURE_H
#define CATA_SRC_R3D_WALL_FURNITURE_H

#include <array>
#include <utility>

#include "r3d_furniture_profiles.h"

namespace r3d
{

struct wall_furniture_options {
    box_layout bounds;
    const furniture_proportions &profile;
    int facing = 0;
    uint8_t contacts = 0;
    /// North, south, west, east distances from the furniture tile to each wall face.
    std::array<float, 4> gaps{};
};

/// Continue native back fitting to displaced wall faces; authored dimensions remain exact.
inline auto fit_furniture_to_wall_edges( const wall_furniture_options &options ) -> box_layout
{
    auto bounds = options.bounds;
    auto size = options.profile.size;
    auto anchor = options.profile.anchor;
    if( ( options.facing & 1 ) != 0 ) {
        std::swap( size[0], size[1] );
        std::swap( anchor[0], anchor[1] );
    }
    const auto fit_axis = [&]( float & low, float & high, const int axis ) {
        const auto negative = axis == 0 ? face_west : face_north;
        const auto positive = axis == 0 ? face_east : face_south;
        const auto touches_negative = ( options.contacts & negative ) != 0;
        const auto touches_positive = ( options.contacts & positive ) != 0;
        constexpr auto old_gap = 0.5f - wall_half_thickness;
        const auto negative_delta = touches_negative ? options.gaps[axis == 0 ? 2 : 0] - old_gap : 0.0f;
        const auto positive_delta = touches_positive ? options.gaps[axis == 0 ? 3 : 1] - old_gap : 0.0f;
        if( size[axis] > 0.0f || anchor[axis] != 0.5f ) {
            const auto along_x = ( options.facing & 1 ) != 0;
            const auto back_positive = ( options.facing & 3 ) == 1 || ( options.facing & 3 ) == 2;
            const auto use_positive = touches_positive &&
                                      ( !touches_negative || ( ( axis == 0 ) == along_x && back_positive ) );
            const auto shift = use_positive ? positive_delta : -negative_delta;
            low += shift;
            high += shift;
        } else {
            low -= negative_delta;
            high += positive_delta;
        }
    };
    fit_axis( bounds.x0, bounds.x1, 0 );
    fit_axis( bounds.y0, bounds.y1, 1 );
    return bounds;
}

} // namespace r3d
#endif // CATA_SRC_R3D_WALL_FURNITURE_H
