// ----------------------------------------------------------------------------------------------
// Copyright 2015 Mårten Rånge
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------------------------------------------------------------------------

#include "stdafx.h"

#include "../cpp_json/cpp_json__parser.hpp"
// #include "../cpp_json/cpp_json__parser__sse2.hpp"

namespace
{
  std::wstring empty;

  struct nop_json_context
  {
    using string_type = std::wstring    ; // Type of string, typically std::wstring
    using char_type   = wchar_t         ; // Type of char, typically wchar_t
    using iter_type   = wchar_t const * ; // Type of string "iterator", typically wchar_t const *

    std::size_t count;

    nop_json_context ()
      : count (0)
    {
    }

    // expected* methods are invoked when parser expected a token but didn't find it
    //  Note: certain elements are considered optional so a call to an expected* method might not stop parsing
    inline void expected_char    (std::size_t /*pos*/, char_type /*ch*/) noexcept
    {
      ++count;
    }
    inline void expected_chars   (std::size_t /*pos*/, string_type const & /*chs*/) noexcept
    {
      ++count;
    }
    inline void expected_token   (std::size_t /*pos*/, string_type const & /*token*/) noexcept
    {
      ++count;
    }

    // unexpected* methods are invoked when parser encountered an unexpected token
    //  Note: certain elements are considered optional so a call to an unexpected* method might not stop parsing
    inline void unexpected_token (std::size_t /*pos*/, string_type const & /*token*/)
    {
      ++count;
    }

    // Methods used to build string values

    // Clears cached string
    inline void clear_string ()
    {
      ++count;
    }
    // Appends an 8-bit or 16-bit char to string (depending on char_type)
    inline void push_char (char_type /*ch*/)
    {
      ++count;
    }
    // Appends an 16-bit char to string (allows encoding when char_type is char)
    inline void push_wchar_t (wchar_t /*ch*/)
    {
      ++count;
    }
    // Gets cached string
    inline string_type const & get_string ()
    {
      ++count;
      return empty;
    }

    // The following methods are invoked when JSON values are discovered

    inline bool array_begin ()
    {
      ++count;
      return true;
    }
    inline bool array_end ()
    {
      ++count;
      return true;
    }

    inline bool object_begin ()
    {
      ++count;
      return true;
    }
    inline bool member_key (string_type const & /*s*/)
    {
      ++count;
      return true;
    }
    inline bool object_end ()
    {
      ++count;
      return true;
    }

    inline bool bool_value (bool /*b*/)
    {
      ++count;
      return true;
    }

    inline bool null_value ()
    {
      ++count;
      return true;
    }

    inline bool string_value (string_type const & /*s*/)
    {
      ++count;
      return true;
    }

    inline bool number_value (double /*d*/)
    {
      ++count;
      return true;
    }
  };
}

void perf__parse_json_callback (std::wstring const & json_document)
{
  auto json_begin     = json_document.data ();
  auto json_end       = json_begin + json_document.size ();

  cpp_json::parser::json_parser<nop_json_context> jp (json_begin, json_end);

  auto presult  = jp.try_parse__json ();

  CPP_JSON__ASSERT (presult);

  CPP_JSON__ASSERT (jp.count > 0);
}
