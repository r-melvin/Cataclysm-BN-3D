#pragma once
#ifndef CATA_SRC_R3D_VEHICLE_RULES_H
#define CATA_SRC_R3D_VEHICLE_RULES_H

#include <array>
#include <cstdint>
#include <vector>
#include "coordinates.h"

class vehicle;

namespace r3d
{
enum class panel_corner { none, front_left, front_right, rear_left, rear_right };

enum class vehicle_render_role : uint8_t {
    frame,
    body_panel,
    glass,
    opaque_structure,
    door,
    openable,
    wheel,
    roof,
    roof_accessory,
    interior,
    detail
};

struct vehicle_render_layer {
    int part_idx = -1;
    std::vector<vehicle_render_role> roles;
    bool open = false;
    bool broken = false;
};

struct vehicle_render_profile {
    tripoint_mnt_veh mount;
    std::vector<vehicle_render_layer> layers;
    int main_part_idx = -1;
    int roof_part_idx = -1;
    int accessory_part_idx = -1;
    panel_corner corner = panel_corner::none;
    bool has_roof = false;
    bool show_roof = false;
    bool has_wheel = false;
    bool is_windshield = false;
    bool is_opaque_wall = false;
    bool is_openable = false;
    bool is_door = false;
    bool is_tall_cabin = false;
    bool is_interior = false;
    float height = 0.0f;
    uint32_t light = 0;
};

/// Classify actual installed parts; roofs do not turn quarterpanels into full walls.
auto vehicle_profile( const vehicle &veh, const tripoint_mnt_veh &mount ) -> vehicle_render_profile;

struct panel_outline_options {
    panel_corner corner;
    float front;
    float rear;
    float left;
    float right;
};
/// Clip the corner specified by the native vehicle-part shape, in mount space.
auto panel_outline( const panel_outline_options &options ) -> std::vector<std::array<float, 2>>;

struct wheel_outline_options {
    int segments;
    float center_forward;
    float center_z;
    float radius_forward;
    float radius_z;
};
/// Sample the cross-section of a vehicle tire in its forward/vertical plane.
auto wheel_outline( const wheel_outline_options &options ) -> std::vector<std::array<float, 2>>;
}
#endif
