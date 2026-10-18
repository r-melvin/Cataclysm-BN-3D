#include "catch/catch.hpp"
#include "r3d_sprite_voxels.h"

#include <array>
#include <ranges>

TEST_CASE("r3d_sprite_alpha_rotation_matches_sprite_orientation", "[r3d][items]") {
    const auto pixels = std::array<uint8_t, 6>{1, 0, 0, 1, 1, 0};
    const auto rotated = r3d::orient_sprite_alpha(
        {.width = 3, .height = 2, .alpha = pixels, .quarter_turns = 1, .flip = false});

    CHECK(rotated.width == 2);
    CHECK(rotated.height == 3);
    CHECK(rotated.pixels == std::vector<uint8_t>{1, 1, 1, 0, 0, 0});

    const auto flipped = r3d::orient_sprite_alpha(
        {.width = 3, .height = 2, .alpha = pixels, .quarter_turns = 0, .flip = true});
    CHECK(std::ranges::count(flipped.pixels, uint8_t{1}) == 3);
    CHECK(flipped.pixels == std::vector<uint8_t>{0, 0, 1, 0, 1, 1});
}

TEST_CASE("r3d_sprite_voxel_surface_follows_opaque_contour", "[r3d][items]") {
    const auto cross = std::array<uint8_t, 9>{0, 1, 0, 1, 1, 1, 0, 1, 0};
    const auto surface = r3d::voxelize_sprite_alpha(3, 3, cross);
    const auto front_faces = std::ranges::count_if(surface.faces, [](const auto& face) {
        return face.face == r3d::sprite_voxel_face::front;
    });
    const auto back_faces = std::ranges::count_if(surface.faces, [](const auto& face) {
        return face.face == r3d::sprite_voxel_face::back;
    });
    const auto contour_faces = surface.faces.size() - static_cast<size_t>(front_faces + back_faces);

    CHECK(front_faces == 3);
    CHECK(back_faces == 3);
    CHECK(contour_faces == 12);
    CHECK(std::ranges::all_of(surface.faces, [](const auto& face) {
        return face.sample_x >= 0 && face.sample_x < 3 && face.sample_y >= 0 && face.sample_y < 3;
    }));
}

TEST_CASE("r3d_empty_sprite_mask_has_no_voxel_faces", "[r3d][items]") {
    const auto empty = std::array<uint8_t, 4>{};
    CHECK(r3d::voxelize_sprite_alpha(2, 2, empty).faces.empty());
    CHECK(r3d::voxelize_sprite_alpha(2, 2, std::span<const uint8_t>{}).faces.empty());
}
