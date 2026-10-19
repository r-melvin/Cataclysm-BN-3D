#pragma once
#include <string>

namespace onboarding
{
/// Expand lesson key tags using complete native key events, including function keys.
auto expand_keybinds( const std::string &text ) -> std::string;
}
