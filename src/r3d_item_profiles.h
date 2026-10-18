#pragma once
#ifndef CATA_SRC_R3D_ITEM_PROFILES_H
#define CATA_SRC_R3D_ITEM_PROFILES_H

#include <string_view>

namespace r3d
{

enum class item_geometry {
    billboard,
    repeated_sprite_volume
};

struct item_render_profile {
    item_geometry geometry = item_geometry::billboard;
    float depth_fraction = 0.0f;
};

/// Presentation-only shape selection for loose items. Unknown items keep the sprite billboard.
auto item_profile( std::string_view id ) -> item_render_profile;

} // namespace r3d

#endif // CATA_SRC_R3D_ITEM_PROFILES_H
