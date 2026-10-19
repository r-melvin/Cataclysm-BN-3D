#include "avatar.h"
#include "cata_utility.h"
#include "catch/catch.hpp"
#include "game.h"
#include "map/map.h"
#include "map/mapdata.h"
#include "map_memory.h"
#include "r3d_knowledge.h"
#include "state_helpers.h"

#include <algorithm>

TEST_CASE("r3d_crosshair_uses_memory_instead_of_unseen_live_content", "[r3d][knowledge]") {
    clear_all_state();
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    auto& here = get_map();
    auto& you = get_avatar();
    const auto target = tripoint_bub_ms(60, 60, 0);
    g->place_player(target + point_west);
    const auto absolute = bub_to_abs(target);
    const auto old_overlay = you.get_memorized_tile(absolute);
    const auto old_terrain = you.get_terrain_tile(absolute);
    const auto old_show = you.should_show_map_memory();
    const auto restore_memory = on_out_of_scope([&] {
        you.clear_memorized_tile(absolute);
        if (!old_overlay.tile.empty()) {
            you.memorize_tile(absolute, old_overlay.tile, old_overlay.subtile, old_overlay.rotation);
        }
        if (!old_terrain.tile.empty()) {
            you.memorize_terrain_tile(
                absolute, old_terrain.tile, old_terrain.subtile, old_terrain.rotation);
        }
        if (you.should_show_map_memory() != old_show) { you.toggle_map_memory(); }
    });
    if (!old_show) { you.toggle_map_memory(); }
    you.clear_memorized_tile(absolute);
    here.ter_set(target, ter_str_id("t_floor"));
    here.furn_set(target, furn_str_id("f_locker"));
    here.build_map_cache(0);
    here.update_visibility_cache(0);
    auto& perception = here.access_cache(0);
    std::ranges::fill(perception.visibility_cache, lit_level::DARK);

    CHECK(r3d::crosshair_description(you, here, target).empty());
    you.memorize_terrain_tile(absolute, "t_floor", 0, 0);
    CHECK(r3d::crosshair_description(you, here, target) == ter_str_id("t_floor")->name());
    you.memorize_tile(absolute, "f_chair", 0, 0);
    CHECK(r3d::crosshair_description(you, here, target) == furn_str_id("f_chair")->name());
    here.furn_set(target, furn_str_id("f_table"));
    CHECK(r3d::crosshair_description(you, here, target) == furn_str_id("f_chair")->name());
    perception.visibility_cache[perception.idx(target.x(), target.y())] = lit_level::BRIGHT_ONLY;
    CHECK(r3d::crosshair_description(you, here, target) == furn_str_id("f_chair")->name());
    perception.visibility_cache[perception.idx(target.x(), target.y())] = lit_level::LOW;
    CHECK(r3d::crosshair_description(you, here, target) == furn_str_id("f_table")->name());
    perception.visibility_cache[perception.idx(target.x(), target.y())] = lit_level::DARK;
    you.toggle_map_memory();
    CHECK(r3d::crosshair_description(you, here, target).empty());
    CHECK(r3d::crosshair_description(you, here, tripoint_bub_ms(-1, -1, 0)).empty());
}
