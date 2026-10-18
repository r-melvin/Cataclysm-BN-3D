#pragma once
#ifndef CATA_SRC_R3D_VOX_H
#    define CATA_SRC_R3D_VOX_H

#    include <array>
#    include <cstdint>
#    include <expected>
#    include <span>
#    include <string>
#    include <vector>

namespace r3d {

struct vox_color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
};

struct vox_cell {
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t z = 0;
    uint8_t color_index = 0;
};

/// One bounded MagicaVoxel model, with coordinates preserved in the VOX x/y/z convention.
struct vox_model {
    uint16_t size_x = 0;
    uint16_t size_y = 0;
    uint16_t size_z = 0;
    std::array<vox_color, 256> palette{};
    std::vector<vox_cell> cells;
};

struct vox_model_unit_size {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

enum class vox_face_direction : uint8_t {
    negative_x,
    positive_x,
    negative_y,
    positive_y,
    negative_z,
    positive_z
};

struct vox_surface_face {
    vox_face_direction direction;
    uint8_t x;
    uint8_t y;
    uint8_t z;
    uint8_t color_index;
};

struct vox_surface {
    std::vector<vox_surface_face> faces;
};

/// Read a single-model MagicaVoxel file; malformed chunks and scene-graph files are rejected.
auto read_vox_model(std::span<const uint8_t> bytes) -> std::expected<vox_model, std::string>;

/// Return model dimensions scaled proportionally to fit within a 1x1x1 unit cube.
auto fit_vox_model_to_unit_cube(const vox_model& model) -> vox_model_unit_size;

/// Build only exterior faces from a voxel grid, keeping each face's voxel palette index.
auto extract_vox_surface(const vox_model& model) -> std::expected<vox_surface, std::string>;

} // namespace r3d

#endif // CATA_SRC_R3D_VOX_H
