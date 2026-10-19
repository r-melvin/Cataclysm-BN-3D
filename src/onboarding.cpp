#include "onboarding.h"
#include "onboarding_input.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "action.h"
#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "cata_variant.h"
#include "character.h"
#include "character_functions.h"
#include "character_id.h"
#include "color.h"
#include "condition.h"
#include "coordinates.h"
#include "creature.h"
#include "debug.h"
#include "dialogue.h"
#include "enum_conversions.h"
#include "event.h"
#include "event_bus.h"

#include "game.h"
#include "gamemode.h"
#include "generic_factory.h"
#include "input.h"
#include "item.h"

#include "json.h"
#include "map/map.h"
#include "map/mapdata.h"
#include "messages.h"
#include "monster.h"
#include "onboarding_core.h"
#include "options.h"
#include "output.h"
#include "npc.h"
#include "point.h"
#include "popup.h"
#include "scenario.h"
#include "string_formatter.h"

#include "translations.h"
#include "type_id.h"
#include "ui.h"
#include "ui_manager.h"
#include "units.h"
#include "worldfactory.h"

class onboarding_lesson;
using onboarding_lesson_id = string_id<onboarding_lesson>;
class onboarding_screen;
using onboarding_screen_id = string_id<onboarding_screen>;

// Text for a particular UI, such as the 3D view.  The first variant whose flags all hold
// replaces the title, short and text it sets; every variant whose flags hold adds its "extra"
// paragraph after the text.
struct ob_text_variant {
    std::vector<std::string> when;
    std::optional<translation> title;
    std::optional<translation> short_text;
    std::optional<translation> text;
    std::optional<translation> extra;
};

namespace
{
auto load_variants( const JsonObject &jo, std::vector<ob_text_variant> &out ) -> void;
auto pick_title( const translation &base,
                 const std::vector<ob_text_variant> &variants ) -> const translation &; // *NOPAD*
auto pick_short( const translation &base,
                 const std::vector<ob_text_variant> &variants ) -> const translation &; // *NOPAD*
auto pick_text( const translation &base,
                const std::vector<ob_text_variant> &variants ) -> std::string;
auto check_ui_flags( const std::string &what, const std::vector<std::string> &flags,
                     const std::vector<ob_text_variant> &variants ) -> void;

}

// One lesson as loaded from JSON (type "onboarding_lesson").
class onboarding_lesson
{
    public:
        onboarding_lesson_id id;
        std::vector<std::pair<onboarding_lesson_id, mod_id>> src;
        bool was_loaded = false;

        int phase = 1;
        int order = 0;
        ob_presentation presentation = ob_presentation::hint;
        translation title;
        // One line for the sidebar and for reminders.
        translation short_text;
        // The full explanation.
        translation text;
        std::vector<std::string> prerequisites;
        std::vector<ob_matcher> trigger;
        std::vector<ob_matcher> done_when;
        int snooze_turns = 600;
        // UI flags (see ui_flag()) that must all hold for the lesson to apply at all.
        std::vector<std::string> only_when;
        // An action the lesson is about; without it (a build that lacks it, or no key bound)
        // the lesson does not apply.
        std::string requires_action;
        // Text for particular UIs; see ob_text_variant.
        std::vector<ob_text_variant> variants;
        // Dialogue conditions used by "condition" matchers, by index.
        std::vector < std::function < auto( const dialogue & ) -> bool >> conditions;

        auto load( const JsonObject &jo, std::string_view /*src*/ ) -> void;
        auto check() const -> void;
        auto def() const -> ob_lesson_def;
        // Whether the lesson applies to the current game and UI.
        auto available() const -> bool;
        // Title, short line and text for the UI in use.
        auto get_title() const -> const translation &; // *NOPAD*
        auto get_short() const -> const translation &; // *NOPAD*
        // The full text for the UI in use, with the extra paragraphs of matching variants.
        auto get_text() const -> std::string;

    private:
        auto load_matchers( const JsonObject &jo, const std::string &member,
                            std::vector<ob_matcher> &out ) -> void;
};

// What "Explain this screen" says for one input context (type "onboarding_screen"; the id is
// the input context's category, such as "INVENTORY").
class onboarding_screen
{
    public:
        onboarding_screen_id id;
        std::vector<std::pair<onboarding_screen_id, mod_id>> src;
        bool was_loaded = false;

        translation title;
        translation text;
        std::vector<ob_text_variant> variants;

        auto load( const JsonObject &jo, std::string_view /*src*/ ) -> void {
            mandatory( jo, was_loaded, "title", title );
            mandatory( jo, was_loaded, "text", text );
            if( jo.has_array( "variants" ) ) {
                load_variants( jo, variants );
            }
        }
        auto check() const -> void {
            check_ui_flags( "onboarding_screen " + id.str(), {}, variants );
        }
};

namespace
{

auto lesson_factory = generic_factory<onboarding_lesson>( "onboarding_lesson" );
auto screen_factory = generic_factory<onboarding_screen>( "onboarding_screen" );

// States the game can poll for, used by "state" matchers.
auto known_states() -> const std::set<std::string> & // *NOPAD*
{
    static const auto states = std::set<std::string> {
        "hungry", "thirsty", "tired", "standing_on_items", "items_nearby", "adjacent_door", "adjacent_examinable",
        "hostile_in_view", "worn_storage", "dark_here", "carries_book", "low_morale",
        "wields_gun", "corpse_here", "is_wet", "adjacent_vehicle", "npc_in_view", "on_stairs"
    };
    return states;
}

// Signals the game sends, used by "signal" matchers.
const auto legacy_events = std::set<std::string> { "avatar_enters_omt", "character_eats_item", "character_consumes_item", "character_starts_activity", "character_falls_asleep" };

auto known_signals() -> const std::set<std::string> & // *NOPAD*
{
    static const auto signals = std::set<std::string> {
        "safe_mode_spotted", "avatar_died"
    };
    return signals;
}

// Option helpers that treat a missing option as off, so this file builds and runs on branches
// with fewer 3D options.
auto option_on( const std::string &name ) -> bool
{ return get_options().has_option( name ) && get_option<bool>( name ); }

auto option_value( const std::string &name ) -> std::string
{ return get_options().has_option( name ) ? get_option<std::string>( name ) : std::string(); }

// UI flags lessons can branch on ("only_when", "variants").  See docs/en/dev/guides/guided_onboarding.md.
auto known_ui_flags() -> const std::set<std::string> & // *NOPAD*
{
    static const auto flags = std::set<std::string> {
        "classic", "r3d", "r3d_orbit", "r3d_first_person", "r3d_wasd", "r3d_cutaway",
        "r3d_compass", "r3d_edge_alert"
    };
    return flags;
}

auto ui_flag( const std::string &flag ) -> bool
{
    const auto r3d = static_cast<bool>( option_on( "RENDERER_3D" ) );
    const auto first_person = static_cast<bool>( r3d &&
                              option_value( "R3D_CAMERA_MODE" ) == "first_person" );
    if( flag == "classic" ) {
        return !r3d;
    } else if( flag == "r3d" ) {
        return r3d;
    } else if( flag == "r3d_orbit" ) {
        return r3d && !first_person;
    } else if( flag == "r3d_first_person" ) {
        return first_person;
    } else if( flag == "r3d_wasd" ) {
        return r3d && option_on( "R3D_WASD" );
    } else if( flag == "r3d_cutaway" ) {
        return r3d && !first_person && option_on( "R3D_CUTAWAY" );
    } else if( flag == "r3d_compass" ) {
        const auto compass = std::string( option_value( "R3D_FP_COMPASS" ) );
        return first_person && !compass.empty() && compass != "off";
    } else if( flag == "r3d_edge_alert" ) {
        return first_person && option_on( "R3D_FP_EDGE_ALERT" );
    }
    return false;
}

auto all_ui_flags( const std::vector<std::string> &flags ) -> bool
{ return std::ranges::all_of( flags, ui_flag ); }

// Whether an action exists in this build and has a key.
auto action_bound( const std::string &action ) -> bool
{
    const auto ctxt = input_context( "DEFAULTMODE" );
    return !inp_mngr.get_input_for_action( action, "DEFAULTMODE" ).empty();
}

} // namespace

/// @relates string_id
template<>
auto string_id<onboarding_lesson>::obj() const -> const onboarding_lesson & // *NOPAD* // *NOPAD*
{ return lesson_factory.obj( *this ); }

/// @relates string_id
template<>
auto string_id<onboarding_lesson>::is_valid() const -> bool
{ return lesson_factory.is_valid( *this ); }

/// @relates string_id
template<>
auto string_id<onboarding_screen>::obj() const -> const onboarding_screen & // *NOPAD* // *NOPAD*
{ return screen_factory.obj( *this ); }

/// @relates string_id
template<>
auto string_id<onboarding_screen>::is_valid() const -> bool
{ return screen_factory.is_valid( *this ); }

namespace
{
auto load_variants( const JsonObject &jo, std::vector<ob_text_variant> &out ) -> void
{
    out.clear();
    for( JsonObject vo : jo.get_array( "variants" ) ) {
        auto v = ob_text_variant{};
        vo.read( "when", v.when );
        auto t = translation{};
        if( vo.read( "title", t ) ) {
            v.title = t;
        }
        if( vo.read( "short", t ) ) {
            v.short_text = t;
        }
        if( vo.read( "text", t ) ) {
            v.text = t;
        }
        if( vo.read( "extra", t ) ) {
            v.extra = t;
        }
        out.push_back( v );
    }
}

auto pick_title( const translation &base,
                 const std::vector<ob_text_variant> &variants ) -> const translation & // *NOPAD*
{
    for( const auto &v : variants ) {
        if( v.title && all_ui_flags( v.when ) ) {
            return *v.title;
        }
    }
    return base;
}

auto pick_short( const translation &base,
                 const std::vector<ob_text_variant> &variants ) -> const translation & // *NOPAD*
{
    for( const auto &v : variants ) {
        if( v.short_text && all_ui_flags( v.when ) ) {
            return *v.short_text;
        }
    }
    return base;
}

auto pick_text( const translation &base,
                const std::vector<ob_text_variant> &variants ) -> std::string
{
    auto chosen = &base;
    for( const auto &v : variants ) {
        if( v.text && all_ui_flags( v.when ) ) {
            chosen = &*v.text;
            break;
        }
    }
    auto ret = std::string( chosen->translated() );
    for( const auto &v : variants ) {
        if( v.extra && all_ui_flags( v.when ) ) {
            ret += "\n\n" + v.extra->translated();
        }
    }
    return ret;
}

auto check_ui_flags( const std::string &what, const std::vector<std::string> &flags,
                     const std::vector<ob_text_variant> &variants ) -> void
{
    const auto check = [&what]( const std::vector<std::string> &list ) {
        for( const auto &f : list ) {
            if( !known_ui_flags().count( f ) ) {
                debugmsg( "%s uses unknown UI flag %s", what, f );
            }
        }
    };
    check( flags );
    for( const auto &v : variants ) {
        check( v.when );
    }
}

} // namespace

auto onboarding_lesson::load_matchers( const JsonObject &jo, const std::string &member,
                                       std::vector<ob_matcher> &out ) -> void
{
    if( !jo.has_member( member ) ) {
        return;
    }
    out.clear();
    for( JsonObject m : jo.get_array( member ) ) {
        ob_matcher match;
        match.count = m.get_int( "count", 1 );
        if( m.has_member( "start" ) ) {
            match.kind = ob_input_kind::start;
            m.get_bool( "start" );
        } else if( m.has_member( "action" ) ) {
            match.kind = ob_input_kind::action;
            match.name = m.get_string( "action" );
        } else if( m.has_member( "event" ) ) {
            match.kind = ob_input_kind::event;
            match.name = m.get_string( "event" );
        } else if( m.has_member( "signal" ) ) {
            match.kind = ob_input_kind::signal;
            match.name = m.get_string( "signal" );
        } else if( m.has_member( "state" ) ) {
            match.kind = ob_input_kind::state;
            match.name = m.get_string( "state" );
        } else if( m.has_member( "after_turns" ) ) {
            match.kind = ob_input_kind::after_turns;
            match.count = m.get_int( "after_turns" );
        } else if( m.has_member( "condition" ) ) {
            match.kind = ob_input_kind::condition;
            std::function < auto( const dialogue & ) -> bool > cond;
            read_condition( m, "condition", cond, false );
            match.condition_index = static_cast<int>( conditions.size() );
            conditions.push_back( cond );
        } else {
            m.throw_error( "a matcher needs one of start, action, event, signal, state, "
                           "after_turns or condition" );
        }
        match.while_shown = m.get_bool( "while_shown", false );
        out.push_back( match );
    }
}

auto onboarding_lesson::load( const JsonObject &jo, std::string_view /*src*/ ) -> void
{
    optional( jo, was_loaded, "phase", phase, 1 );
    optional( jo, was_loaded, "order", order, 0 );
    if( jo.has_member( "presentation" ) ) {
        const auto p = std::string( jo.get_string( "presentation" ) );
        if( p == "hint" ) {
            presentation = ob_presentation::hint;
        } else if( p == "popup" ) {
            presentation = ob_presentation::popup;
        } else if( p == "silent" ) {
            presentation = ob_presentation::silent;
        } else {
            jo.throw_error( "expected hint, popup or silent", "presentation" );
        }
    }
    mandatory( jo, was_loaded, "title", title );
    mandatory( jo, was_loaded, "short", short_text );
    mandatory( jo, was_loaded, "text", text );
    optional( jo, was_loaded, "prerequisites", prerequisites );
    optional( jo, was_loaded, "snooze_turns", snooze_turns, 600 );
    optional( jo, was_loaded, "only_when", only_when );
    optional( jo, was_loaded, "requires_action", requires_action );
    if( jo.has_array( "variants" ) ) {
        load_variants( jo, variants );
    }
    load_matchers( jo, "trigger", trigger );
    load_matchers( jo, "done_when", done_when );
}

auto onboarding_lesson::available() const -> bool
{
    return all_ui_flags( only_when ) &&
           ( requires_action.empty() || action_bound( requires_action ) );
}

auto onboarding_lesson::get_title() const -> const translation & // *NOPAD*
{ return pick_title( title, variants ); }

auto onboarding_lesson::get_short() const -> const translation & // *NOPAD*
{ return pick_short( short_text, variants ); }

auto onboarding_lesson::get_text() const -> std::string
{ return pick_text( text, variants ); }

auto onboarding_lesson::check() const -> void
{
    for( const auto &pre : prerequisites ) {
        if( !onboarding_lesson_id( pre ).is_valid() ) {
            debugmsg( "onboarding_lesson %s has unknown prerequisite %s", id.str(), pre );
        }
    }
    const auto check_list = [this]( const std::vector<ob_matcher> &list, bool is_trigger ) {
        for( const auto &m : list ) {
            switch( m.kind ) {
                case ob_input_kind::action:
                    // Actions of lessons for a particular UI or build (such as the 3D view's
                    // first-person keys) may be missing from this build on purpose.
                    if( only_when.empty() && m.name != requires_action &&
                        look_up_action( m.name ) == ACTION_NULL ) {
                        debugmsg( "onboarding_lesson %s uses unknown action %s", id.str(), m.name );
                    }
                    break;
                case ob_input_kind::event:
                    if( !io::enum_is_valid<event_type>( m.name ) && !legacy_events.contains( m.name ) ) {
                        debugmsg( "onboarding_lesson %s uses unknown event %s", id.str(), m.name );
                    }
                    break;
                case ob_input_kind::signal:
                    if( !known_signals().count( m.name ) ) {
                        debugmsg( "onboarding_lesson %s uses unknown signal %s", id.str(), m.name );
                    }
                    break;
                case ob_input_kind::state:
                    if( !known_states().count( m.name ) ) {
                        debugmsg( "onboarding_lesson %s uses unknown state %s", id.str(), m.name );
                    }
                    break;
                case ob_input_kind::start:
                    if( !is_trigger ) {
                        debugmsg( "onboarding_lesson %s uses start in done_when", id.str() );
                    }
                    break;
                case ob_input_kind::after_turns:
                case ob_input_kind::condition:
                    break;
            }
        }
    };
    check_list( trigger, true );
    check_list( done_when, false );
    check_ui_flags( "onboarding_lesson " + id.str(), only_when, variants );
}

auto onboarding_lesson::def() const -> ob_lesson_def
{
    auto d = ob_lesson_def{};
    d.id = id.str();
    d.phase = phase;
    d.order = order;
    d.presentation = presentation;
    d.prerequisites = prerequisites;
    d.trigger = trigger;
    d.done_when = done_when;
    d.snooze_turns = snooze_turns;
    return d;
}

namespace
{

auto status_to_string( ob_status s ) -> std::string
{
    switch( s ) {
        case ob_status::locked:
            return "locked";
        case ob_status::waiting:
            return "waiting";
        case ob_status::queued:
            return "queued";
        case ob_status::active:
            return "active";
        case ob_status::done:
            return "done";
    }
    return "locked";
}

auto status_from_string( const std::string &s ) -> ob_status
{
    for( const auto st : {
             ob_status::locked, ob_status::waiting, ob_status::queued, ob_status::active,
             ob_status::done
         } ) {
        if( status_to_string( st ) == s ) {
            return st;
        }
    }
    return ob_status::locked;
}

auto now() -> int64_t
{ return to_turn<int>( calendar::turn ); }

// How much of baby mode is on for this game.
enum class ob_level : int {
    off,
    hints,
    full
};

// A friendlier name for a key, or an empty string for keys not worth naming next to the main
// one (the keypad copies of Enter and the number keys).
auto key_name( const std::string &raw ) -> std::string
{
    if( raw.rfind( "KEYPAD_", 0 ) == 0 ) {
        return std::string();
    } else if( raw == "RETURN" ) {
        return _( "Enter" );
    } else if( raw == "ESC" ) {
        return _( "Esc" );
    } else if( raw == "SPACE" ) {
        return _( "Space" );
    } else if( raw == "TAB" ) {
        return _( "Tab" );
    } else if( raw == "BACKSPACE" ) {
        return _( "Backspace" );
    }
    return raw;
}

// Expands <keybind:ACTION> and <keybind:CATEGORY:ACTION> into the keys currently bound, like
// the tags in help and dialogue text, but naming at most two keys so the hints stay short.
auto expand_keys( const std::string &raw ) -> std::string
{
    auto ret = std::string( raw );
    static const auto open = std::string( "<keybind:" );
    auto pos = size_t( 0 );
    while( ( pos = ret.find( open, pos ) ) != std::string::npos ) {
        const auto end = size_t( ret.find( '>', pos ) );
        if( end == std::string::npos ) {
            break;
        }
        const auto spec = std::string( ret.substr( pos + open.size(), end - pos - open.size() ) );
        auto category = std::string( "DEFAULTMODE" );
        auto action = std::string( spec );
        const auto split = size_t( spec.find( ':' ) );
        if( split != std::string::npos ) {
            category = spec.substr( 0, split );
            action = spec.substr( split + 1 );
        }
        const auto ctxt = input_context( category );
        const auto &keys = inp_mngr.get_input_for_action( action, category );
        auto names = std::vector<std::string> {};
        for( const auto &event : keys ) {
            if( event.type != input_event_t::keyboard ) { continue; }
            auto name = std::string{};
            for( const auto key : event.sequence ) {
                name += key_name( inp_mngr.get_keyname( key, event.type ) );
            }
            if( !name.empty() && names.size() < 2 && std::ranges::find( names, name ) == names.end() ) {
                names.push_back( name );
            }
        }
        // With the 3D view's WASD controls on, w a s d walk instead of doing what they are bound
        // to, so do not offer them for anything else.
        auto taken_by_wasd = bool( false );
        if( !names.empty() && ui_flag( "r3d_wasd" ) ) {
            std::erase_if( names, []( const auto & n ) { return n == "w" || n == "a" || n == "s" || n == "d"; } );
            taken_by_wasd = names.empty();
        }
        auto replacement = std::string{};
        if( taken_by_wasd ) {
            //~ Shown in a hint when the action's key walks in the 3D view; %s is the action menu key.
            replacement = string_format( _( "the action menu (%s)" ),
                                         expand_keys( "<keybind:action_menu>" ) );
        } else if( names.empty() ) {
            //~ Shown in a hint for an action that has no key; %s is the action's name.
            replacement = colorize( string_format( _( "(no key set for \"%s\")" ),
                                                   ctxt.get_action_name( action ) ), c_red );
        } else {
            for( auto i = size_t( 0 ); i < names.size(); ++i ) {
                if( i > 0 ) {
                    //~ Between two keys in a hint: "y or 7".
                    replacement += _( " or " );
                }
                replacement += colorize( names[i], c_yellow );
            }
        }
        ret.replace( pos, end - pos + 1, replacement );
        pos += replacement.size();
    }
    return ret;
}

// Wraps text to the given width, keeping its paragraphs, for menus that do not wrap.
auto wrap_text( const std::string &text, int width ) -> std::string
{
    auto ret = std::string{};
    auto start = size_t( 0 );
    while( start <= text.size() ) {
        auto end = size_t( text.find( '\n', start ) );
        if( end == std::string::npos ) {
            end = text.size();
        }
        const auto para = std::string( text.substr( start, end - start ) );
        if( para.empty() ) {
            ret += "\n";
        } else {
            for( const auto &line : foldstring( para, width ) ) {
                ret += line + "\n";
            }
        }
        start = end + 1;
    }
    while( !ret.empty() && ret.back() == '\n' ) {
        ret.pop_back();
    }
    return ret;
}

// The message log shows one line per message, so paragraphs become one line.
auto flatten( const std::string &text ) -> std::string
{
    auto ret = std::string{};
    ret.reserve( text.size() );
    for( auto i = size_t( 0 ); i < text.size(); ++i ) {
        if( text[i] != '\n' ) {
            ret += text[i];
        } else if( !ret.empty() && ret.back() != ' ' ) {
            ret += ' ';
        }
    }
    return ret;
}

class manager : public event_subscriber
{
    public:
        auto notify( const cata::event &e ) -> void override;
        using event_subscriber::notify;

        // Builds the tracker from the loaded lessons, keeping any progress already known.
        auto ensure_built() -> void;
        auto invalidate() -> void;
        auto reset_progress() -> void;
        // Marks lessons that do not apply to the current UI or build (see only_when).
        auto refresh_availability() -> void;
        auto level() const -> ob_level;
        auto poll( size_t lesson, const ob_matcher &match ) const -> bool;
        // Shows pending notices; returns whether a popup was answered.
        auto present() -> bool;
        auto show_popup( size_t lesson ) -> void;
        auto log_hint( size_t lesson, bool reminder ) -> void;

        onboarding_tracker tracker;
        // The lesson behind each tracker index.
        std::vector<onboarding_lesson_id> lesson_ids;
        bool built = false;
        // Progress waiting for the lessons to be loaded, or kept across a data reload.
        std::map<std::string, ob_lesson_state> pending_states;
        // The character was made with the Guided Play menu entry.
        bool guided = false;
        // The world is the Tutorial Game's.
        bool in_tutorial = false;
        // The player has had a first turn.  Events before it (the starting outfit being put
        // on, the start location being entered) are the game setting up, not the player.
        bool started = false;
        std::optional<tripoint_abs_omt> last_omt;
};

auto get_manager() -> manager & // *NOPAD*
{
    static auto m = manager{};
    return m;
}

auto manager::ensure_built() -> void
{
    if( built ) {
        return;
    }
    auto defs = std::vector<ob_lesson_def> {};
    lesson_ids.clear();
    for( const auto &l : lesson_factory.get_all() ) {
        defs.push_back( l.def() );
        lesson_ids.push_back( l.id );
    }
    tracker.set_lessons( std::move( defs ) );
    tracker.import_states( pending_states );
    pending_states.clear();
    built = true;
}

auto manager::refresh_availability() -> void
{
    for( auto i = size_t( 0 ); i < lesson_ids.size(); ++i ) {
        tracker.set_available( i, lesson_ids[i].is_valid() && lesson_ids[i]->available() );
    }
}

auto manager::invalidate() -> void
{
    if( built ) {
        pending_states = tracker.export_states();
    }
    built = false;
    lesson_ids.clear();
}

auto manager::reset_progress() -> void
{
    pending_states.clear();
    last_omt.reset();
    built = false;
    lesson_ids.clear();
    guided = false;
    started = false;
    tracker.take_notices();
    const auto *world = world_generator ? world_generator->active_world.get() : nullptr;
    in_tutorial = world != nullptr &&
                  std::ranges::find( world->info->active_mod_order,
                                     mod_id( "dda_tutorial" ) ) != world->info->active_mod_order.end();
}

auto manager::level() const -> ob_level
{
    if( in_tutorial || ( g && g->gametype() == special_game_type::TUTORIAL ) ) {
        // The Tutorial Game teaches in its own way.
        return ob_level::off;
    }
    const auto opt = std::string( get_option<std::string>( "ONBOARDING_HINTS" ) );
    if( opt == "full" ) {
        return ob_level::full;
    } else if( opt == "hints" ) {
        return ob_level::hints;
    } else if( opt == "play_now" ) {
        return guided ? ob_level::full : ob_level::off;
    }
    return ob_level::off;
}

auto manager::poll( size_t lesson, const ob_matcher &match ) const -> bool
{
    auto &u = get_avatar();
    if( match.kind == ob_input_kind::condition ) {
        if( lesson >= lesson_ids.size() || !lesson_ids[lesson].is_valid() ) {
            return false;
        }
        const auto &l = lesson_ids[lesson].obj();
        if( match.condition_index < 0 ||
            match.condition_index >= static_cast<int>( l.conditions.size() ) ) {
            return false;
        }
        auto d = dialogue{};
        d.alpha = &u;
        return l.conditions[match.condition_index]( d );
    }

    auto &here = get_map();
    const auto pos = tripoint_bub_ms( u.bub_pos() );
    const auto &name = match.name;
    if( name == "hungry" ) {
        const auto color = u.get_hunger_description().second;
        return color == c_yellow || color == c_light_red || color == c_red;
    } else if( name == "thirsty" ) {
        return u.get_thirst() > 40;
    } else if( name == "tired" ) {
        return u.get_fatigue() >= 191;
    } else if( name == "standing_on_items" ) {
        return here.has_items( pos ) && here.accessible_items( pos );
    } else if( name == "items_nearby" ) {
        for( const auto &p : here.points_in_radius( pos, 1 ) ) {
            if( here.has_items( p ) && here.accessible_items( p ) && u.sees( p ) ) {
                return true;
            }
        }
    } else if( name == "adjacent_door" ) {
        for( const auto &p : here.points_in_radius( pos, 1 ) ) {
            if( p == pos ) {
                continue;
            }
            const auto &ter = here.ter( p ).obj();
            const auto &furn = here.furn( p ).obj();
            if( ter.open || ter.close || furn.open || furn.close ) {
                return true;
            }
        }
    } else if( name == "adjacent_examinable" ) {
        for( const auto &p : here.points_in_radius( pos, 1 ) ) {
            if( here.has_furn( p ) && here.furn( p ).obj().examine != nullptr ) {
                return true;
            }
        }
    } else if( name == "hostile_in_view" ) {
        for( monster &critter : g->all_monsters() ) {
            if( critter.attitude_to( u ) == Attitude::A_HOSTILE && u.sees( critter ) ) {
                return true;
            }
        }
    } else if( name == "dark_here" ) {
        // Too dark to read or craft well.
        return character_funcs::fine_detail_vision_mod( u ) > 4.0f;
    } else if( name == "carries_book" ) {
        return u.has_item_with( []( const item & it ) {
            return it.is_book();
        } );
    } else if( name == "low_morale" ) {
        return u.get_morale_level() < -10;
    } else if( name == "wields_gun" ) {
        const auto &wielded = u.primary_weapon();
        return wielded.is_gun();
    } else if( name == "corpse_here" ) {
        for( const auto *it : here.i_at( pos ) ) {
            if( it->is_corpse() ) {
                return true;
            }
        }
    } else if( name == "is_wet" ) {
        for( const auto &bp : u.get_all_body_parts() ) {
            if( u.get_part( bp ).get_wetness() > 0 ) {
                return true;
            }
        }
    } else if( name == "adjacent_vehicle" ) {
        for( const auto &p : here.points_in_radius( pos, 1 ) ) {
            if( here.veh_at( p ) ) {
                return true;
            }
        }
    } else if( name == "npc_in_view" ) {
        for( auto &guy : g->all_npcs() ) {
            if( !guy.is_hallucination() && u.sees( guy ) ) {
                return true;
            }
        }
    } else if( name == "on_stairs" ) {
        return here.has_flag( "GOES_DOWN", pos ) ||
               here.has_flag( "GOES_UP", pos );
    } else if( name == "worn_storage" ) {
        // Clothes have small pockets; anything that holds five liters is a bag.
        for( const auto *worn : u.worn ) {
            if( worn->get_storage() >= units::from_liter( 5 ) ) {
                return true;
            }
        }
    }
    return false;
}

auto manager::log_hint( size_t lesson, bool reminder ) -> void
{
    const auto &l = lesson_ids[lesson].obj();
    const auto body = std::string( reminder ? l.get_short().translated() : l.get_text() );
    //~ Prefix of a guided hint in the message log; %1$s is the lesson title, %2$s its text.
    add_msg( m_info, _( "%1$s %2$s" ),
             colorize( string_format( _( "[Hint: %s]" ), l.get_title().translated() ), c_light_green ),
             flatten( expand_keys( body ) ) );
}

auto manager::show_popup( size_t lesson ) -> void
{
    const auto &l = lesson_ids[lesson].obj();
    // Keep a copy in the message log so it can be read again.
    add_msg( m_info, "%s %s",
             colorize( string_format( _( "[Hint: %s]" ), l.get_title().translated() ), c_light_green ),
             expand_keys( l.get_short().translated() ) );

    // On the first turn the map has not been drawn yet; draw it so the popup has a background.
    ui_manager::redraw();
    const auto body = std::string( colorize( l.get_title().translated(), c_light_green ) + "\n\n" +
                                   expand_keys( l.get_text() ) );
    const auto res = query_popup::result( query_popup()
                                          .context( "ONBOARDING_POPUP" )
                                          .message( "%s", body )
                                          .option( "GOT_IT" )
                                          .option( "LATER" )
                                          .option( "STOP_HINTS" )
                                          .allow_cancel( true )
                                          .query() );
    if( res.action == "LATER" ) {
        tracker.snooze( lesson, now() );
    } else if( res.action == "STOP_HINTS" ) {
        tracker.complete( lesson, now() );
        get_options().get_option( "ONBOARDING_HINTS" ).setValue( "off" );
        get_options().save();
        add_msg( m_info, _( "Guided hints are off.  You can turn them back on in Settings > "
                            "Interface > Guided hints." ) );
    } else {
        tracker.complete( lesson, now() );
    }
}

auto manager::present() -> bool
{
    auto answered = bool( false );
    const auto lvl = ob_level( level() );
    for( const auto &n : tracker.take_notices() ) {
        if( lvl == ob_level::off || n.lesson >= lesson_ids.size() ||
            !lesson_ids[n.lesson].is_valid() ) {
            continue;
        }
        const auto popup = bool( lvl == ob_level::full &&
                                 tracker.def( n.lesson ).presentation == ob_presentation::popup );
        switch( n.type ) {
            case ob_notice_type::shown:
            case ob_notice_type::reminder:
                if( popup ) {
                    show_popup( n.lesson );
                    answered = true;
                } else {
                    log_hint( n.lesson, n.type == ob_notice_type::reminder );
                    // Hints-only mode has no popup answer to finish these lessons.
                    // Logging the lesson must still unlock the next steps.
                    if( lvl == ob_level::hints &&
                        tracker.def( n.lesson ).presentation == ob_presentation::popup ) {
                        tracker.complete( n.lesson, now() );
                        answered = true;
                    }
                }
                break;
            case ob_notice_type::done: {
                const auto &l = lesson_ids[n.lesson].obj();
                const auto phase = int( tracker.def( n.lesson ).phase );
                //~ A guided hint was completed; %1$s is the "[Hint]" tag, %2$s the title, %3$d and %4$d the lessons done and in all.
                add_msg( m_good, _( "%1$s Done: %2$s  (%3$d/%4$d)" ),
                         colorize( _( "[Hint]" ), c_light_green ), l.get_title().translated(),
                         tracker.count_done( phase, phase ), tracker.count_total( phase, phase ) );
                break;
            }
        }
    }
    return answered;
}

auto manager::notify( const cata::event &e ) -> void
{
    if( !started || level() == ob_level::off ) {
        return;
    }
    // Only the player's own actions count.
    const auto who = cata_variant( e.get_variant_or_void( "character" ) );
    if( who.type() == cata_variant_type::character_id &&
        who.get<character_id>() != get_avatar().getID() ) {
        return;
    }
    ensure_built();
    tracker.on_input( ob_input_kind::event, io::enum_to_string<event_type>( e.type() ), now() );
    if( e.type() == event_type::avatar_moves ) {
        const auto current = get_avatar().abs_omt_pos();
        if( last_omt && *last_omt != current ) { tracker.on_input( ob_input_kind::event, "avatar_enters_omt", now() ); }
        last_omt = current;
    }

}

} // namespace

namespace onboarding
{

auto load_lesson( const JsonObject &jo, const std::string &src ) -> void
{
    lesson_factory.load( jo, src );
}

auto finalize_lessons() -> void
{
    lesson_factory.finalize();
    screen_factory.finalize();
    get_manager().invalidate();
}

auto check_lessons() -> void
{
    lesson_factory.check();
    screen_factory.check();
}

auto expand_keybinds( const std::string &text ) -> std::string
{ return expand_keys( text ); }

auto reset_lessons() -> void
{
    get_manager().invalidate();
    lesson_factory.reset();
    screen_factory.reset();
}

auto subscriber() -> event_subscriber & // *NOPAD*
{ return get_manager(); }

auto reset_progress() -> void
{
    get_manager().reset_progress();
}

auto prepare_new_game( bool guided ) -> void
{
    auto &m = get_manager();
    m.reset_progress();
    m.guided = guided;
}

auto start_scenario() -> std::string
{
    static const auto guided_scenario = std::string( "evacuee_guided" );
    auto &m = get_manager();
    if( !m.guided || m.level() == ob_level::off || !option_on( "ONBOARDING_SOFT_START" ) ||
        !string_id<scenario>( guided_scenario ).is_valid() ) {
        return std::string();
    }
    return guided_scenario;
}

auto enabled() -> bool
{ return get_manager().level() != ob_level::off; }

auto before_input() -> void
{
    auto &m = get_manager();
    if( m.level() == ob_level::off ) {
        return;
    }
    m.ensure_built();
    m.refresh_availability();
    if( m.level() == ob_level::hints ) {
        // A save or an option change may retain a popup that was already shown.
        // There is no modal answer in this mode, so let its successors advance.
        for( auto i = size_t( 0 ); i < m.tracker.size(); ++i ) {
            if( m.tracker.available( i ) && m.tracker.state( i ).status == ob_status::active &&
                m.tracker.def( i ).presentation == ob_presentation::popup ) {
                m.log_hint( i, false );
                m.tracker.complete( i, now() );
            }
        }
    }
    m.started = true;
    if( !m.last_omt ) { m.last_omt = get_avatar().abs_omt_pos(); }
    // A popup answered with "Got it" can unlock the next lesson; show it now rather than after
    // the next key press.  Bounded, in case a lesson keeps popping up.
    for( auto pass = int( 0 ); pass < 3; ++pass ) {
        m.tracker.update( now(), [&m]( size_t lesson, const ob_matcher & match ) {
            return m.poll( lesson, match );
        } );
        if( !m.present() ) {
            break;
        }
    }
}

auto on_action( const std::string &action_id ) -> void
{
    auto &m = get_manager();
    if( m.level() == ob_level::off ) {
        return;
    }
    m.ensure_built();
    m.tracker.on_input( ob_input_kind::action, action_id, now() );
}

auto on_event( const std::string &name ) -> void
{
    auto &m = get_manager();
    if( !m.started || m.level() == ob_level::off ) { return; }
    m.ensure_built();
    m.tracker.on_input( ob_input_kind::event, name, now() );
}

auto on_signal( const std::string &name ) -> void
{
    auto &m = get_manager();
    if( m.level() == ob_level::off ) {
        return;
    }
    m.ensure_built();
    m.tracker.on_input( ob_input_kind::signal, name, now() );
}

auto sidebar_text( int width ) -> std::string
{
    auto &m = get_manager();
    if( m.level() == ob_level::off ) {
        return std::string();
    }
    m.ensure_built();
    const auto cur = size_t( m.tracker.current() );
    if( cur >= m.lesson_ids.size() || !m.lesson_ids[cur].is_valid() ) {
        return std::string();
    }
    const auto &l = m.lesson_ids[cur].obj();
    const auto phase = int( l.phase );
    //~ Sidebar line for guided hints; %1$s is "Next:", %2$s the lesson title, %3$d and %4$d lessons done and in all.
    auto ret = std::string( string_format( _( "%1$s %2$s  (%3$d/%4$d)" ),
                                           colorize( _( "Next:" ), c_light_green ), l.get_title().translated(),
                                           m.tracker.count_done( phase, phase ),
                                           m.tracker.count_total( phase, phase ) ) );
    const auto lines = std::vector<std::string>( foldstring( expand_keys( l.get_short().translated() ),
                       std::max( width, 10 ) ) );
    for( auto i = size_t( 0 ); i < lines.size() && i < 2; ++i ) {
        ret += "\n" + lines[i];
    }
    return ret;
}

auto on_death() -> void
{
    auto &m = get_manager();
    if( m.level() == ob_level::off ) {
        return;
    }
    m.ensure_built();
    m.refresh_availability();
    m.tracker.on_input( ob_input_kind::signal, "avatar_died", now() );
    m.present();
}

auto handle_action( const std::string &action ) -> bool
{
    if( action == "onboarding_checklist" ) {
        show_checklist();
        return true;
    } else if( action == "EXPLAIN_SCREEN" ) {
        explain_screen( "DEFAULTMODE" );
        return true;
    }
    return false;
}

auto load_screen( const JsonObject &jo, const std::string &src ) -> void
{
    screen_factory.load( jo, src );
}

auto explain_screen( const std::string &category ) -> bool
{
    auto &m = get_manager();
    // The popup below waits for input itself; do not explain the popup.
    static auto explaining = bool( false );
    if( explaining || m.level() == ob_level::off ) {
        return false;
    }
    explaining = true;
    auto reset_explaining = on_out_of_scope( []() { explaining = false; } );
    const auto sid = onboarding_screen_id( category );
    auto body = std::string{};
    if( sid.is_valid() ) {
        body = colorize( pick_title( sid->title, sid->variants ).translated(), c_light_green ) +
               "\n\n" + expand_keys( pick_text( sid->text, sid->variants ) );
    } else {
        body = colorize( _( "This screen" ), c_light_green ) + "\n\n" +
               _( "There is no explanation for this screen yet." );
    }
    //~ Added under every "explain this screen" text.
    body += "\n\n" + expand_keys( string_format( _( "Press %s on any screen to see all of its keys." ),
                                  "<keybind:HELP_KEYBINDINGS>" ) );
    popup( body, PF_NONE );
    m.tracker.on_input( ob_input_kind::action, "EXPLAIN_SCREEN", now() );
    return true;
}

auto show_checklist() -> void
{
    auto &m = get_manager();
    if( m.level() == ob_level::off ) {
        popup( _( "Guided hints are off.  You can turn them on in Settings > Interface > Guided "
                  "hints." ) );
        return;
    }
    m.ensure_built();
    m.refresh_availability();
    m.tracker.on_input( ob_input_kind::action, "onboarding_checklist", now() );

    // Lesson phases as the checklist shows them.
    const auto phase_name = []( int phase ) -> std::string {
        switch( phase ) {
            case 0:
            case 1:
                return _( "First steps" );
            case 2:
                return _( "The first hour" );
            case 3:
                return _( "The first days" );
            default:
                return _( "Later" );
        }
    };

    auto selected = int( 0 );
    while( true ) {
        auto order = std::vector<size_t> {};
        for( auto i = size_t( 0 ); i < m.tracker.size(); ++i ) {
            if( m.tracker.available( i ) && i < m.lesson_ids.size() && m.lesson_ids[i].is_valid() ) {
                order.push_back( i );
            }
        }
        std::ranges::stable_sort( order, [&m]( size_t a, size_t b ) {
            const auto &da = m.tracker.def( a );
            const auto &db = m.tracker.def( b );
            return da.phase != db.phase ? da.phase < db.phase : da.order < db.order;
        } );

        auto menu = uilist{};
        menu.title = _( "Guided hints" );
        menu.text = string_format( _( "Done: %1$d of %2$d.  Pick a hint to read it again." ),
                                   m.tracker.count_done( 0, 3 ), m.tracker.count_total( 0, 3 ) );
        menu.desc_enabled = true;
        menu.allow_disabled = false;
        auto last_phase = std::string{};
        for( const auto i : order ) {
            const auto &def = m.tracker.def( i );
            if( def.phase >= 9 ) {
                // Shown at death only.
                continue;
            }
            const auto group = std::string( phase_name( def.phase ) );
            if( group != last_phase ) {
                menu.addentry( -1, false, 0, colorize( group, c_light_blue ) );
                last_phase = group;
            }
            const auto &st = m.tracker.state( i );
            auto mark = std::string{};
            switch( st.status ) {
                case ob_status::done:
                    mark = colorize( "[x]", c_light_green );
                    break;
                case ob_status::active:
                    mark = colorize( "[>]", c_yellow );
                    break;
                case ob_status::queued:
                case ob_status::waiting:
                    mark = "[ ]";
                    break;
                case ob_status::locked:
                    mark = colorize( "[ ]", c_dark_gray );
                    break;
            }
            const auto &l = m.lesson_ids[i].obj();
            menu.addentry_desc( static_cast<int>( i ), true, 0,
                                mark + " " + l.get_title().translated(),
                                expand_keys( l.get_short().translated() ) );
        }
        menu.selected = selected;
        menu.query();
        if( menu.ret < 0 || menu.ret >= static_cast<int>( m.tracker.size() ) ) {
            break;
        }
        selected = menu.selected;
        const auto lesson = size_t( static_cast<size_t>( menu.ret ) );
        const auto &l = m.lesson_ids[lesson].obj();

        auto detail = uilist{};
        detail.title = l.get_title().translated();
        detail.text = wrap_text( expand_keys( l.get_text() ), 64 );
        detail.addentry( 0, true, 'b', _( "Back" ) );
        detail.addentry( 1, true, 's', _( "Show this hint again" ) );
        detail.addentry( 2, m.tracker.state( lesson ).status != ob_status::done, 'd',
                         _( "Mark as done" ) );
        detail.query();
        if( detail.ret == 1 ) {
            m.tracker.reopen( lesson, now() );
            // Shown as a log hint even for popup lessons: the player just read it.
            for( const auto &n : m.tracker.take_notices() ) {
                if( n.type == ob_notice_type::shown && n.lesson < m.lesson_ids.size() ) {
                    m.log_hint( n.lesson, false );
                }
            }
        } else if( detail.ret == 2 ) {
            m.tracker.complete( lesson, now() );
        }
    }
}

auto serialize( JsonOut &json ) -> void
{
    auto &m = get_manager();
    const auto states = std::map<std::string, ob_lesson_state>( m.built ? m.tracker.export_states() :
                        m.pending_states );
    json.start_object();
    json.member( "guided", m.guided );
    json.member( "lessons" );
    json.start_object();
    for( const auto &entry : states ) {
        const auto &st = entry.second;
        // Untouched lessons need no entry.
        if( st.status == ob_status::locked && st.reminders == 0 &&
        std::ranges::all_of( st.progress, []( auto p ) { return p == 0; } ) ) {
            continue;
        }
        json.member( entry.first );
        json.start_object();
        json.member( "status", status_to_string( st.status ) );
        json.member( "progress", st.progress );
        json.member( "unlocked", st.unlocked_turn );
        json.member( "activated", st.activated_turn );
        json.member( "snoozed_until", st.snoozed_until );
        json.member( "reminders", st.reminders );
        json.member( "quiet", st.quiet );
        json.end_object();
    }
    json.end_object();
    json.end_object();
}

auto deserialize( const JsonObject &jo ) -> void
{
    auto &m = get_manager();
    m.reset_progress();

    jo.read( "guided", m.guided );
    if( jo.has_object( "lessons" ) ) {
        for( const auto member : jo.get_object( "lessons" ) ) {
            auto lo = JsonObject( member.get_object() );

            auto st = ob_lesson_state{};
            st.status = status_from_string( lo.get_string( "status", "locked" ) );
            lo.read( "progress", st.progress );
            lo.read( "unlocked", st.unlocked_turn );
            lo.read( "activated", st.activated_turn );
            lo.read( "snoozed_until", st.snoozed_until );
            lo.read( "reminders", st.reminders );
            lo.read( "quiet", st.quiet );
            m.pending_states[member.name()] = st;
        }
    }
}

} // namespace onboarding
