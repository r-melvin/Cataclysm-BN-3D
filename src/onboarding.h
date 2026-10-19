#pragma once
#ifndef CATA_SRC_ONBOARDING_H
#define CATA_SRC_ONBOARDING_H

#include <string>

// Baby mode: guided hints for new players, an overlay on a normal game.  Lessons are JSON
// (type "onboarding_lesson", data/json/onboarding/); the rules live in onboarding_core.h.  This
// header is the game's side: loading, the hooks the game calls, and saving.  Nothing here changes
// game state; it reads what happens and writes messages, popups and its own progress.
// See docs/en/dev/guides/guided_onboarding.md.

class JsonObject;
class JsonOut;
class event_subscriber;

namespace onboarding
{

// JSON loading, called by DynamicDataLoader.
auto load_lesson( const JsonObject &jo, const std::string &src ) -> void;
auto finalize_lessons() -> void;
auto check_lessons() -> void;
auto reset_lessons() -> void;

// The event bus subscriber that feeds game events to the lessons.
auto subscriber() -> event_subscriber &; // *NOPAD*

// Forgets the progress of the previous character.  Called when a game is set up.
auto reset_progress() -> void;
/// Called before a new character is created. Guided Play enables hints under the default option.
auto prepare_new_game( bool guided ) -> void;

/// The Guided Play scenario when hints and the kit are enabled, otherwise empty.
/// Call after prepare_new_game().
auto start_scenario() -> std::string;

// Whether hints are on for this game at all.
auto enabled() -> bool;

// Called whenever the player is about to choose an action: checks lessons and shows any
// pending popup or hint.
auto before_input() -> void;
// Called when the player chose an action, with the action's id from keybindings.json.
auto on_action( const std::string &action_id ) -> void;
auto on_event( const std::string &name ) -> void;
// A one-off thing the game reports, such as "safe_mode_spotted".
auto on_signal( const std::string &name ) -> void;

// The character died: shows the death lesson, before the death screens.
auto on_death() -> void;
// Default-mode actions handled here ("onboarding_checklist", "EXPLAIN_SCREEN"); returns
// whether the action was one of them.
auto handle_action( const std::string &action ) -> bool;
// The checklist of all lessons, to read them again, show them again or mark them done.
auto show_checklist() -> void;
// Explains the screen whose input context has the given category.  Returns false, doing
// nothing, when hints are off.
auto explain_screen( const std::string &category ) -> bool;
// JSON loading of "onboarding_screen" (the explain-this-screen texts).
auto load_screen( const JsonObject &jo, const std::string &src ) -> void;

// The sidebar's "Next" line, empty when there is nothing to show.
// Wrapped to the given width, at most three lines.
auto sidebar_text( int width ) -> std::string;

// Saved with the avatar.
auto serialize( JsonOut &json ) -> void;
auto deserialize( const JsonObject &jo ) -> void;

} // namespace onboarding

#endif // CATA_SRC_ONBOARDING_H
