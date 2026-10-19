#include "catch/catch.hpp"
#include "path_info.h"
#include "r3d_vox.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
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

auto make_vox_file_with_children(const std::vector<uint8_t>& children) -> std::vector<uint8_t> {
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

auto make_size_content(uint32_t x, uint32_t y, uint32_t z) -> std::vector<uint8_t> {
    auto size = std::vector<uint8_t>{};
    append_u32(size, x);
    append_u32(size, y);
    append_u32(size, z);
    return size;
}

auto make_xyzi_content(const std::vector<std::array<uint8_t, 4>>& voxels) -> std::vector<uint8_t> {
    auto content = std::vector<uint8_t>{};
    append_u32(content, static_cast<uint32_t>(voxels.size()));
    for (const auto& voxel : voxels) { content.insert(content.end(), voxel.begin(), voxel.end()); }
    return content;
}

auto make_vox_file(bool include_palette = true) -> std::vector<uint8_t> {
    auto children = std::vector<uint8_t>{};
    append_chunk(children, "SIZE", make_size_content(2, 3, 4));
    append_chunk(children, "XYZI", make_xyzi_content({{1, 2, 3, 1}}));

    if (include_palette) {
        auto colors = std::vector<uint8_t>(1024, 255);
        colors[0] = 12;
        colors[1] = 34;
        colors[2] = 56;
        append_chunk(children, "RGBA", colors);
    }

    return make_vox_file_with_children(children);
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

TEST_CASE("r3d_authored_co2_tank_asset_is_a_valid_supported_vox_model", "[r3d][voxels]") {
    const auto path = PATH_INFO::datadir() + "raw/r3d/items/carbondioxide_tank.vox";
    auto input = std::ifstream(path, std::ios::binary);
    REQUIRE(input.good());
    const auto bytes = std::vector<uint8_t>(std::istreambuf_iterator<char>(input), {});
    const auto model = r3d::read_vox_model(bytes);

    REQUIRE(model.has_value());
    CHECK(model->size_x == 14);
    CHECK(model->size_y == 12);
    CHECK(model->size_z == 30);
    CHECK(std::ranges::any_of(model->cells, [](const auto& cell) {
        return cell.color_index == 4;
    }));
    CHECK(model->palette[1].a == 255);
    CHECK(model->palette[4].r == 151);
    const auto surface = r3d::extract_vox_surface(*model);
    REQUIRE(surface.has_value());
    CHECK_FALSE(surface->faces.empty());
}

TEST_CASE("r3d_authored_wide_wheel_asset_has_tire_and_hub_materials", "[r3d][voxels]") {
    const auto path = PATH_INFO::datadir() + "raw/r3d/vehicle_parts/wheel_wide.vox";
    auto input = std::ifstream(path, std::ios::binary);
    REQUIRE(input.good());
    const auto bytes = std::vector<uint8_t>(std::istreambuf_iterator<char>(input), {});
    const auto model = r3d::read_vox_model(bytes);

    REQUIRE(model.has_value());
    CHECK(model->size_x == 24);
    CHECK(model->size_y == 10);
    CHECK(model->size_z == 24);
    CHECK(std::ranges::any_of(model->cells, [](const auto& cell) {
        return cell.color_index == 4;
    }));
    CHECK(model->palette[1].r == 28);
    CHECK(model->palette[4].r == 174);
    const auto surface = r3d::extract_vox_surface(*model);
    REQUIRE(surface.has_value());
    CHECK_FALSE(surface->faces.empty());
}

TEST_CASE("r3d_authored_standard_wheel_asset_has_tire_and_hub_materials", "[r3d][voxels]") {
    const auto path = PATH_INFO::datadir() + "raw/r3d/vehicle_parts/wheel.vox";
    auto input = std::ifstream(path, std::ios::binary);
    REQUIRE(input.good());
    const auto bytes = std::vector<uint8_t>(std::istreambuf_iterator<char>(input), {});
    const auto model = r3d::read_vox_model(bytes);

    REQUIRE(model.has_value());
    CHECK(model->size_x == 22);
    CHECK(model->size_y == 8);
    CHECK(model->size_z == 22);
    CHECK(std::ranges::any_of(model->cells, [](const auto& cell) {
        return cell.color_index == 4;
    }));
    CHECK(model->palette[1].r == 25);
    CHECK(model->palette[4].r == 174);
    const auto surface = r3d::extract_vox_surface(*model);
    REQUIRE(surface.has_value());
    CHECK_FALSE(surface->faces.empty());
}

TEST_CASE("r3d_vox_models_fit_inside_a_unit_cube_without_losing_proportions", "[r3d][voxels]") {
    const auto model =
        r3d::vox_model{.size_x = 8, .size_y = 4, .size_z = 16, .palette = {}, .cells = {}};
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

TEST_CASE("r3d_vox_surface_merges_solid_coplanar_faces", "[r3d][voxels]") {
    auto model = r3d::vox_model{.size_x = 8, .size_y = 8, .size_z = 8, .palette = {}, .cells = {}};
    for (auto z = 0; z < model.size_z; ++z) {
        for (auto y = 0; y < model.size_y; ++y) {
            for (auto x = 0; x < model.size_x; ++x) {
                model.cells.push_back(
                    {.x = static_cast<uint8_t>(x),
                     .y = static_cast<uint8_t>(y),
                     .z = static_cast<uint8_t>(z),
                     .color_index = 3});
            }
        }
    }
    const auto surface = r3d::extract_vox_surface(model);

    REQUIRE(surface.has_value());
    REQUIRE(surface->faces.size() == 6);
    CHECK(std::ranges::all_of(surface->faces, [](const auto& face) {
        return face.span_u == 8 && face.span_v == 8 && face.color_index == 3;
    }));
}

TEST_CASE("r3d_vox_reader_rejects_duplicate_voxel_coordinates", "[r3d][voxels]") {
    auto children = std::vector<uint8_t>{};
    append_chunk(children, "SIZE", make_size_content(2, 3, 4));
    append_chunk(children, "XYZI", make_xyzi_content({{1, 2, 3, 1}, {1, 2, 3, 2}}));
    const auto model = r3d::read_vox_model(make_vox_file_with_children(children));

    REQUIRE_FALSE(model.has_value());
    CHECK(model.error() == "duplicate voxel coordinate");
}

TEST_CASE("r3d_vox_reader_rejects_voxels_with_color_index_zero", "[r3d][voxels]") {
    auto children = std::vector<uint8_t>{};
    append_chunk(children, "SIZE", make_size_content(2, 3, 4));
    append_chunk(children, "XYZI", make_xyzi_content({{1, 2, 3, 0}}));
    const auto model = r3d::read_vox_model(make_vox_file_with_children(children));

    REQUIRE_FALSE(model.has_value());
    CHECK(model.error() == "voxel coordinate or color index is outside the model");
}

TEST_CASE("r3d_vox_reader_rejects_duplicate_size_chunks", "[r3d][voxels]") {
    auto children = std::vector<uint8_t>{};
    append_chunk(children, "SIZE", make_size_content(2, 3, 4));
    append_chunk(children, "SIZE", make_size_content(2, 3, 4));
    append_chunk(children, "XYZI", make_xyzi_content({{1, 2, 3, 1}}));
    const auto model = r3d::read_vox_model(make_vox_file_with_children(children));

    REQUIRE_FALSE(model.has_value());
    CHECK(model.error() == "duplicate or malformed SIZE chunk");
}

TEST_CASE("r3d_vox_reader_rejects_duplicate_xyzi_chunks", "[r3d][voxels]") {
    auto children = std::vector<uint8_t>{};
    append_chunk(children, "SIZE", make_size_content(2, 3, 4));
    append_chunk(children, "XYZI", make_xyzi_content({{1, 2, 3, 1}}));
    append_chunk(children, "XYZI", make_xyzi_content({{0, 0, 0, 1}}));
    const auto model = r3d::read_vox_model(make_vox_file_with_children(children));

    REQUIRE_FALSE(model.has_value());
    CHECK(model.error() == "duplicate or malformed XYZI chunk");
}

TEST_CASE("r3d_vox_reader_rejects_rgba_chunks_not_1024_bytes", "[r3d][voxels]") {
    auto children = std::vector<uint8_t>{};
    append_chunk(children, "SIZE", make_size_content(2, 3, 4));
    append_chunk(children, "XYZI", make_xyzi_content({{1, 2, 3, 1}}));
    append_chunk(children, "RGBA", std::vector<uint8_t>(1020, 255));
    const auto model = r3d::read_vox_model(make_vox_file_with_children(children));

    REQUIRE_FALSE(model.has_value());
    CHECK(model.error() == "duplicate or malformed RGBA chunk");
}

TEST_CASE("r3d_vox_reader_rejects_versions_before_150", "[r3d][voxels]") {
    auto file = make_vox_file();
    file[4] = 149; // The version is a little-endian u32 at offset 4.
    const auto model = r3d::read_vox_model(file);

    REQUIRE_FALSE(model.has_value());
    CHECK(model.error() == "unsupported VOX version");
}

TEST_CASE("r3d_vox_reader_rejects_unknown_chunks_with_children", "[r3d][voxels]") {
    auto child = std::vector<uint8_t>{};
    append_chunk(child, "SIZE", make_size_content(2, 3, 4));

    auto children = std::vector<uint8_t>{};
    append_chunk(children, "SIZE", make_size_content(2, 3, 4));
    append_chunk(children, "XYZI", make_xyzi_content({{1, 2, 3, 1}}));
    for (const auto character : std::string_view("nTRN")) {
        children.push_back(static_cast<uint8_t>(character));
    }
    append_u32(children, 0);
    append_u32(children, static_cast<uint32_t>(child.size()));
    children.insert(children.end(), child.begin(), child.end());
    const auto model = r3d::read_vox_model(make_vox_file_with_children(children));

    REQUIRE_FALSE(model.has_value());
    CHECK(model.error() == "nested VOX scene chunks are not supported");
}

TEST_CASE("r3d_vox_reader_rejects_xyzi_count_that_mismatches_content", "[r3d][voxels]") {
    auto content = make_xyzi_content({{1, 2, 3, 1}});
    content[0] = 2; // Claims two voxels while only one voxel's bytes are present.
    auto children = std::vector<uint8_t>{};
    append_chunk(children, "SIZE", make_size_content(2, 3, 4));
    append_chunk(children, "XYZI", content);
    const auto model = r3d::read_vox_model(make_vox_file_with_children(children));

    REQUIRE_FALSE(model.has_value());
    CHECK(model.error() == "invalid XYZI voxel count");
}

TEST_CASE("r3d_vox_reader_rejects_zero_and_oversized_dimensions", "[r3d][voxels]") {
    // The SIZE chunk's X dimension is the first u32 of its content, at offset 32 in
    // make_vox_file().
    constexpr auto size_x_offset = size_t{32};

    auto zero_size = make_vox_file();
    zero_size[size_x_offset] = 0;
    const auto zero_model = r3d::read_vox_model(zero_size);
    REQUIRE_FALSE(zero_model.has_value());
    CHECK(zero_model.error() == "VOX dimensions must be between 1 and 256");

    auto oversized = make_vox_file();
    oversized[size_x_offset] = 1; // 257 = 0x101
    oversized[size_x_offset + 1] = 1;
    const auto oversized_model = r3d::read_vox_model(oversized);
    REQUIRE_FALSE(oversized_model.has_value());
    CHECK(oversized_model.error() == "VOX dimensions must be between 1 and 256");
}

TEST_CASE("r3d_vox_surface_rejects_models_exposing_too_many_faces", "[r3d][voxels]") {
    // Isolated voxels on an even lattice expose six faces each and never merge.
    // 200,000 of them expose 1,200,000 faces, above the 1,000,000 face cap.
    constexpr auto voxel_count = size_t{200'000};
    auto model = r3d::vox_model{.size_x = 256, .size_y = 256, .size_z = 256};
    for (auto z = 0; z < 256 && model.cells.size() < voxel_count; z += 2) {
        for (auto y = 0; y < 256 && model.cells.size() < voxel_count; y += 2) {
            for (auto x = 0; x < 256 && model.cells.size() < voxel_count; x += 2) {
                model.cells.push_back(
                    {.x = static_cast<uint8_t>(x),
                     .y = static_cast<uint8_t>(y),
                     .z = static_cast<uint8_t>(z),
                     .color_index = 1});
            }
        }
    }
    REQUIRE(model.cells.size() == voxel_count);

    const auto surface = r3d::extract_vox_surface(model);
    REQUIRE_FALSE(surface.has_value());
    CHECK(surface.error() == "voxel model exposes too many faces");
}
