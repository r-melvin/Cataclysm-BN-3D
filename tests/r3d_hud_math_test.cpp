#include "catch/catch.hpp"
#include "r3d_camera.h"
#include "r3d_hud_math.h"

#include <cmath>
#include <cstdint>

namespace {
auto approx(float a, float b, float eps = 1e-3f) -> bool { return std::fabs(a - b) <= eps; }
} // namespace

TEST_CASE("r3d_sky_direction_follows_calendar_convention", "[r3d]") {
    // Azimuth 0 is south: the sun stands to the south (world -y), at its altitude.
    const auto noon = r3d::sky_direction(0.0f, 0.5f);
    CHECK(approx(noon.x, 0.0f));
    CHECK(noon.y < 0.0f);
    CHECK(approx(noon.z, std::sin(0.5f)));
    // Positive azimuth turns towards the west (world -x).
    const auto west = r3d::sky_direction(1.5707963f, 0.0f);
    CHECK(approx(west.x, -1.0f));
    CHECK(approx(west.y, 0.0f));
    CHECK(approx(r3d::length(r3d::sky_direction(0.3f, 0.7f)), 1.0f));
}

TEST_CASE("r3d_sky_color_brightens_by_day_and_darkens_with_cloud", "[r3d]") {
    const auto night = r3d::sky_color(0.0f, 0.0f, false);
    const auto day = r3d::sky_color(1.0f, 0.0f, false);
    const auto overcast = r3d::sky_color(1.0f, 1.0f, false);
    CHECK(day.b > night.b);
    CHECK(day.r + day.g + day.b > overcast.r + overcast.g + overcast.b);
    // Overcast is grey: its channels are equal.
    CHECK(approx(overcast.r, overcast.b));
    CHECK(r3d::sky_color(0.0f, 0.0f, true).r > day.r);
}

TEST_CASE("r3d_wind_blows_away_from_its_direction", "[r3d]") {
    // A north wind (from 0 degrees) blows south: world -y.
    const auto north = r3d::wind_velocity(0, 5.0f);
    CHECK(approx(north.x, 0.0f));
    CHECK(approx(north.y, -5.0f));
    // An east wind blows west.
    const auto east = r3d::wind_velocity(90, 2.0f);
    CHECK(approx(east.x, -2.0f));
    CHECK(approx(east.y, 0.0f, 1e-3f));
}

TEST_CASE("r3d_particles_drift_with_the_wind_and_stay_in_their_box", "[r3d]") {
    auto f = r3d::particle_field{};
    f.half_extent = 10.0f;
    f.height = 4.0f;
    f.fall_speed = 3.0f;
    f.wind = r3d::wind_velocity(270, 2.0f); // from the west: drifts east
    auto sum_dx = 0.0;
    auto sum_dz = 0.0;
    for (auto i = uint32_t{0}; i < 200; ++i) {
        for (float t : {0.0f, 0.37f, 5.0f, 123.4f}) {
            const auto p = r3d::particle_position(
                {.index = i, .seconds = t, .field = f, .world_offset = {3.5f, -2.0f, 0.0f}});
            CHECK(p.x >= -10.0f);
            CHECK(p.x < 10.0f + 1e-3f);
            CHECK(p.y >= -10.0f);
            CHECK(p.y < 10.0f + 1e-3f);
            CHECK(p.z >= 0.0f);
            CHECK(p.z < 4.0f + 1e-3f);
        }
        // Over a short step (no wrapping for most), particles move east and down.
        const auto a = r3d::particle_position({.index = i, .seconds = 1.0f, .field = f});
        const auto b = r3d::particle_position({.index = i, .seconds = 1.05f, .field = f});
        if (std::fabs(b.x - a.x) < 1.0f && std::fabs(b.z - a.z) < 1.0f) {
            sum_dx += b.x - a.x;
            sum_dz += b.z - a.z;
        }
    }
    CHECK(sum_dx > 0.0);
    CHECK(sum_dz < 0.0);
    // The same particle at the same time is in the same place: the picture does not flicker.
    CHECK(approx(r3d::particle_position({.index = 7, .seconds = 2.0f, .field = f}).x,
                 r3d::particle_position({.index = 7, .seconds = 2.0f, .field = f}).x));
}

TEST_CASE("r3d_fog_grows_with_sight_penalty_and_distance", "[r3d]") {
    CHECK(r3d::fog_density(1.0f) == 0.0f);
    CHECK(r3d::fog_density(1.7f) > r3d::fog_density(1.1f));
    const auto d = r3d::fog_density(1.7f);
    CHECK(r3d::fog_amount(0.0f, d) == 0.0f);
    CHECK(r3d::fog_amount(10.0f, d) < r3d::fog_amount(40.0f, d));
    CHECK(r3d::fog_amount(200.0f, d) > 0.99f);
}

TEST_CASE("r3d_wind_band_ignores_flicker_across_an_edge", "[r3d]") {
    CHECK(r3d::wind_band(0, -1) == 0);
    CHECK(r3d::wind_band(3, -1) == 1);
    CHECK(r3d::wind_band(30, -1) == 3);
    // Hovering at the 13 mph edge keeps the band last reported.
    CHECK(r3d::wind_band(12, 2) == 2);
    CHECK(r3d::wind_band(13, 1) == 1);
    CHECK(r3d::wind_band(14, 1) == 1);
    // A clear step past it changes the band.
    CHECK(r3d::wind_band(15, 1) == 2);
    CHECK(r3d::wind_band(10, 2) == 1);
    CHECK(r3d::wind_band(0, 2) == 0);
}

TEST_CASE("r3d_bearings_match_the_camera", "[r3d]") {
    CHECK(approx(r3d::map_bearing(0.0f, -1.0f), 0.0f));      // north
    CHECK(approx(r3d::map_bearing(1.0f, 0.0f), 1.5707963f)); // east
    CHECK(approx(r3d::map_bearing(0.0f, 1.0f), 3.1415927f)); // south
    CHECK(approx(r3d::map_bearing(-1.0f, 0.0f), 4.712389f)); // west
    for (auto step = -12; step <= 12; ++step) {
        auto cam = r3d::camera{};
        cam.yaw = 0.2617994f * static_cast<float>(step);
        const auto fwd = r3d::camera_forward_bearing(cam.yaw);
        const auto eighths = static_cast<float>(cam.forward_eighths()) * 0.7853982f;
        CHECK(std::fabs(r3d::bearing_delta(eighths, fwd)) <= 0.3927f + 1e-3f);
    }
    CHECK(approx(r3d::bearing_delta(6.1f, 0.1f), 0.1f + 6.2831853f - 6.1f));
    CHECK(approx(r3d::bearing_delta(0.1f, 6.1f), -(0.1f + 6.2831853f - 6.1f)));
}

TEST_CASE("r3d_edge_marks_sit_on_the_border_and_point_out", "[r3d]") {
    // On screen: no mark.
    CHECK(r3d::place_edge_mark(
              {.clip_x = 0.2f, .clip_y = -0.3f, .clip_w = 1.0f, .margin_x = 0.1f, .margin_y = 0.1f})
              .on_screen);
    // Far to the right: on the right border, pointing right.
    const auto right = r3d::place_edge_mark(
        {.clip_x = 5.0f, .clip_y = 0.5f, .clip_w = 1.0f, .margin_x = 0.1f, .margin_y = 0.1f});
    CHECK_FALSE(right.on_screen);
    CHECK(approx(right.x, 0.9f));
    CHECK(approx(right.y, 0.09f));
    CHECK(std::fabs(right.angle) < 0.2f);
    // Up and left: on the top border, pointing up-left.
    const auto up = r3d::place_edge_mark(
        {.clip_x = -1.0f, .clip_y = 4.0f, .clip_w = 1.0f, .margin_x = 0.1f, .margin_y = 0.1f});
    CHECK(approx(up.y, 0.9f));
    CHECK(up.x < 0.0f);
    CHECK(up.angle > 1.5707963f);
    // Behind the camera, the projection is mirrored back.
    const auto behind = r3d::place_edge_mark(
        {.clip_x = 0.5f, .clip_y = 0.0f, .clip_w = -1.0f, .margin_x = 0.1f, .margin_y = 0.1f});
    CHECK_FALSE(behind.on_screen);
    CHECK(behind.x < 0.0f);
    const auto straight_back = r3d::place_edge_mark(
        {.clip_x = 0.0f, .clip_y = 0.0f, .clip_w = -1.0f, .margin_x = 0.1f, .margin_y = 0.1f});
    CHECK(approx(straight_back.y, -0.9f));
}
