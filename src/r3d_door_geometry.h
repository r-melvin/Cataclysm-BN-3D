#pragma once
#ifndef CATA_SRC_R3D_DOOR_GEOMETRY_H
#define CATA_SRC_R3D_DOOR_GEOMETRY_H

namespace r3d
{

constexpr auto door_jamb_width = 0.08f;

/// A swung leaf keeps the closed opening's width, even when it crosses a tile boundary.
constexpr auto open_door_tip( float hinge, int swing_direction ) -> float
{
    return hinge + swing_direction * ( 1.0f - 2.0f * door_jamb_width );
}

} // namespace r3d

#endif // CATA_SRC_R3D_DOOR_GEOMETRY_H
