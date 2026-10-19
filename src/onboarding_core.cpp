#include "onboarding_core.h"

#include <algorithm>
#include <utility>

onboarding_tracker::onboarding_tracker( std::vector<ob_lesson_def> defs )
{
    set_lessons( std::move( defs ) );
}

auto onboarding_tracker::set_lessons( std::vector<ob_lesson_def> defs ) -> void
{
    defs_ = std::move( defs );
    reset();
}

auto onboarding_tracker::reset() -> void
{
    states_.assign( defs_.size(), ob_lesson_state() );
    available_.assign( defs_.size(), true );
    for( auto i = size_t( 0 ); i < defs_.size(); ++i ) {
        states_[i].progress.assign( defs_[i].done_when.size(), 0 );
    }
    notices_.clear();
}

auto onboarding_tracker::find( const std::string &id ) const -> size_t
{
    for( auto i = size_t( 0 ); i < defs_.size(); ++i ) {
        if( defs_[i].id == id ) {
            return i;
        }
    }
    return defs_.size();
}

auto onboarding_tracker::sorted() const -> std::vector<size_t>
{
    auto order = std::vector<size_t>( defs_.size() );
    for( auto i = size_t( 0 ); i < order.size(); ++i ) {
        order[i] = i;
    }
    std::ranges::stable_sort( order, [this]( size_t a, size_t b ) {
        if( defs_[a].phase != defs_[b].phase ) {
            return defs_[a].phase < defs_[b].phase;
        }
        return defs_[a].order < defs_[b].order;
    } );
    return order;
}

auto onboarding_tracker::prerequisites_done( size_t i ) const -> bool
{
    for( const auto &pre : defs_[i].prerequisites ) {
        const auto p = size_t( find( pre ) );
        // An unknown prerequisite (a lesson removed by a mod) does not block anything.
        if( p < defs_.size() && available_[p] && states_[p].status != ob_status::done ) {
            return false;
        }
    }
    return true;
}

auto onboarding_tracker::active_count() const -> int
{
    auto n = static_cast<int>( 0 );
    // Popups do not take a place: they are dealt with on the spot.
    for( auto i = size_t( 0 ); i < states_.size(); ++i ) {
        if( available_[i] && states_[i].status == ob_status::active &&
            defs_[i].presentation != ob_presentation::popup ) {
            ++n;
        }
    }
    return n;
}

auto onboarding_tracker::activate( size_t i, int64_t turn ) -> void
{
    auto &st = states_[i];
    // Popups are about what is happening right now, so they never wait in the queue.
    if( defs_[i].presentation != ob_presentation::popup && active_count() >= max_active ) {
        st.status = ob_status::queued;
        return;
    }
    st.status = ob_status::active;
    st.activated_turn = turn;
    st.snoozed_until = turn + defs_[i].snooze_turns;
    st.reminders = 0;
    if( defs_[i].presentation != ob_presentation::silent ) {
        notices_.push_back( { .type = ob_notice_type::shown, .lesson = i } );
    }
}

auto onboarding_tracker::finish( size_t i, int64_t turn ) -> void
{
    auto &st = states_[i];
    if( st.status == ob_status::done ) {
        return;
    }
    const auto was_shown = static_cast<bool>( st.status == ob_status::active );
    st.status = ob_status::done;
    st.quiet = !was_shown;
    if( was_shown && defs_[i].presentation != ob_presentation::silent ) {
        notices_.push_back( { .type = ob_notice_type::done, .lesson = i } );
    }
    promote_queued( turn );
}

auto onboarding_tracker::promote_queued( int64_t turn ) -> void
{
    for( const auto i : sorted() ) {
        if( active_count() >= max_active ) {
            return;
        }
        if( available_[i] && states_[i].status == ob_status::queued ) {
            activate( i, turn );
        }
    }
}

auto onboarding_tracker::on_input( ob_input_kind kind, const std::string &name,
                                   int64_t turn ) -> void
{
    for( const auto i : sorted() ) {
        const auto &def = defs_[i];
        auto &st = states_[i];
        if( !available_[i] || st.status == ob_status::done ) {
            continue;
        }
        // Progress counts in every state, so a player who already knows something is not
        // taught it later.
        auto completed = static_cast<bool>( false );
        for( auto m = size_t( 0 ); m < def.done_when.size(); ++m ) {
            const auto &match = def.done_when[m];
            if( match.kind != kind || match.name != name ) {
                continue;
            }
            st.progress[m] = std::min( st.progress[m] + 1, match.count );
            if( st.progress[m] >= match.count ) {
                completed = true;
            }
        }
        if( completed ) {
            finish( i, turn );
            continue;
        }
        if( st.status == ob_status::waiting ) {
            for( const auto &match : def.trigger ) {
                if( match.kind == kind && match.name == name ) {
                    activate( i, turn );
                    break;
                }
            }
        }
    }
}

auto onboarding_tracker::update( int64_t turn, const poll_fn &poll ) -> void
{
    // Unlock lessons whose prerequisites are done.  Repeat because a lesson unlocked here can
    // already be done (quietly) and unlock others in turn.
    auto changed = static_cast<bool>( true );
    while( changed ) {
        changed = false;
        for( auto i = size_t( 0 ); i < defs_.size(); ++i ) {
            if( states_[i].status == ob_status::locked && prerequisites_done( i ) ) {
                states_[i].status = ob_status::waiting;
                states_[i].unlocked_turn = turn;
                changed = true;
            }
        }
    }

    for( const auto i : sorted() ) {
        const auto &def = defs_[i];
        auto &st = states_[i];
        if( !available_[i] || st.status == ob_status::done || st.status == ob_status::locked ) {
            continue;
        }

        // Polled done conditions.
        auto completed = static_cast<bool>( false );
        for( auto m = size_t( 0 ); m < def.done_when.size() && !completed; ++m ) {
            const auto &match = def.done_when[m];
            switch( match.kind ) {
                case ob_input_kind::state:
                case ob_input_kind::condition:
                    completed = ( !match.while_shown || st.status == ob_status::active ) &&
                                poll && poll( i, match );
                    break;
                case ob_input_kind::after_turns:
                    completed = st.status == ob_status::active &&
                                turn - st.activated_turn >= match.count;
                    break;
                default:
                    break;
            }
        }
        if( completed ) {
            finish( i, turn );
            continue;
        }

        if( st.status == ob_status::waiting ) {
            auto triggered = static_cast<bool>( def.trigger.empty() );
            for( const auto &match : def.trigger ) {
                switch( match.kind ) {
                    case ob_input_kind::start:
                        triggered = true;
                        break;
                    case ob_input_kind::after_turns:
                        triggered = turn - st.unlocked_turn >= match.count;
                        break;
                    case ob_input_kind::state:
                    case ob_input_kind::condition:
                        triggered = poll && poll( i, match );
                        break;
                    default:
                        break;
                }
                if( triggered ) {
                    break;
                }
            }
            if( triggered ) {
                activate( i, turn );
            }
        } else if( st.status == ob_status::active && def.snooze_turns > 0 &&
                   st.reminders < max_reminders && turn >= st.snoozed_until ) {
            st.reminders++;
            st.snoozed_until = turn + def.snooze_turns;
            if( def.presentation != ob_presentation::silent ) {
                notices_.push_back( { .type = ob_notice_type::reminder, .lesson = i } );
            }
        }
    }
    promote_queued( turn );
}

auto onboarding_tracker::complete( size_t i, int64_t turn ) -> void
{
    if( i >= defs_.size() || states_[i].status == ob_status::done ) {
        return;
    }
    states_[i].status = ob_status::done;
    states_[i].quiet = false;
    promote_queued( turn );
}

auto onboarding_tracker::snooze( size_t i, int64_t turn ) -> void
{
    if( i >= defs_.size() ) {
        return;
    }
    states_[i].snoozed_until = turn + defs_[i].snooze_turns;
}

auto onboarding_tracker::reopen( size_t i, int64_t turn ) -> void
{
    if( i >= defs_.size() ) {
        return;
    }
    auto &st = states_[i];
    st.status = ob_status::active;
    st.progress.assign( defs_[i].done_when.size(), 0 );
    st.activated_turn = turn;
    st.snoozed_until = turn + defs_[i].snooze_turns;
    st.reminders = 0;
    st.quiet = false;
    notices_.push_back( { .type = ob_notice_type::shown, .lesson = i } );
}

auto onboarding_tracker::set_available( size_t i, bool available ) -> void
{
    if( i < available_.size() ) {
        available_[i] = available;
    }
}

auto onboarding_tracker::take_notices() -> std::vector<ob_notice>
{
    auto ret = std::vector<ob_notice> {};
    ret.swap( notices_ );
    return ret;
}

auto onboarding_tracker::current() const -> size_t
{
    for( const auto i : sorted() ) {
        if( available_[i] && states_[i].status == ob_status::active &&
            defs_[i].presentation != ob_presentation::silent ) {
            return i;
        }
    }
    return defs_.size();
}

auto onboarding_tracker::count_done( int min_phase, int max_phase ) const -> int
{
    auto n = static_cast<int>( 0 );
    for( auto i = size_t( 0 ); i < defs_.size(); ++i ) {
        if( available_[i] && defs_[i].phase >= min_phase && defs_[i].phase <= max_phase &&
            states_[i].status == ob_status::done ) {
            ++n;
        }
    }
    return n;
}

auto onboarding_tracker::count_total( int min_phase, int max_phase ) const -> int
{
    auto n = static_cast<int>( 0 );
    for( auto i = size_t( 0 ); i < defs_.size(); ++i ) {
        if( available_[i] && defs_[i].phase >= min_phase && defs_[i].phase <= max_phase ) {
            ++n;
        }
    }
    return n;
}

auto onboarding_tracker::export_states() const -> std::map<std::string, ob_lesson_state>
{
    auto ret = std::map<std::string, ob_lesson_state> {};
    for( auto i = size_t( 0 ); i < defs_.size(); ++i ) {
        ret[defs_[i].id] = states_[i];
    }
    return ret;
}

auto onboarding_tracker::import_states( const std::map<std::string, ob_lesson_state> &states ) ->
void
{
    reset();
    for( auto i = size_t( 0 ); i < defs_.size(); ++i ) {
        const auto it = states.find( defs_[i].id );
        if( it == states.end() ) {
            continue;
        }
        states_[i] = it->second;
        // The lesson's done_when list may have changed since the save was made.
        states_[i].progress.resize( defs_[i].done_when.size(), 0 );
    }
}
