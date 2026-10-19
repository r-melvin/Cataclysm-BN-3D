#include "catch/catch.hpp"
#include "r3d_item_profiles.h"

#include <algorithm>
#include <cstdint>
#include <limits>

TEST_CASE("r3d_cylinder_items_use_repeated_sprite_geometry", "[r3d][items]") {
    for (const auto id :
         {"carbondioxide_tank", "hydrogen_tank", "nitrogen_tank", "oxygen_cylinder", "oxygen_tank",
          "smoxygen_tank", "scuba_tank", "small_scuba_tank", "tinyweldtank", "weldtank",
          "gradcylinder"}) {
        const auto profile = r3d::item_profile(id);
        CHECK(profile.geometry == r3d::item_geometry::sprite_volume);
        CHECK(
            profile.depth_fraction
            == Approx(
                id == std::string_view("small_scuba_tank") || id == std::string_view("tinyweldtank")
                    ? 0.28f
                    : 0.38f));
    }
}

TEST_CASE("r3d_unprofiled_items_use_shallow_sprite_volume", "[r3d][items]") {
    const auto profile = r3d::item_profile("unknown_item");
    CHECK(profile.geometry == r3d::item_geometry::sprite_volume);
    CHECK(profile.depth_fraction == Approx(0.22f));
}

TEST_CASE("r3d_authored_depth_fraction_beats_size_ratio", "[r3d][items]") {
    // Authored depth_fraction wins over the size[1]/size[0] ratio, which wins over the default.
    CHECK(
        r3d::resolve_item_depth_fraction(
            {.fallback = 0.22f, .authored_fraction = 0.38f, .size = {0.5f, 0.1f, 1.0f}})
        == Approx(0.38f));
    CHECK(
        r3d::resolve_item_depth_fraction(
            {.fallback = 0.22f, .authored_fraction = 0.0f, .size = {0.5f, 0.1f, 1.0f}})
        == Approx(0.2f));
    CHECK(
        r3d::resolve_item_depth_fraction(
            {.fallback = 0.22f, .authored_fraction = 0.0f, .size = {0.5f, 0.0f, 1.0f}})
        == Approx(0.22f));
    CHECK(
        r3d::resolve_item_depth_fraction(
            {.fallback = 0.22f, .authored_fraction = 0.0f, .size = {0.0f, 0.1f, 1.0f}})
        == Approx(0.22f));
}
