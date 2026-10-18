#include "r3d_vox.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>

namespace r3d {
namespace {

struct chunk_header {
    std::string_view id;
    size_t content_start;
    size_t content_end;
    size_t chunk_end;
};

auto read_u32(std::span<const uint8_t> bytes, size_t offset)
    -> std::expected<uint32_t, std::string> {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        return std::unexpected("truncated 32-bit value");
    }
    return static_cast<uint32_t>(bytes[offset]) | (static_cast<uint32_t>(bytes[offset + 1]) << 8)
         | (static_cast<uint32_t>(bytes[offset + 2]) << 16)
         | (static_cast<uint32_t>(bytes[offset + 3]) << 24);
}

auto read_chunk_header(std::span<const uint8_t> bytes, size_t offset, size_t parent_end)
    -> std::expected<chunk_header, std::string> {
    if (offset > parent_end || parent_end - offset < 12) {
        return std::unexpected("truncated chunk header");
    }
    const auto content_size = read_u32(bytes, offset + 4);
    const auto children_size = read_u32(bytes, offset + 8);
    if (!content_size || !children_size) { return std::unexpected("truncated chunk lengths"); }
    const auto content_start = offset + 12;
    if (*content_size > parent_end - content_start) {
        return std::unexpected("chunk content exceeds parent");
    }
    const auto content_end = content_start + static_cast<size_t>(*content_size);
    if (*children_size > parent_end - content_end) {
        return std::unexpected("chunk children exceed parent");
    }
    return chunk_header{
        .id = std::string_view(reinterpret_cast<const char*>(bytes.data() + offset), 4),
        .content_start = content_start,
        .content_end = content_end,
        .chunk_end = content_end + static_cast<size_t>(*children_size)};
}

} // namespace

auto read_vox_model(std::span<const uint8_t> bytes) -> std::expected<vox_model, std::string> {
    if (bytes.size() < 20
        || std::string_view(reinterpret_cast<const char*>(bytes.data()), 4) != "VOX ") {
        return std::unexpected("invalid VOX header");
    }
    const auto version = read_u32(bytes, 4);
    if (!version || *version < 150) { return std::unexpected("unsupported VOX version"); }
    const auto main = read_chunk_header(bytes, 8, bytes.size());
    if (!main || main->id != "MAIN" || main->content_end != main->content_start
        || main->chunk_end != bytes.size()) {
        return std::unexpected("missing or invalid MAIN chunk");
    }

    auto model = vox_model{};
    auto has_size = false;
    auto has_xyzi = false;
    auto has_palette = false;
    auto cursor = main->content_end;
    const auto end = main->chunk_end;
    while (cursor < end) {
        const auto chunk = read_chunk_header(bytes, cursor, end);
        if (!chunk) { return std::unexpected(chunk.error()); }
        const auto content =
            bytes.subspan(chunk->content_start, chunk->content_end - chunk->content_start);
        if (chunk->id == "SIZE") {
            if (has_size || content.size() != 12) {
                return std::unexpected("duplicate or malformed SIZE chunk");
            }
            const auto sx = read_u32(content, 0);
            const auto sy = read_u32(content, 4);
            const auto sz = read_u32(content, 8);
            if (!sx || !sy || !sz || *sx == 0 || *sy == 0 || *sz == 0 || *sx > 256 || *sy > 256
                || *sz > 256) {
                return std::unexpected("VOX dimensions must be between 1 and 256");
            }
            model.size_x = static_cast<uint16_t>(*sx);
            model.size_y = static_cast<uint16_t>(*sy);
            model.size_z = static_cast<uint16_t>(*sz);
            has_size = true;
        } else if (chunk->id == "XYZI") {
            if (has_xyzi || content.size() < 4) {
                return std::unexpected("duplicate or malformed XYZI chunk");
            }
            const auto count = read_u32(content, 0);
            if (!count || *count > 1'000'000
                || content.size() != 4 + static_cast<size_t>(*count) * 4) {
                return std::unexpected("invalid XYZI voxel count");
            }
            model.cells.reserve(*count);
            for (auto index = size_t{0}; index < *count; ++index) {
                const auto offset = 4 + index * 4;
                model.cells.push_back(
                    {.x = content[offset],
                     .y = content[offset + 1],
                     .z = content[offset + 2],
                     .color_index = content[offset + 3]});
            }
            has_xyzi = true;
        } else if (chunk->id == "RGBA") {
            if (has_palette || content.size() != 1024) {
                return std::unexpected("duplicate or malformed RGBA chunk");
            }
            for (auto index = size_t{0}; index < 255; ++index) {
                const auto offset = index * 4;
                model.palette[index + 1] =
                    {.r = content[offset],
                     .g = content[offset + 1],
                     .b = content[offset + 2],
                     .a = content[offset + 3]};
            }
            const auto offset = 255 * 4;
            model.palette[0] =
                {.r = content[offset],
                 .g = content[offset + 1],
                 .b = content[offset + 2],
                 .a = content[offset + 3]};
            has_palette = true;
        } else if (chunk->chunk_end != chunk->content_end) {
            // Scene graph chunks contain children and represent more than one model or transform.
            return std::unexpected("nested VOX scene chunks are not supported");
        }
        cursor = chunk->chunk_end;
    }
    if (cursor != end || !has_size || !has_xyzi) {
        return std::unexpected("VOX file must contain one SIZE and one XYZI chunk");
    }
    if (!has_palette) {
        constexpr auto palette = std::array<uint32_t, 256>{
            {0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff,
             0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff,
             0xffcc99ff, 0xff9999ff, 0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff,
             0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff,
             0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff, 0xffcc00ff, 0xff9900ff, 0xff6600ff,
             0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc,
             0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc, 0xff00cccc,
             0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc,
             0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc,
             0xff9933cc, 0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc,
             0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99,
             0xff33ff99, 0xff00ff99, 0xffffcc99, 0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99,
             0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999,
             0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699, 0xff006699, 0xffff3399,
             0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099,
             0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
             0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66,
             0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966,
             0xff009966, 0xffff6666, 0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666,
             0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066,
             0xffcc0066, 0xff990066, 0xff660066, 0xff330066, 0xff000066, 0xffffff33, 0xffccff33,
             0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33,
             0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933, 0xff669933,
             0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633,
             0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333,
             0xffff0033, 0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00,
             0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00,
             0xff99cc00, 0xff66cc00, 0xff33cc00, 0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900,
             0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600,
             0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300, 0xff663300, 0xff333300,
             0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee,
             0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044,
             0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800,
             0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000,
             0xffbb0000, 0xffaa0000, 0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000,
             0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777,
             0xff555555, 0xff444444, 0xff222222, 0xff111111}};
        for (auto index = size_t{0}; index < palette.size(); ++index) {
            const auto value = palette[index];
            model.palette[index] =
                {.r = static_cast<uint8_t>(value),
                 .g = static_cast<uint8_t>(value >> 8),
                 .b = static_cast<uint8_t>(value >> 16),
                 .a = static_cast<uint8_t>(value >> 24)};
        }
    }
    auto occupied = std::vector<uint8_t>(
        static_cast<size_t>(model.size_x) * model.size_y * model.size_z);
    for (const auto& cell : model.cells) {
        if (cell.color_index == 0 || cell.x >= model.size_x || cell.y >= model.size_y
            || cell.z >= model.size_z) {
            return std::unexpected("voxel coordinate or color index is outside the model");
        }
        const auto index =
            (static_cast<size_t>(cell.z) * model.size_y + cell.y) * model.size_x + cell.x;
        if (occupied[index] != 0) { return std::unexpected("duplicate voxel coordinate"); }
        occupied[index] = cell.color_index;
    }
    return model;
}

auto fit_vox_model_to_unit_cube(const vox_model& model) -> vox_model_unit_size {
    const auto largest_dimension = static_cast<float>(
                                      std::max({model.size_x, model.size_y, model.size_z}));
    if (largest_dimension <= 0.0f) { return {}; }
    return {.x = static_cast<float>(model.size_x) / largest_dimension,
            .y = static_cast<float>(model.size_y) / largest_dimension,
            .z = static_cast<float>(model.size_z) / largest_dimension};
}

auto extract_vox_surface(const vox_model& model) -> std::expected<vox_surface, std::string> {
    constexpr auto max_faces = static_cast<size_t>(1'000'000);
    if (model.size_x == 0 || model.size_y == 0 || model.size_z == 0 || model.size_x > 256
        || model.size_y > 256 || model.size_z > 256 || model.cells.size() > 1'000'000) {
        return std::unexpected("invalid or oversized voxel model");
    }
    const auto volume = static_cast<size_t>(model.size_x) * model.size_y * model.size_z;
    auto occupied = std::vector<uint8_t>(volume);
    for (const auto& cell : model.cells) {
        if (cell.color_index == 0 || cell.x >= model.size_x || cell.y >= model.size_y
            || cell.z >= model.size_z) {
            return std::unexpected("voxel coordinate or color index is outside the model");
        }
        const auto index =
            (static_cast<size_t>(cell.z) * model.size_y + cell.y) * model.size_x + cell.x;
        if (occupied[index] != 0) { return std::unexpected("duplicate voxel coordinate"); }
        occupied[index] = cell.color_index;
    }

    const auto is_occupied = [&occupied, &model](int x, int y, int z) {
        if (x < 0 || y < 0 || z < 0 || x >= model.size_x || y >= model.size_y
            || z >= model.size_z) {
            return false;
        }
        const auto index =
            (static_cast<size_t>(z) * model.size_y + static_cast<size_t>(y)) * model.size_x
            + static_cast<size_t>(x);
        return occupied[index] != 0;
    };
    auto surface = vox_surface{};
    surface.faces.reserve(std::min(model.cells.size() * 3, max_faces));
    const auto add_merged_faces = [&](int axis, vox_face_direction direction) -> bool {
        const auto positive = direction == vox_face_direction::positive_x
                              || direction == vox_face_direction::positive_y
                              || direction == vox_face_direction::positive_z;
        const auto plane_count = axis == 0 ? static_cast<int>(model.size_x)
                               : axis == 1 ? static_cast<int>(model.size_y)
                               : static_cast<int>(model.size_z);
        const auto width = axis == 0 ? static_cast<int>(model.size_y)
                          : static_cast<int>(model.size_x);
        const auto height = axis == 2 ? static_cast<int>(model.size_y)
                           : static_cast<int>(model.size_z);
        for (auto plane = 0; plane <= plane_count; ++plane) {
            auto colors = std::vector<uint8_t>(static_cast<size_t>(width * height));
            for (auto v = 0; v < height; ++v) {
                for (auto u = 0; u < width; ++u) {
                    const auto source_axis = positive ? plane - 1 : plane;
                    const auto neighbor_axis = source_axis + ( positive ? 1 : -1 );
                    const auto x = axis == 0 ? source_axis : u;
                    const auto y = axis == 1 ? source_axis : axis == 0 ? u : v;
                    const auto z = axis == 2 ? source_axis : v;
                    const auto neighbor_x = axis == 0 ? neighbor_axis : x;
                    const auto neighbor_y = axis == 1 ? neighbor_axis : y;
                    const auto neighbor_z = axis == 2 ? neighbor_axis : z;
                    if (is_occupied(x, y, z) && !is_occupied(neighbor_x, neighbor_y, neighbor_z)) {
                        colors[static_cast<size_t>(v * width + u)] = occupied[
                            (static_cast<size_t>(z) * model.size_y + static_cast<size_t>(y))
                            * model.size_x + static_cast<size_t>(x)];
                    }
                }
            }
            for (auto v = 0; v < height; ++v) {
                for (auto u = 0; u < width;) {
                    const auto color = colors[static_cast<size_t>(v * width + u)];
                    if (color == 0) {
                        ++u;
                        continue;
                    }
                    auto span_u = 1;
                    while (u + span_u < width
                           && colors[static_cast<size_t>(v * width + u + span_u)] == color) {
                        ++span_u;
                    }
                    auto span_v = 1;
                    auto can_extend = true;
                    while (v + span_v < height && can_extend) {
                        for (auto du = 0; du < span_u; ++du) {
                            if (colors[static_cast<size_t>((v + span_v) * width + u + du)] != color) {
                                can_extend = false;
                                break;
                            }
                        }
                        if (can_extend) {
                            ++span_v;
                        }
                    }
                    for (auto dv = 0; dv < span_v; ++dv) {
                        for (auto du = 0; du < span_u; ++du) {
                            colors[static_cast<size_t>((v + dv) * width + u + du)] = 0;
                        }
                    }
                    if (surface.faces.size() == max_faces) {
                        return false;
                    }
                    const auto source_axis = positive ? plane - 1 : plane;
                    const auto x = axis == 0 ? source_axis : u;
                    const auto y = axis == 1 ? source_axis : axis == 0 ? u : v;
                    const auto z = axis == 2 ? source_axis : v;
                    surface.faces.push_back({.direction = direction,
                                             .x = static_cast<uint8_t>(x),
                                             .y = static_cast<uint8_t>(y),
                                             .z = static_cast<uint8_t>(z),
                                             .span_u = static_cast<uint16_t>(span_u),
                                             .span_v = static_cast<uint16_t>(span_v),
                                             .color_index = color});
                    u += span_u;
                }
            }
        }
        return true;
    };
    constexpr auto directions = std::array<vox_face_direction, 6>{
        vox_face_direction::negative_x, vox_face_direction::positive_x,
        vox_face_direction::negative_y, vox_face_direction::positive_y,
        vox_face_direction::negative_z, vox_face_direction::positive_z};
    for (auto axis = 0; axis < 3; ++axis) {
        for (const auto direction : std::span(directions).subspan(static_cast<size_t>(axis * 2), 2)) {
            if (!add_merged_faces(axis, direction)) {
                return std::unexpected("voxel model exposes too many faces");
            }
        }
    }
    return surface;
}

} // namespace r3d
