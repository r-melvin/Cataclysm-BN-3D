#pragma once
#ifndef CATA_SRC_R3D_VEHICLE_DETAILS_H
#define CATA_SRC_R3D_VEHICLE_DETAILS_H

#include "r3d_fixture_geometry.h"

namespace r3d
{
struct rotor_geometry_options {
    float diameter;
    int blades = 2;
    bool broken = false;
};
/// Rotor radius is in native metres/tiles, not the footprint of its mounting sprite.
auto rotor_pieces( const rotor_geometry_options &options ) -> std::vector<fixture_piece>;
/// A shared cabin crown keeps neighbouring roof cells joined while softening the silhouette.
auto roof_crown( float normalized_right ) -> float;
}
#endif
