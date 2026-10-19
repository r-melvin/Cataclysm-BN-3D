#include "catch/catch.hpp"
#include "filesystem.h"
#include "input.h"
#include "path_info.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

class user_keybindings_file_guard {
public:
    ~user_keybindings_file_guard() {
        remove_file(PATH_INFO::user_keybindings());
        inp_mngr.init();
    }
};

auto write_user_keybindings(const std::string& entry) -> void {
    auto file = std::ofstream(PATH_INFO::user_keybindings(), std::ios::binary);
    REQUIRE(file.good());
    file << "[\n" << entry << "\n]\n";
}

auto read_user_keybindings() -> std::string {
    auto file = std::ifstream(PATH_INFO::user_keybindings(), std::ios::binary);
    REQUIRE(file.good());
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

auto npc_trade_right_keys() -> std::vector<char> {
    auto ctxt = input_context("NPC_TRADE");
    return ctxt.keys_bound_to("RIGHT");
}

auto contains_key(const std::vector<char>& keys, const char key) -> bool {
    return std::ranges::find(keys, key) != keys.end();
}

} // namespace

TEST_CASE("deleted local keybindings fall back to global bindings", "[input][keybindings]") {
    auto restore_keybindings = user_keybindings_file_guard();

    write_user_keybindings(R"({
  "type": "keybinding",
  "id": "RIGHT",
  "category": "NPC_TRADE",
  "bindings": []
})");
    inp_mngr.init();

    auto keys = npc_trade_right_keys();
    CHECK(keys.empty());

    write_user_keybindings(R"({
  "type": "keybinding",
  "id": "RIGHT",
  "category": "NPC_TRADE",
  "is_deleted": true,
  "bindings": []
})");
    inp_mngr.init();

    keys = npc_trade_right_keys();
    CHECK(contains_key(keys, 'l'));
    CHECK(contains_key(keys, '6'));

    inp_mngr.save();
    CHECK(read_user_keybindings().find("\"is_deleted\": true") != std::string::npos);
}

TEST_CASE("manual combat default bindings reserve backtab for the toggle", "[input][keybindings]") {
    const auto autoattack_key = inp_mngr.get_first_char_for_action("autoattack", "DEFAULTMODE");
    const auto toggle_key = inp_mngr.get_first_char_for_action(
        "toggle_manual_combat_mode",
        "DEFAUL"
        "TMOD"
        "E");
    const auto removed_manual_attack_key = inp_mngr.get_first_char_for_action(
        "manual_attack",
        "DEF"
        "AUL"
        "TMO"
        "D"
        "E");

    CHECK(autoattack_key == inp_mngr.get_keycode("TAB"));
    CHECK(toggle_key == inp_mngr.get_keycode("BACKTAB"));
    CHECK(removed_manual_attack_key == 0);
}

TEST_CASE("input_context active category stack", "[input]") {
    CHECK(input_context::get_active_category().empty());
    {
        auto ctxt_default = input_context("DEFAULTMODE");
        CHECK(input_context::get_active_category() == "DEFAULTMODE");

        {
            auto ctxt_menu = input_context("INVENTORY");
            CHECK(input_context::get_active_category() == "INVENTORY");
        }

        CHECK(input_context::get_active_category() == "DEFAULTMODE");
    }
    CHECK(input_context::get_active_category().empty());
}

TEST_CASE("wasd keys are not remapped to movement in defaultmode", "[input]") {
    auto ctxt = input_context("DEFAULTMODE");
    ctxt.register_action("wield");
    ctxt.register_action("apply");
    ctxt.register_action("smash");
    ctxt.register_action("drop");
    ctxt.register_action("UP");
    ctxt.register_action("DOWN");
    ctxt.register_action("LEFT");
    ctxt.register_action("RIGHT");

    CHECK(ctxt.input_to_action(input_event('w', input_event_t::keyboard)) == "wield");
    CHECK(ctxt.input_to_action(input_event('a', input_event_t::keyboard)) == "apply");
    CHECK(ctxt.input_to_action(input_event('s', input_event_t::keyboard)) == "smash");
    CHECK(ctxt.input_to_action(input_event('d', input_event_t::keyboard)) == "drop");
}

TEST_CASE("contextual interaction default keys in defaultmode", "[input]") {
    CHECK(inp_mngr.get_first_char_for_action("examine", "DEFAULTMODE") == 'e');
    CHECK(inp_mngr.get_first_char_for_action("open", "DEFAULTMODE") == 'o');
    CHECK(inp_mngr.get_first_char_for_action("close", "DEFAULTMODE") == 'c');
    CHECK(inp_mngr.get_first_char_for_action("pickup", "DEFAULTMODE") == 'g');
    CHECK(inp_mngr.get_first_char_for_action("control_vehicle", "DEFAULTMODE") == '^');
}
