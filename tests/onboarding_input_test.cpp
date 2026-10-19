#include "catch/catch.hpp"
#include "input.h"
#include "onboarding_input.h"

#include <algorithm>

TEST_CASE("onboarding_hints_display_bound_function_keys", "[onboarding][input]") {
    // Use the shipped bindings loaded through the native input manager, including F1/F2.
    const auto text = onboarding::expand_keybinds(
        "<keybind:onboarding_checklist> / <keybind:EXPLAIN_SCREEN>");
    CHECK(text.find("F1") != std::string::npos);
    CHECK(text.find("F2") != std::string::npos);
}

TEST_CASE("onboarding_popup_choices_have_native_bindings", "[onboarding][input]") {
    for (const auto* action : {"GOT_IT", "LATER", "STOP_HINTS"}) {
        CAPTURE(action);
        CHECK_FALSE(inp_mngr.get_input_for_action(action, "ONBOARDING_POPUP").empty());
    }
}

TEST_CASE("onboarding_confirm_accepts_gamepad_button_zero", "[onboarding][input]") {
    const auto& bindings = inp_mngr.get_input_for_action("GOT_IT", "ONBOARDING_POPUP");
    CHECK(std::ranges::any_of(bindings, [](const auto& event) {
        return event.type == input_event_t::gamepad && event.sequence.size() == 1
            && event.get_first_input() == JOY_0;
    }));
}
