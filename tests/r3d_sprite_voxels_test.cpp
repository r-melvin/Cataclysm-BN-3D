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

TEST_CASE("r3d_four_orthogonal_voxel_extrusions_boolean_intersection", "[r3d][voxels]") {
    // Empty mask generates no voxels.
    const auto empty = std::array<uint8_t, 9>{};
    const auto empty_shape = r3d::generate_orthogonal_voxel_shape({.width = 3, .height = 3, .mask = empty});
    CHECK(empty_shape.size_x == 3);
    CHECK(empty_shape.size_y == 3);
    CHECK(empty_shape.size_z == 3);
    CHECK(std::ranges::none_of(empty_shape.voxels, [](const auto v) { return v != 0; }));

    // Solid mask occupies the entire bounding cube.
    const auto solid = std::array<uint8_t, 9>{1, 1, 1, 1, 1, 1, 1, 1, 1};
    const auto solid_shape = r3d::generate_orthogonal_voxel_shape({.width = 3, .height = 3, .mask = solid});
    CHECK(solid_shape.voxels.size() == 27);
    CHECK(std::ranges::all_of(solid_shape.voxels, [](const auto v) { return v == 1; }));

    // Cross mask:
    // row 0 (top, z=2):    [0, 1, 0] -> center column only (x=1, y=1)
    // row 1 (middle, z=1): [1, 1, 1] -> full 3x3 platform
    // row 2 (bottom, z=0): [0, 1, 0] -> center column only (x=1, y=1)
    const auto cross = std::array<uint8_t, 9>{0, 1, 0, 1, 1, 1, 0, 1, 0};
    const auto cross_shape = r3d::generate_orthogonal_voxel_shape({.width = 3, .height = 3, .mask = cross});

    // Top layer z=2: only center (1, 1) is occupied.
    CHECK(cross_shape.is_occupied(1, 1, 2));
    CHECK_FALSE(cross_shape.is_occupied(0, 0, 2));
    CHECK_FALSE(cross_shape.is_occupied(1, 0, 2));
    CHECK_FALSE(cross_shape.is_occupied(0, 1, 2));

    // Middle layer z=1: all 9 voxels are occupied.
    for (auto y = 0; y < 3; ++y) {
        for (auto x = 0; x < 3; ++x) {
            CHECK(cross_shape.is_occupied(x, y, 1));
        }
    }

    // Bottom layer z=0: only center (1, 1) is occupied.
    CHECK(cross_shape.is_occupied(1, 1, 0));
    CHECK_FALSE(cross_shape.is_occupied(0, 0, 0));
    CHECK_FALSE(cross_shape.is_occupied(2, 2, 0));

    const auto total_occupied = std::ranges::count(cross_shape.voxels, uint8_t{1});
    CHECK(total_occupied == 11);
}

TEST_CASE("r3d_orthogonal_voxel_mesh_surface_quads", "[r3d][voxels]") {
    const auto cross = std::array<uint8_t, 9>{0, 1, 0, 1, 1, 1, 0, 1, 0};
    const auto cross_shape = r3d::generate_orthogonal_voxel_shape({.width = 3, .height = 3, .mask = cross});
    const auto mesh = r3d::extract_orthogonal_voxel_mesh(cross_shape);

    REQUIRE_FALSE(mesh.quads.empty());
    CHECK(std::ranges::any_of(mesh.quads, [](const auto& q) {
        return q.face == r3d::orthogonal_face_kind::top;
    }));
    CHECK(std::ranges::any_of(mesh.quads, [](const auto& q) {
        return q.face == r3d::orthogonal_face_kind::south;
    }));
    CHECK(std::ranges::any_of(mesh.quads, [](const auto& q) {
        return q.face == r3d::orthogonal_face_kind::north;
    }));
    CHECK(std::ranges::any_of(mesh.quads, [](const auto& q) {
        return q.face == r3d::orthogonal_face_kind::west;
    }));
    CHECK(std::ranges::any_of(mesh.quads, [](const auto& q) {
        return q.face == r3d::orthogonal_face_kind::east;
    }));

    // All quad coordinates and UVs are within normalized [0, 1] bounds.
    for (const auto& q : mesh.quads) {
        CHECK(q.x0 >= 0.0f);
        CHECK(q.x1 <= 1.0f);
        CHECK(q.y0 >= 0.0f);
        CHECK(q.y1 <= 1.0f);
        CHECK(q.z0 >= 0.0f);
        CHECK(q.z1 <= 1.0f);
        CHECK(q.u0 >= 0.0f);
        CHECK(q.u1 <= 1.0f);
        CHECK(q.v0 >= 0.0f);
        CHECK(q.v1 <= 1.0f);
    }
}
