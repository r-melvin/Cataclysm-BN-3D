#pragma once
#ifndef CATA_SRC_R3D_VEHICLE_CAMERA_H
#define CATA_SRC_R3D_VEHICLE_CAMERA_H

#include <cmath>

#include "units.h"

namespace r3d
{

/// Wraps an angle in radians into [-pi, pi].
inline auto wrap_angle( float radians ) -> float
{
    constexpr auto pi = static_cast<float>( M_PI );
    constexpr auto two_pi = static_cast<float>( 2.0 * M_PI );
    auto wrapped = std::fmod( radians, two_pi );
    if( wrapped > pi ) {
        wrapped -= two_pi;
    } else if( wrapped < -pi ) {
        wrapped += two_pi;
    }
    return wrapped;
}

/// Camera yaw looking along a vehicle heading: 270 deg (North) is yaw 0, 0 deg (East) is -pi/2.
inline auto vehicle_heading_to_yaw( units::angle face ) -> float
{ return wrap_angle( static_cast<float>( units::to_radians( 270_degrees - face ) ) ); }

/// First person camera yaw looking along a vehicle heading: 270 deg (North) is yaw 0, 0 deg (East) is pi/2.
inline auto vehicle_heading_to_fp_yaw( units::angle face ) -> float
{ return wrap_angle( static_cast<float>( units::to_radians( face + 90_degrees ) ) ); }

} // namespace r3d

#endif // CATA_SRC_R3D_VEHICLE_CAMERA_H
