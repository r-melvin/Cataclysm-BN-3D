#include "avatar.h"
#include "calendar.h"
#include "cata_utility.h"
#include "catch/catch.hpp"
#include "fstream_utils.h"
#include "json.h"
#include "onboarding.h"
#include "options_helpers.h"

#include <string>

namespace {
auto saved_hints() -> std::string {
    return serialize_wrapper([](JsonOut& out) { onboarding::serialize(out); });
}

auto load_hints(const std::string& state) -> void {
    deserialize_wrapper([](JsonIn& in) { onboarding::deserialize(in.get_object()); }, state);
}
} // namespace

TEST_CASE("onboarding_native_modes_distinguish_guided_characters", "[onboarding][integration]") {
    auto cleanup = on_out_of_scope([]() { onboarding::reset_progress(); });
    const auto mode = GENERATE("off", "play_now", "hints", "full");
    const auto option = override_option("ONBOARDING_HINTS", mode);
    CAPTURE(mode);
    onboarding::prepare_new_game(false);
    CHECK(onboarding::enabled() == (std::string(mode) == "hints" || std::string(mode) == "full"));
    onboarding::prepare_new_game(true);
    CHECK(onboarding::enabled() == (std::string(mode) != "off"));
}

TEST_CASE(
    "onboarding_native_progress_survives_loading_and_resets_for_another_character",
    "[onboarding][integration][save]") {
    auto cleanup = on_out_of_scope([]() { onboarding::reset_progress(); });
    const auto option = override_option("ONBOARDING_HINTS", "play_now");
    load_hints(
        R"({"guided":true,"lessons":{"ob_move":{"status":"active","progress":[4],"activated":12,"snoozed_until":600,"reminders":1}}})");
    REQUIRE(onboarding::enabled());
    const auto state = saved_hints();
    onboarding::prepare_new_game(false);
    CHECK_FALSE(onboarding::enabled());
    CHECK(saved_hints().find("ob_move") == std::string::npos);
    load_hints(state);
    CHECK(onboarding::enabled());
    CHECK(saved_hints() == state);
    // Older saves have no onboarding record; avatar::load calls this same reset.
    onboarding::reset_progress();
    CHECK_FALSE(onboarding::enabled());
    CHECK(saved_hints().find("ob_move") == std::string::npos);
}

TEST_CASE("onboarding_hint_refresh_does_not_spend_game_time", "[onboarding][integration]") {
    auto cleanup = on_out_of_scope([]() { onboarding::reset_progress(); });
    const auto option = override_option("ONBOARDING_HINTS", "hints");
    const auto moves = get_avatar().moves;
    const auto turn = calendar::turn;
    SECTION("fresh character") { onboarding::prepare_new_game(false); }
    SECTION("a previously displayed popup survives a save or mode switch") {
        load_hints(R"({"guided":false,"lessons":{"ob_welcome":{"status":"active"}}})");
    }
    onboarding::before_input();
    CHECK(get_avatar().moves == moves);
    CHECK(calendar::turn == turn);
    auto state = std::string(saved_hints());
    deserialize_wrapper(
        [](JsonIn& in) {
            const auto root = in.get_object();
            root.allow_omitted_members();
            const auto lessons = root.get_object("lessons");
            lessons.allow_omitted_members();
            const auto welcome = lessons.get_object("ob_welcome");
            welcome.allow_omitted_members();
            const auto movement = lessons.get_object("ob_move");
            movement.allow_omitted_members();
            CHECK(welcome.get_string("status") == "done");
            CHECK(movement.get_string("status") == "active");
        },
        state);
}

TEST_CASE("onboarding_start_kit_is_optional", "[onboarding][integration]") {
    auto cleanup = on_out_of_scope([]() { onboarding::reset_progress(); });
    const auto mode = override_option("ONBOARDING_HINTS", "play_now");
    const auto kit_enabled = GENERATE("false", "true");
    const auto kit = override_option("ONBOARDING_SOFT_START", kit_enabled);
    onboarding::prepare_new_game(true);
    CHECK(onboarding::start_scenario()
          == (std::string(kit_enabled) == "true" ? "evacuee_guided" : ""));
    onboarding::prepare_new_game(false);
    CHECK(onboarding::start_scenario().empty());
}
