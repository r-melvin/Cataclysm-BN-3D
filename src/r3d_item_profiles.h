#pragma once
#ifndef CATA_SRC_R3D_ITEM_PROFILES_H
#define CATA_SRC_R3D_ITEM_PROFILES_H

#include <array>
#include <string_view>

namespace r3d
{

enum class item_geometry {
    billboard,
    sprite_volume
};

struct item_render_profile {
    item_geometry geometry = item_geometry::billboard;
    float depth_fraction = 0.0f;
};

/// Presentation-only shape and depth selection for loose items.
auto item_profile( std::string_view id ) -> item_render_profile;

struct item_depth_options {
    float fallback;
    float authored_fraction;
    std::array<float, 3> size;
};

/// Sprite depth as a width fraction: authored `depth_fraction` first, then size[1] / size[0]
/// when both are authored, else the built-in fallback.
auto resolve_item_depth_fraction( const item_depth_options &options ) -> float;

} // namespace r3d

#endif // CATA_SRC_R3D_ITEM_PROFILES_H
