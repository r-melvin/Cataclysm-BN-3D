#if defined(TILES)

#    include "calendar.h"
#    include "cata_utility.h"
#    include "catch/catch.hpp"
#    include "r3d_sky.h"
#    include "weather/weather.h"

TEST_CASE("r3d_atmosphere_respects_weather_toggle_and_underground", "[r3d][weather]") {
    auto& weather = get_weather();
    const auto restore_weather = restore_on_out_of_scope(weather.weather_id);
    const auto restore_turn = restore_on_out_of_scope(calendar::turn);
    calendar::turn = calendar::turn_zero + 12_hours;
    weather.weather_id = weather_type_id("snowstorm");
    REQUIRE(weather.weather_id.is_valid());

    const auto outside = r3d::current_atmosphere(0, true);
    CHECK(outside.particles > 0);
    CHECK(outside.snow);
    CHECK_FALSE(outside.particle_id.empty());
    CHECK(outside.fog_density > 0.0f);

    for (const auto inside :
         {r3d::current_atmosphere(-1, true), r3d::current_atmosphere(0, false)}) {
        CHECK(inside.particles == 0);
        CHECK(inside.particle_id.empty());
        CHECK(inside.fog_density == 0.0f);
    }
    CHECK(r3d::sky_bodies(-1, outside).empty());
    CHECK(r3d::sky_bodies(0, outside).empty());
}

#endif
