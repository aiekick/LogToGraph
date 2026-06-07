/*
Copyright 2022-2026 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once

// Lua-callable regex object wrapping boost::regex (the engine vendored by imguipack via
// USE_IMGUI_COLOR_TEXT_EDIT; both host and plugin already link the boost_regex CMake target).
// Built once at script init, called many times in the parse loop — the typical log-parsing usage
// pattern compiles a handful of patterns at startup and matches each against every log row.
//
// API surface (callable from Lua as `local re = ltg:regex(pattern); re:match(input)` etc.):
//   re:test(input)            -> bool       — any match anywhere in input
//   re:match(input)           -> string|table|nil
//                                            — captures of the FIRST match, returned as multiple
//                                              values via sol::variadic_results; nil if no match;
//                                              a single value (the whole match) when the pattern
//                                              has no capture groups
//   re:find(input)            -> (int,int)|nil — 1-based [start,end] of the first match, Lua-style
//   re:gsub(input, replace)   -> (string,int)   — substituted result + count, mirrors string.gsub
//   re:gmatch(input)          -> Lua-style iterator over matches (returns the capture list each
//                                                  step, nil to terminate)
//
// Errors: an invalid pattern throws boost::regex_error from the constructor; sol2's exception
// handler catches it as a std::exception and surfaces e.what() to the Lua error path — the user
// sees the real boost message instead of "C++ exception".

#include <boost/regex.hpp>

#include <sol/sol.hpp>

#include <string>
#include <tuple>
#include <utility>

class LuaRegex {
public:
    explicit LuaRegex(const std::string& aPattern) : m_regex(aPattern) {}

    bool test(const std::string& aInput) const {
        return boost::regex_search(aInput, m_regex);
    }

    // Variadic-returning helper used by both `match` and the `gmatch` iterator. Emits the captures
    // of `aMatch` into `aResults`. If the pattern has no capture groups, emits the whole match.
    // Uses `aMatch.str(idx)` (rather than `aMatch[idx]`) so the index parameter is unambiguous
    // — boost::match_results has size_t / const char* overloads on operator[] (positional vs
    // named captures), which silently picks one and emits C4267 size_t→int conversion warnings
    // on MSVC otherwise.
    static void m_pushCaptures(sol::variadic_results& aoResults, sol::state_view aLua, const boost::smatch& aMatch) {
        if (aMatch.size() <= 1) {
            aoResults.push_back({aLua, sol::in_place_type<std::string>, aMatch.str(0)});
            return;
        }
        for (int i = 1; i < static_cast<int>(aMatch.size()); ++i) {
            aoResults.push_back({aLua, sol::in_place_type<std::string>, aMatch.str(i)});
        }
    }

    sol::variadic_results match(const std::string& aInput, sol::this_state aThisState) const {
        sol::state_view lua(aThisState);
        sol::variadic_results results;
        boost::smatch m;
        if (boost::regex_search(aInput, m, m_regex)) {
            m_pushCaptures(results, lua, m);
        }
        // no match -> empty results -> Lua sees nil (no return values)
        return results;
    }

    sol::optional<std::tuple<int, int>> find(const std::string& aInput) const {
        boost::smatch m;
        if (boost::regex_search(aInput, m, m_regex)) {
            // boost positions are 0-based offsets into the input; Lua's string.find returns
            // 1-based inclusive [start, end] — translate accordingly.
            //
            // Every numeric accessor on boost::match_results has a `const char*` overload
            // for named-capture lookup (`operator[]`, `position`, `length`, ...) and passing
            // even a static_cast<size_t>(0) is ambiguous on MSVC. Workaround: use prefix() and
            // suffix() instead — they return sub_match references for what surrounds the whole
            // match, no overload set, no index parameter:
            //   prefix.second == first character of the match
            //   suffix.first  == one past the last character of the match
            const auto& prefix = m.prefix();
            const auto& suffix = m.suffix();
            const int start = static_cast<int>(prefix.second - aInput.cbegin()) + 1;
            const int finish = static_cast<int>(suffix.first - aInput.cbegin());
            return std::make_tuple(start, finish);
        }
        return sol::nullopt;
    }

    std::tuple<std::string, int> gsub(const std::string& aInput, const std::string& aReplacement) const {
        // boost replaces all non-overlapping matches by default; count them by iterating in
        // parallel so the user gets the same (result, count) shape as Lua's string.gsub.
        const std::string result = boost::regex_replace(aInput, m_regex, aReplacement);
        int count = 0;
        for (auto it = boost::sregex_iterator(aInput.begin(), aInput.end(), m_regex); it != boost::sregex_iterator(); ++it) {
            ++count;
        }
        return std::make_tuple(result, count);
    }

    // Lua-style iterator: each call returns the capture list of the next match, then nil. Same
    // contract as `string.gmatch`. The iterator state is captured in a sol2 closure that owns
    // the begin/end sregex_iterator pair — no input-string copy needed (sregex_iterator stores
    // iterators into the bound string, so the closure also keeps the string alive).
    sol::function gmatch(const std::string& aInput, sol::this_state aThisState) const {
        sol::state_view lua(aThisState);
        // shared_ptr so the lambda's captures stay alive across iterator advances; the underlying
        // boost::regex (this->m_regex) is referenced via the iterator pair built on `*this`.
        auto state = std::make_shared<std::pair<boost::sregex_iterator, boost::sregex_iterator>>(
            boost::sregex_iterator(aInput.begin(), aInput.end(), m_regex),
            boost::sregex_iterator()
        );
        // the input string itself must outlive the iterators (boost::sregex_iterator stores raw
        // begin/end iterators into it). Capture by value.
        auto holder = std::make_shared<std::string>(aInput);
        // re-build the begin iterator now that holder owns the storage, so the iterators point
        // into the captured copy and not the caller's temporary.
        state->first = boost::sregex_iterator(holder->begin(), holder->end(), m_regex);
        state->second = boost::sregex_iterator();

        return sol::make_object(lua, [state, holder](sol::this_state ts) -> sol::variadic_results {
            sol::state_view L(ts);
            sol::variadic_results out;
            if (state->first == state->second) {
                return out;  // empty -> Lua iterator terminates (nil)
            }
            m_pushCaptures(out, L, *state->first);
            ++state->first;
            return out;
        });
    }

private:
    boost::regex m_regex;
};
