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

#include <cassert>
#include <cmath>

#define CPP_JSON__ASSERT    assert
#define CPP_JSON__NO_COPY_MOVE(name)          \
  name              (name const &)  = delete; \
  name              (name &&     )  = delete; \
  name & operator=  (name const &)  = delete; \
  name & operator=  (name &&     )  = delete;
#define CPP_JSON__PICK(s)    json_string_literal<char_type>::pick (s, L##s)

namespace cpp_json { namespace parser
{
  namespace details
  {
    template<typename char_type>
    struct json_string_literal;

    template<>
    struct json_string_literal<char>
    {
      constexpr static auto pick (char const * s, wchar_t const * /*ws*/) noexcept
      {
        return s;
      }
    };

    template<>
    struct json_string_literal<wchar_t>
    {
      constexpr static auto pick (char const * /*s*/, wchar_t const * ws) noexcept
      {
        return ws;
      }
    };

    struct json_pow10table
    {
      constexpr static int  min_power_10  = -322;
      constexpr static int  max_power_10  = 307;
      constexpr static int  table_size    = max_power_10 - min_power_10 + 1;

      inline json_pow10table () noexcept
      {
        for (auto p = 0; p < table_size; ++p)
        {
          pow10table[p] = std::pow (10.0, static_cast<double> (p + min_power_10));
        }
      }

      inline double pow10 (int i) noexcept
      {
        if (i < min_power_10)
        {
          return 0.0;
        }
        else if (i > max_power_10)
        {
          return INFINITY;
        }
        else
        {
          return pow10table[i - min_power_10];
        }
      }

    private:
      double                pow10table[table_size];
    };

    template<typename string_type>
    struct json_tokens
    {
      using char_type = typename string_type::value_type;

      json_tokens ()
        : token__eos                  (CPP_JSON__PICK ("EOS"        ))
        , token__null                 (CPP_JSON__PICK ("null"       ))
        , token__false                (CPP_JSON__PICK ("false"      ))
        , token__true                 (CPP_JSON__PICK ("true"       ))
        , token__digit                (CPP_JSON__PICK ("digit"      ))
        , token__hex_digit            (CPP_JSON__PICK ("hexdigit"   ))
        , token__char                 (CPP_JSON__PICK ("char"       ))
        , token__escapes              (CPP_JSON__PICK ("\"\\/bfnrtu"))
        , token__new_line             (CPP_JSON__PICK ("NEWLINE"    ))
        , token__root_value_preludes  (CPP_JSON__PICK ("{["         ))
        , token__value_preludes       (CPP_JSON__PICK ("\"{[-"      ))
      {
      }

      string_type const token__eos                ;
      string_type const token__null               ;
      string_type const token__false              ;
      string_type const token__true               ;
      string_type const token__digit              ;
      string_type const token__hex_digit          ;
      string_type const token__char               ;
      string_type const token__escapes            ;
      string_type const token__new_line           ;
      string_type const token__root_value_preludes;
      string_type const token__value_preludes     ;
    };
  }
  // TContext must fulfill the following contract
  //  struct some_json_context
  //  {
  //    using string_type = ... ; // Type of string, typically std::wstring
  //    using char_type   = ... ; // Type of char, typically wchar_t
  //    using iter_type   = ... ; // Type of string "iterator", typically wchar_t const *
  //
  //    // expected* methods are invoked when parser expected a token but didn't find it
  //    //  Note: certain elements are considered optional so a call to an expected* method might not stop parsing
  //    void expected_char    (std::size_t pos, char_type ch) noexcept
  //    void expected_chars   (std::size_t pos, string_type const & chs) noexcept
  //    void expected_token   (std::size_t pos, string_type const & token) noexcept
  //
  //    // unexpected* methods are invoked when parser encountered an unexpected token
  //    //  Note: certain elements are considered optional so a call to an unexpected* method might not stop parsing
  //    void unexpected_token (std::size_t pos, string_type const & token);
  //
  //    // Methods used to build string values
  //
  //    // Clears cached string
  //    void clear_string ();
  //    // Appends an 8-bit or 16-bit char to string (depending on char_type)
  //    void push_char (char_type ch);
  //    // Appends an 16-bit char to string (allows encoding when char_type is char)
  //    void push_wchar_t (wchar_t ch);
  //    // Gets cached string
  //    string_type const & get_string ();
  //
  //    // The following methods are invoked when JSON values are discovered
  //
  //    bool array_begin ();
  //    bool array_end ();
  //
  //    bool object_begin ();
  //    bool member_key (string_type const & s);
  //    bool object_end ();
  //
  //    bool bool_value (bool b);
  //
  //    bool null_value ();
  //
  //    bool string_value (string_type const & s);
  //
  //    bool number_value (double d);
  //
  //  };
  template<typename TContext>
  struct json_parser : TContext
  {
    using context_type    = TContext                            ;
    using char_type       = typename context_type::char_type    ;
    using string_type     = typename context_type::string_type  ;
    using iter_type       = typename context_type::iter_type    ;

    constexpr json_parser (iter_type begin, iter_type end) noexcept
      : begin   (begin)
      , end     (end)
      , current (begin)
    {
    }

    // Gets current parser position
    constexpr std::size_t pos () const noexcept
    {
      CPP_JSON__ASSERT (current >= begin);
      return static_cast<std::size_t> (current - begin);
    }

    // Tries to parse input as JSON
    bool try_parse__json ()
    {
      return
            consume__white_space ()
        &&  try_parse__root_value ()
        &&  try_parse__eos ()
        ;
    }

  private:
    static details::json_tokens<string_type> tokens             ;
    static details::json_pow10table          pow10table         ;

    iter_type const begin                                       ;
    iter_type const end                                         ;
    iter_type       current                                     ;

    constexpr bool eos () const noexcept
    {
      return current >= end;
    }

    constexpr bool neos () const noexcept
    {
      return current < end;
    }

    constexpr char_type ch () const noexcept
    {
      return *current;
    }

    inline void adv () noexcept
    {
      ++current;
    }

    bool raise__eos ()
    {
      context_type::unexpected_token (pos (), tokens.token__eos);
      return false;
    }

    bool raise__eeos ()
    {
      context_type::expected_token (pos (), tokens.token__eos);
      return false;
    }

    bool raise__value ()
    {
      auto p = pos ();
      context_type::expected_token  (p, tokens.token__null);
      context_type::expected_token  (p, tokens.token__true);
      context_type::expected_token  (p, tokens.token__false);
      context_type::expected_token  (p, tokens.token__digit);
      context_type::expected_chars  (p, tokens.token__value_preludes);
      return false;
    }

    bool raise__root_value ()
    {
      context_type::expected_chars  (pos (), tokens.token__root_value_preludes);
      return false;
    }

    bool raise__char ()
    {
      context_type::expected_token  (pos (), tokens.token__char);
      return false;
    }

    bool raise__digit ()
    {
      context_type::expected_token  (pos (), tokens.token__digit);
      return false;
    }

    bool raise__hex_digit ()
    {
      context_type::expected_token  (pos (), tokens.token__hex_digit);
      return false;
    }

    bool raise__escapes ()
    {
      context_type::expected_chars  (pos (), tokens.token__escapes);
      return false;
    }

    static constexpr bool is_white_space (char_type ch) noexcept
    {
      return ch == '\t' || ch == '\n' || ch == '\r' || ch == ' ';
    }

    static constexpr bool is_digit (char_type ch) noexcept
    {
      return ch >= '0' && ch <= '9';
    }

    inline bool consume__white_space () noexcept
    {
      while (neos () && is_white_space (ch ()))
      {
        adv ();
      }
      return true;
    }

    constexpr bool test__char (char c) const noexcept
    {
      return neos () && ch () == c;
    }

    inline bool try_consume__char (char c)
    {
      if (eos ())
      {
        context_type::expected_char (pos (), c);
        return raise__eos ();
      }
      else if (ch () == c)
      {
        adv ();
        return true;
      }
      else
      {
        context_type::expected_char (pos (), c);
        return false;
      }
    }

    inline bool try_parse__any_of_2 (char_type c0, char_type c1, char_type & r)
    {
      if (eos ())
      {
        context_type::expected_char (pos (), c0);
        context_type::expected_char (pos (), c1);
        return raise__eos ();
      }
      else if (ch () == c0 || ch () == c1)
      {
        r = ch ();
        adv ();
        return true;
      }
      else
      {
        context_type::expected_char (pos (), c0);
        context_type::expected_char (pos (), c1);
        return false;
      }
    }

    inline bool try_consume__token (string_type const & tk) noexcept
    {
      auto tsz = tk.size ();

      if (current + tsz <= end)
      {
        auto scurrent = current;
        auto tpos     = 0U;
        for (; tpos < tsz && tk[tpos] == ch (); ++tpos)
        {
          adv ();
        }

        if (tsz == tpos)
        {
          return true;
        }
        else
        {
          // In order to support error report, restore current
          current = scurrent;
          return false;
        }
      }
      else
      {
        return false;
      }
    }

    bool try_parse__null ()
    {
      if (try_consume__token (tokens.token__null))
      {
        return context_type::null_value ();
      }
      else
      {
        return raise__value ();
      }
    }

    bool try_parse__true ()
    {
      if (try_consume__token (tokens.token__true))
      {
        return context_type::bool_value (true);
      }
      else
      {
        return raise__value ();
      }
    }

    bool try_parse__false ()
    {
      if (try_consume__token (tokens.token__false))
      {
        return context_type::bool_value (false);
      }
      else
      {
        return raise__value ();
      }
    }

    bool try_parse__uint (double & r)
    {
      auto first = true;
      for (;;)
      {
        if (eos ())
        {
          return raise__digit () || raise__eos () || !first;
        }
        else
        {
          auto c = ch ();
          if (is_digit (c))
          {
            adv ();
            r = 10.0*r + (c - '0');
          }
          else
          {
            return raise__digit () || !first;
          }
        }
        first = false;
      }
    }

    bool try_parse__uint0 (double & r)
    {
      // try_parse__uint0 only consumes 0 if input is 0123, this in order to be conformant with spec
      if (try_consume__char ('0'))
      {
        r = 0.0;
        return true;
      }
      else
      {
        return try_parse__uint (r);
      }
    }

    constexpr static double pow10 (int i) noexcept
    {
      return pow10table.pow10 (i);
    }

    bool try_parse__fraction (double & r)
    {
      if (try_consume__char ('.'))
      {
        auto scurrent = current;
        auto uf = 0.0;
        if (try_parse__uint (uf))
        {
          r = uf * pow10 (static_cast<int> (scurrent - current));
          return true;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return true;  // Fraction is optional
      }
    }

    bool try_parse__exponent (int & r)
    {
      char_type exp = ' ';
      if (try_parse__any_of_2 ('e', 'E', exp))
      {
        char_type sign = '+';
        try_parse__any_of_2 ('+', '-', sign); // Don't check result as sign is optional

        // TODO: Seems overkill parsing exponent as double
        auto ue = 0.0;
        if (try_parse__uint (ue))
        {
          auto ur = static_cast<int> (ue);
          r = (sign == '-') ? -ur : ur;
          return true;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return true;  // Exponent is optional
      }
    }

    bool try_parse__number ()
    {
      auto hasMinus = try_consume__char ('-');

      auto i = 0.0;
      auto f = 0.0;
      auto e = 0;

      if (
            try_parse__uint0    (i)
        &&  try_parse__fraction (f)
        &&  try_parse__exponent (e)
        )
      {
        auto uu = i + f;
        auto ff = hasMinus ? -uu : uu;
        auto ee = pow10 (e);
        auto rr = ff*ee;
        return context_type::number_value (rr);
      }
      else
      {
        return false;
      }
    }

    bool try_parse__chars ()
    {
      context_type::clear_string ();

      for (;;)
      {
        if (eos ())
        {
          return raise__char () || raise__eos ();
        }

        auto c = ch ();
        switch (c)
        {
        case '"':
          return true;
        case '\n':
        case '\r':
          context_type::unexpected_token (pos (), tokens.token__new_line);
          return false;
        case '\\':
          {
            adv ();
            auto e = ch ();
            switch (e)
            {
            case '"':
            case '\\':
            case '/':
              context_type::push_char (e);
              break;
            case 'b':
              context_type::push_char ('\b');
              break;
            case 'f':
              context_type::push_char ('\f');
              break;
            case 'n':
              context_type::push_char ('\n');
              break;
            case 'r':
              context_type::push_char ('\r');
              break;
            case 't':
              context_type::push_char ('\t');
              break;
            case 'u':
              {
                wchar_t wresult = 0;

                for (auto iter = 0U; iter < 4U; ++iter)
                {
                  if (eos ())
                  {
                    return raise__hex_digit () || raise__eos ();
                  }

                  wresult = wresult << 4;
                  adv ();
                  auto hd = ch ();
                  switch (hd)
                  {
                  case '0':
                  case '1':
                  case '2':
                  case '3':
                  case '4':
                  case '5':
                  case '6':
                  case '7':
                  case '8':
                  case '9':
                    wresult += hd - '0';
                    break;
                  case 'A':
                  case 'B':
                  case 'C':
                  case 'D':
                  case 'E':
                  case 'F':
                    wresult += hd - 'A' + 10;
                    break;
                  case 'a':
                  case 'b':
                  case 'c':
                  case 'd':
                  case 'e':
                  case 'f':
                    wresult += hd - 'a' + 10;
                    break;
                  default:
                    return raise__hex_digit ();
                  }
                }

                context_type::push_wchar_t (wresult);
              }
              break;
            default:
              return raise__escapes ();
            }
          }
          break;
        default:
          context_type::push_char (c);
          break;
        }
        adv ();
      }
    }

    inline bool try_parse__string_impl ()
    {
      return
            try_consume__char ('"')
        &&  try_parse__chars  ()
        &&  try_consume__char ('"')
        ;
    }

    bool try_parse__string ()
    {
      return
            try_parse__string_impl ()
        &&  context_type::string_value (context_type::get_string ())
        ;
    }

    bool try_consume__delimiter (bool first)
    {
      return first || (try_consume__char (',') && consume__white_space  ());
    }

    bool try_parse__array_values ()
    {
      auto first = true;
      for (;;)
      {
        if (test__char (']'))
        {
          return true;
        }
        else if (
              try_consume__delimiter  (first)
          &&  try_parse__value        ()
          )
        {
        }
        else
        {
          return false;
        }
        first = false;
      }
    }

    bool try_parse__array ()
    {
      return
            try_consume__char         ('[')
        &&  consume__white_space      ()
        &&  context_type::array_begin ()
        &&  try_parse__array_values   ()
        &&  try_consume__char         (']')
        &&  context_type::array_end   ()
        ;
    }

    bool try_parse__member_key ()
    {
      return
            try_parse__string_impl ()
        &&  context_type::member_key (context_type::get_string ())
        ;
    }

    bool try_parse__object_members ()
    {
      auto first = true;
      for (;;)
      {
        if (test__char ('}'))
        {
          return true;
        }
        else if (
              try_consume__delimiter  (first)
          &&  try_parse__member_key   ()
          &&  consume__white_space    ()
          &&  try_consume__char       (':')
          &&  consume__white_space    ()
          &&  try_parse__value        ()
          )
        {
        }
        else
        {
          return false;
        }
        first = false;
      }
    }

    bool try_parse__object ()
    {
      return
            try_consume__char         ('{')
        &&  consume__white_space      ()
        &&  context_type::object_begin()
        &&  try_parse__object_members ()
        &&  try_consume__char         ('}')
        &&  context_type::object_end  ()
        ;
    }

    bool try_parse__value ()
    {
      if (eos ())
      {
        return raise__value () || raise__eos ();
      }
      else
      {
        auto pv = [this] ()
          {
            switch (ch ())
            {
            case 'n':
              return try_parse__null ();
            case 't':
              return try_parse__true ();
            case 'f':
              return try_parse__false ();
            case '[':
              return try_parse__array ();
            case '{':
              return try_parse__object ();
            case '"':
              return try_parse__string ();
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
              return try_parse__number ();
            default:
              return raise__value ();
            }
          };
        return pv () && consume__white_space ();
      }
    }

    bool try_parse__root_value ()
    {
      if (eos ())
      {
        return raise__root_value () || raise__eos ();
      }
      else
      {
        auto pv = [this] ()
          {
            switch (ch ())
            {
            case '[':
              return try_parse__array ();
            case '{':
              return try_parse__object ();
            default:
              return raise__root_value ();
            }
          };
        return pv () && consume__white_space ();
      }
    }

    bool try_parse__eos ()
    {
      if (eos ())
      {
        return true;
      }
      else
      {
        raise__eeos ();
        return false;
      }
    }
  };

  template<typename TContext>
  details::json_pow10table json_parser<TContext>::pow10table;

  template<typename TContext>
  details::json_tokens<typename TContext::string_type> json_parser<TContext>::tokens;

} }
