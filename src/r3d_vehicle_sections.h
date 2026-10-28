#pragma once
#ifndef CATA_SRC_R3D_VEHICLE_SECTIONS_H
#define CATA_SRC_R3D_VEHICLE_SECTIONS_H

#include <array>
#include <map>
#include <vector>

#include "coordinates.h"
#include "r3d_shapes.h"
#include "r3d_vehicle_rules.h"

class vehicle;

namespace r3d
{

struct vehicle_section_bounds {
    int front = 0;
    int rear = 0;
    int left = 0;
    int right = 0;
};

struct vehicle_section_cell {
    vehicle_render_profile profile;
    bool skin = false;
    bool full_panel = false;
    bool glazing = false;
    int body = -1;
    int canopy = -1;
    vehicle_section_bounds row;
    float height = vehicle_cabin_h;
};

struct vehicle_section {
    vehicle_section_bounds bounds;
    std::vector<tripoint_mnt_veh> mounts;
    bool glazed = false;
    bool box = false;
    float height = vehicle_body_h;
};

/// Native mount topology, independent of camera, visibility and decorative attachments.
struct vehicle_sections {
    std::map<tripoint_mnt_veh, vehicle_section_cell> cells;
    std::vector<vehicle_section> bodies;
    std::vector<vehicle_section> canopies;
};

auto describe_vehicle_sections( const vehicle &veh ) -> vehicle_sections;

struct vehicle_wheel_opening_options {
    float from;
    float to;
    float bottom;
    float center;
    float center_z;
    float radius_forward;
    float radius_up;
};

struct vehicle_wall_strip {
    float from;
    float to;
    float bottom_from;
    float bottom_to;
};

/// Faceted arch boundary. The space below these strips belongs to the native wheel.
auto vehicle_wheel_opening( const vehicle_wheel_opening_options &options ) ->
std::vector<vehicle_wall_strip>;

struct vehicle_section_inset_options {
    float position;
    float left;
    float right;
    float amount;
};

/// Symmetric shoulder fitting in section space, independent of the vehicle's mount origin.
auto vehicle_section_inset( const vehicle_section_inset_options &options ) -> float;

} // namespace r3d
#endif
