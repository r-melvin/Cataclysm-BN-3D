#include "avatar.h"
#include "calendar.h"
#include "catch/catch.hpp"
#include "coordinates.h"
#include "map_helpers.h"
#include "state_helpers.h"
#include "status_report.h"

#include <string>

namespace {

auto contains(const std::string& text, const std::string& part) -> bool {
    return text.find(part) != std::string::npos;
}

} // namespace

TEST_CASE("status_report_describes_a_quiet_scene", "[status_report]") {
    clear_all_state();
    set_time(calendar::turn_zero + 12_hours);

    const auto report = status_report(get_avatar());

    CHECK(contains(report, "Place:"));
    CHECK(contains(report, "No hostiles in sight."));
    CHECK_FALSE(contains(report, "Hostiles in sight:"));
}

TEST_CASE("status_report_lists_visible_hostiles_with_distance_and_direction", "[status_report]") {
    clear_all_state();
    set_time(calendar::turn_zero + 12_hours);
    const auto& you = get_avatar();
    const auto zombie_pos = you.bub_pos() + point_east * 3;
    spawn_test_monster("mon_zombie", zombie_pos);

    const auto report = status_report(you);

    CHECK(contains(report, "Hostiles in sight:"));
    CHECK(contains(report, "3 tiles"));
    CHECK_FALSE(contains(report, "No hostiles in sight."));
}
