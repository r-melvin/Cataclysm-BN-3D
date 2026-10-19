#pragma once
#ifndef CATA_SRC_R3D_PRESENTATION_PROFILES_H
#define CATA_SRC_R3D_PRESENTATION_PROFILES_H

#include <array>
#include <istream>
#include <string>
#include <unordered_map>
#include <vector>

namespace r3d
{

struct presentation_material_region {
    std::string name;
    std::string role;
    std::vector<int> palette_indices;
};

/// Authored proportions and contact/material annotations shared by the renderer and art gallery.
struct presentation_profile {
    std::array<float, 3> size{}; // forward, right, up in map-tile units; zero means infer
    std::array<float, 3> support_anchor{ 0.5f, 0.5f, 0.0f }; // normalized model coordinates
    float depth_fraction = 0.0f; // sprite extrusion depth as a fraction of its width
    std::vector<presentation_material_region> material_regions;
};

using presentation_profile_map = std::unordered_map<std::string, presentation_profile>;

/// Parse versioned profiles from the renderer's shared presentation_profiles.json file.
auto load_presentation_profiles( std::istream &input ) -> presentation_profile_map;

/// Build the stable lookup key used by JSON assets, runtime rendering, and the gallery.
auto presentation_profile_key( const std::string &category, const std::string &id ) -> std::string;

} // namespace r3d

#endif // CATA_SRC_R3D_PRESENTATION_PROFILES_H
