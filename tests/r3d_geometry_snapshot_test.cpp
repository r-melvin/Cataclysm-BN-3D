#if defined(TILES)
#    include "catch/catch.hpp"
#    include "avatar.h"
#    include "cata_tiles.h"
#    include "cata_utility.h"
#    include "item.h"
#    include "map/map.h"
#    include "map_helpers.h"
#    include "options.h"
#    include "options_helpers.h"
#    include "path_info.h"
#    include "r3d_entities.h"
#    include "r3d_presentation_profiles.h"
#    include "r3d_vox.h"
#    include "rng.h"
#    include "sdltiles.h"
#    include "state_helpers.h"
#    include "tile_helpers.h"
#    include "vehicle/vehicle.h"

#    include <algorithm>
#    include <cmath>
#    include <cstdint>
#    include <cstdlib>
#    include <fstream>
#    include <iterator>
#    include <locale>
#    include <map>
#    include <sstream>
#    include <string>
#    include <unordered_map>
#    include <vector>

// Geometry regression check: each scene's mesh is reduced to an order-independent fingerprint
// (quads sorted, UVs left out because atlas placement may move) and compared with
// tests/data/r3d_geometry_snapshot.txt. After an intended geometry change, regenerate it with
// R3D_UPDATE_GEOMETRY_SNAPSHOT=1 and review the changed scenes.

namespace {

const auto snapshot_path = std::string("tests/data/r3d_geometry_snapshot.txt");

struct mesh_fingerprint {
    size_t quads = 0;
    uint64_t hash = 0;
};

/// quad_sink sorts quads by texture-sheet pointer, which changes from run to run, so quads are
/// compared as a sorted set. Each keeps its own vertex order (winding).
auto fingerprint(const r3d::chunk_mesh& mesh) -> mesh_fingerprint {
    const auto quantize = [](float value) { return std::lround(value * 1000.0f); };
    auto quads = std::vector<std::string>{};
    for (auto q = size_t{0}; q + 3 < mesh.vertices.size(); q += 4) {
        auto out = std::ostringstream{};
        out.imbue(std::locale::classic());
        for (auto i = q; i < q + 4; ++i) {
            const auto& v = mesh.vertices[i];
            out << quantize(v.x) << ',' << quantize(v.y) << ',' << quantize(v.z) << ','
                << (mesh.light[i] & 0x00ffffffu) << ';';
        }
        quads.push_back(out.str());
    }
    std::ranges::sort(quads);
    auto hash = uint64_t{1469598103934665603ull};
    for (const auto& quad : quads) {
        for (const auto c : quad) {
            hash = (hash ^ static_cast<unsigned char>(c)) * 1099511628211ull;
        }
        hash = (hash ^ '\n') * 1099511628211ull;
    }
    return {.quads = quads.size(), .hash = hash};
}

auto load_vox_asset(const std::string& path) -> r3d::vox_asset {
    auto input = std::ifstream(path, std::ios::binary);
    REQUIRE(input.good());
    const auto bytes = std::vector<uint8_t>(std::istreambuf_iterator<char>(input), {});
    auto model = r3d::read_vox_model(bytes);
    REQUIRE(model.has_value());
    auto surface = r3d::extract_vox_surface(*model);
    REQUIRE(surface.has_value());
    auto palette = r3d::sprite_ref{};
    palette.valid = true;
    return {.model = std::move(*model), .surface = std::move(*surface), .palette = palette};
}

auto format_line(const std::string& scene, const mesh_fingerprint& print) -> std::string {
    auto out = std::ostringstream{};
    out.imbue(std::locale::classic());
    out << scene << ' ' << print.quads << ' ' << std::hex << print.hash;
    return out.str();
}

} // namespace

TEST_CASE("r3d_geometry_matches_snapshot", "[r3d][geometry][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    const auto voxel_items = override_option("R3D_VOXEL_ITEMS", "true");
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);

    const auto parts_dir = PATH_INFO::datadir() + "raw/r3d/vehicle_parts/";
    const auto vehicle_vox = std::unordered_map<std::string, r3d::vox_asset>{
        {"wheel", load_vox_asset(parts_dir + "wheel.vox")},
        {"wheel_wide", load_vox_asset(parts_dir + "wheel_wide.vox")}};
    const auto item_vox = std::unordered_map<std::string, r3d::vox_asset>{
        {"carbondioxide_tank",
         load_vox_asset(PATH_INFO::datadir() + "raw/r3d/items/carbondioxide_tank.vox")}};
    auto profile_input = std::ifstream(PATH_INFO::datadir() + "raw/r3d/presentation_profiles.json");
    REQUIRE(profile_input.good());
    const auto profiles = r3d::load_presentation_profiles(profile_input);

    const auto origin = tripoint_bub_ms(60, 60, 0);
    auto actual = std::map<std::string, std::string>{};
    const auto render = [&](const std::string& scene, auto&& setup, bool vehicles) {
        clear_all_state();
        clear_map();
        get_avatar().setpos(origin);
        rng_set_engine_seed(12345);
        setup();
        auto cache = r3d::tile_info_cache{};
        cache.set_voxel_items(true);
        const auto cam = r3d::camera{};
        auto view = r3d::entity_view{
            .tiles = *tilecontext,
            .cache = cache,
            .cam = cam,
            .center = origin,
            .light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(255, 255, 255); },
            .markers = {},
            .uprights = {},
            .surface = [](const tripoint_bub_ms& /*p*/) { return r3d::floor_height; },
            .vehicle_vox_parts = &vehicle_vox,
            .item_vox_models = &item_vox,
            .presentation_profiles = &profiles};
        const auto mesh = vehicles ? r3d::build_vehicle_mesh(view) : r3d::build_entity_mesh(view);
        REQUIRE_FALSE(mesh.empty());
        actual[scene] = format_line(scene, fingerprint(mesh));
    };

    for (const auto* proto : {"car", "pickup", "suv", "bus", "semi_truck"}) {
        for (const auto degrees : {0, 90, 225}) {
            const auto scene = std::string("vehicle_") + proto + "_" + std::to_string(degrees);
            render(
                scene,
                [&] {
                    REQUIRE(get_map().add_vehicle(
                        vproto_id(proto), origin + tripoint(4, 0, 0), units::from_degrees(degrees),
                        0, 0));
                },
                true);
        }
    }
    for (const auto* id : {"carbondioxide_tank", "rock"}) {
        render(
            std::string("item_") + id,
            [&] { get_map().add_item(origin + tripoint(1, 0, 0), item::spawn(id, calendar::turn)); },
            false);
    }

    if (std::getenv("R3D_UPDATE_GEOMETRY_SNAPSHOT") != nullptr) {
        auto out = std::ofstream(snapshot_path);
        REQUIRE(out.good());
        for (const auto& [scene, line] : actual) { out << line << '\n'; }
        WARN("Wrote " << snapshot_path);
        return;
    }
    auto input = std::ifstream(snapshot_path);
    INFO("Missing " << snapshot_path << "; generate it with R3D_UPDATE_GEOMETRY_SNAPSHOT=1");
    REQUIRE(input.good());
    auto expected = std::map<std::string, std::string>{};
    for (auto line = std::string{}; std::getline(input, line);) {
        if (!line.empty()) { expected[line.substr(0, line.find(' '))] = line; }
    }
    for (const auto& [scene, line] : actual) {
        INFO("Geometry changed; if intended, regenerate with R3D_UPDATE_GEOMETRY_SNAPSHOT=1");
        CHECK(line == expected[scene]);
    }
    CHECK(expected.size() == actual.size());
}
#endif
