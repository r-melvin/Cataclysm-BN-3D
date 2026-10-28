#pragma once
#ifndef CATA_SRC_R3D_VEHICLE_SILHOUETTE_H
#define CATA_SRC_R3D_VEHICLE_SILHOUETTE_H

#include "coordinates.h"
#include "r3d_shapes.h"

class vehicle;
struct vehicle_part;

namespace r3d
{
struct vehicle_silhouette {
    bool rotorcraft = false;
    bool enclosed = false;
    int main_rotors = 0;
    float rotor_forward = 0.0f;
    float rotor_right = 0.0f;
    int roof_front = 0;
    int roof_rear = 0;
    float cabin_height = vehicle_cabin_h;
    int cabin_rear = 0;
    int tail_rear = 0;
    float tail_center = 0.0f;
};
/// Infer proportions from installed cabin, wheel and rotor parts, including custom assemblies.
auto describe_vehicle( const vehicle &veh ) -> vehicle_silhouette;
enum class airframe_section { cabin, boom, stabilizer, fin };
struct airframe_section_options {
    const vehicle &veh;
    const vehicle_silhouette &silhouette;
    tripoint_mnt_veh mount;
};
/// Only existing aft half-panels are reshaped; cargo, floors and full walls stay intact.
auto describe_airframe_section( const airframe_section_options &options ) -> airframe_section;
struct wheel_dimensions {
    float radius_forward;
    float radius_up;
    float width;
};
/// Native millimetres converted to tile space; vertical units use the renderer's 1.8 storey scale.
auto native_wheel_dimensions( const vehicle_part &part ) -> wheel_dimensions;
}
#endif
