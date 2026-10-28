#pragma once
#ifndef CATA_SRC_R3D_WALL_LAYOUT_H
#define CATA_SRC_R3D_WALL_LAYOUT_H

#include <array>

#include "r3d_shapes.h"

namespace r3d
{

struct wall_layout_options {
    /// Row-major 3 by 3 neighbourhood of actual indoor floors; centre is ignored.
    std::array<bool, 9> indoors{};
    float half_width = wall_half_thickness;
};

struct wall_layout {
    float center_x = 0.5f;
    float center_y = 0.5f;
    std::array<float, 4> x_edges{};
    std::array<float, 4> y_edges{};
};

/// A room-facing wall occupies the far edge of its tile. Shared/ambiguous walls consistently use the north/west edge.
inline auto exterior_wall_layout( const wall_layout_options &options ) -> wall_layout
{
    const auto &inside = options.indoors;
    const auto axis_center = [half = options.half_width]( bool negative, bool positive ) -> float {
        return negative && !positive ? 1.0f - half : half;
    };
    // Cardinal floors take precedence; diagonals identify convex/concave building corners.
    const auto horizontal = inside[3] || inside[5];
    const auto vertical = inside[1] || inside[7];
    const auto x = axis_center( horizontal ? inside[3] : inside[0] || inside[6],
                                horizontal ? inside[5] : inside[2] || inside[8] );
    const auto y = axis_center( vertical ? inside[1] : inside[0] || inside[2],
                                vertical ? inside[7] : inside[6] || inside[8] );
    const auto half = options.half_width;
    return { .center_x = x, .center_y = y,
             .x_edges = { 0.0f, x - half, x + half, 1.0f },
             .y_edges = { 0.0f, y - half, y + half, 1.0f } };
}

} // namespace r3d
#endif // CATA_SRC_R3D_WALL_LAYOUT_H
