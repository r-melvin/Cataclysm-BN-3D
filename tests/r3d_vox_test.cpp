#include "catch/catch.hpp"
#include "r3d_vox.h"

#include <array>
#include <cstdint>
#include <ranges>
#include <string_view>
#include <vector>

namespace {

auto append_u32(std::vector<uint8_t>& bytes, uint32_t value) -> void {
    bytes.push_back(static_cast<uint8_t>(value));
    bytes.push_back(static_cast<uint8_t>(value >> 8));
    bytes.push_back(static_cast<uint8_t>(value >> 16));
    bytes.push_back(static_cast<uint8_t>(value >> 24));
}

auto append_chunk(
    std::vector<uint8_t>& bytes, std::string_view id, const std::vector<uint8_t>& content) -> void {
    for (const auto character : id) { bytes.push_back(static_cast<uint8_t>(character)); }
    append_u32(bytes, static_cast<uint32_t>(content.size()));
    append_u32(bytes, 0);
    bytes.insert(bytes.end(), content.begin(), content.end());
}

auto make_vox_file(bool include_palette = true) -> std::vector<uint8_t> {
    auto children = std::vector<uint8_t>{};
    auto size = std::vector<uint8_t>{};
    append_u32(size, 2);
    append_u32(size, 3);
    append_u32(size, 4);
    append_chunk(children, "SIZE", size);

    auto voxels = std::vector<uint8_t>{};
    append_u32(voxels, 1);
    voxels.insert(voxels.end(), {1, 2, 3, 1});
    append_chunk(children, "XYZI", voxels);

    if (include_palette) {
        auto colors = std::vector<uint8_t>(1024, 255);
        colors[0] = 12;
        colors[1] = 34;
        colors[2] = 56;
        append_chunk(children, "RGBA", colors);
    }

    auto file = std::vector<uint8_t>{'V', 'O', 'X', ' '};
    append_u32(file, 150);
    for (const auto character : std::string_view("MAIN")) {
        file.push_back(static_cast<uint8_t>(character));
    }
    append_u32(file, 0);
    append_u32(file, static_cast<uint32_t>(children.size()));
    file.insert(file.end(), children.begin(), children.end());
    return file;
}

} // namespace

TEST_CASE("r3d_vox_reader_preserves_model_geometry_and_palette", "[r3d][voxels]") {
    const auto file = make_vox_file();
    const auto model = r3d::read_vox_model(file);

    REQUIRE(model.has_value());
    CHECK(model->size_x == 2);
    CHECK(model->size_y == 3);
    CHECK(model->size_z == 4);
    REQUIRE(model->cells.size() == 1);
    CHECK(model->cells.front().x == 1);
    CHECK(model->cells.front().y == 2);
    CHECK(model->cells.front().z == 3);
    CHECK(model->palette[1].r == 12);
    CHECK(model->palette[1].g == 34);
    CHECK(model->palette[1].b == 56);
    CHECK(model->palette[1].a == 255);
}

TEST_CASE("r3d_vox_reader_uses_the_magica_voxel_default_palette", "[r3d][voxels]") {
    const auto file = make_vox_file(false);
    const auto model = r3d::read_vox_model(file);

    REQUIRE(model.has_value());
    CHECK(model->palette[0].a == 0);
    CHECK(model->palette[1].r == 255);
    CHECK(model->palette[1].g == 255);
    CHECK(model->palette[1].b == 255);
    CHECK(model->palette[2].r == 255);
    CHECK(model->palette[2].g == 255);
    CHECK(model->palette[2].b == 204);
}

TEST_CASE("r3d_vox_models_fit_inside_a_unit_cube_without_losing_proportions", "[r3d][voxels]") {
    const auto model = r3d::vox_model{
        .size_x = 8, .size_y = 4, .size_z = 16, .palette = {}, .cells = {}};
    const auto size = r3d::fit_vox_model_to_unit_cube(model);

    CHECK(size.x == Approx(0.5f));
    CHECK(size.y == Approx(0.25f));
    CHECK(size.z == Approx(1.0f));
    CHECK(r3d::fit_vox_model_to_unit_cube({}).x == 0.0f);
}

TEST_CASE("r3d_vox_reader_rejects_truncated_and_out_of_bounds_models", "[r3d][voxels]") {
    const auto file = make_vox_file();
    CHECK_FALSE(r3d::read_vox_model(std::span<const uint8_t>(file).first(14)));

    auto invalid_cell = file;
    invalid_cell[60] = 2; // The fixture's voxel x coordinate is outside its 2-cell model width.
    CHECK_FALSE(r3d::read_vox_model(invalid_cell));
}

TEST_CASE("r3d_vox_surface_omits_internal_faces_and_keeps_palette_indices", "[r3d][voxels]") {
    const auto model = r3d::vox_model{
        .size_x = 2,
        .size_y = 1,
        .size_z = 1,
        .cells = {{.x = 0, .y = 0, .z = 0, .color_index = 7},
                  {.x = 1, .y = 0, .z = 0, .color_index = 9}}};
    const auto surface = r3d::extract_vox_surface(model);

    REQUIRE(surface.has_value());
    CHECK(surface->faces.size() == 10);
    CHECK(std::ranges::none_of(surface->faces, [](const auto& face) {
        return (face.direction == r3d::vox_face_direction::positive_x && face.x == 0)
            || (face.direction == r3d::vox_face_direction::negative_x && face.x == 1);
    }));
    CHECK(
        std::ranges::count_if(surface->faces, [](const auto& face) { return face.color_index == 7; })
        == 5);
    CHECK(
        std::ranges::count_if(surface->faces, [](const auto& face) { return face.color_index == 9; })
        == 5);
}
