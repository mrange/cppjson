#include "stdafx.h"

#include <cassert>
#include <cmath>

#define CPP_JSON__ASSERT    assert
#define CPP_JSON__NOEXCEPT  throw ()
#define CPP_JSON__NO_COPY_MOVE(name)          \
  name              (name const &)  = delete; \
  name              (name &&     )  = delete; \
  name & operator=  (name const &)  = delete; \
  name & operator=  (name &&     )  = delete;

namespace cpp_json { namespace parser
{
  template<typename string_type>
  struct json_tokens
  {
    json_tokens ()
      : token__eos                  ("EOS"        )
      , token__null                 ("null"       )
      , token__false                ("false"      )
      , token__true                 ("true"       )
      , token__digit                ("digit"      )
      , token__hex_digit            ("hexdigit"   )
      , token__char                 ("char"       )
      , token__escapes              ("\"\\/bfnrtu")
      , token__new_line             ("NEWLINE"    )
      , token__root_value_preludes  ("{["         )
      , token__value_preludes       ("\"{[-"      )
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

  template<typename TContext>
  struct json_parser : TContext
  {
    using context_type    = TContext                            ;
    using char_type       = typename context_type::char_type    ;
    using string_type     = typename context_type::string_type  ;
    using iter_type       = typename context_type::iter_type    ;

    static json_tokens<string_type> tokens;

    iter_type const begin                                       ;
    iter_type const end                                         ;
    iter_type       current                                     ;

    constexpr json_parser (iter_type begin, iter_type end) throw ()
      : begin   (begin)
      , end     (end)
      , current (begin)
    {
    }

    constexpr bool eos () const throw ()
    {
      return current >= end;
    }

    constexpr bool neos () const throw ()
    {
      return current < end;
    }

    constexpr char_type ch () const throw ()
    {
      return *current;
    }

    inline void adv () throw ()
    {
      ++current;
    }

    bool raise__eos ()
    {
      context_type::unexpected_token (current, tokens.token__eos);
      return false;
    }

    bool raise__eeos ()
    {
      context_type::expected_token (current, tokens.token__eos);
      return false;
    }

    bool raise__value ()
    {
      context_type::expected_token  (current, tokens.token__null);
      context_type::expected_token  (current, tokens.token__true);
      context_type::expected_token  (current, tokens.token__false);
      context_type::expected_token  (current, tokens.token__digit);
      context_type::expected_chars  (current, tokens.token__value_preludes);
      return false;
    }

    bool raise__root_value ()
    {
      context_type::expected_chars  (current, tokens.token__root_value_preludes);
      return false;
    }

    bool raise__char ()
    {
      context_type::expected_token  (current, tokens.token__char);
      return false;
    }

    bool raise__digit ()
    {
      context_type::expected_token  (current, tokens.token__digit);
      return false;
    }

    bool raise__hex_digit ()
    {
      context_type::expected_token  (current, tokens.token__hex_digit);
      return false;
    }

    bool raise__escapes ()
    {
      context_type::expected_chars  (current, tokens.token__escapes);
      return false;
    }

    static constexpr bool is_white_space (char ch) throw ()
    {
      return ch == '\t' || ch == '\n' || ch == '\r' || ch == ' ';
    }

    static constexpr bool is_digit (char ch) throw ()
    {
      return ch >= '0' && ch <= '9';
    }

    inline bool consume__white_space () throw ()
    {
      while (neos () && is_white_space (ch ()))
      {
        adv ();
      }
      return true;
    }

    inline bool test__char (char c) throw ()
    {
      return neos () && ch () == c;
    }

    inline bool try_consume__char (char c)
    {
      if (eos ())
      {
        context_type::expected_char (current, c);
        return raise__eos ();
      }
      else if (ch () == c)
      {
        adv ();
        return true;
      }
      else
      {
        context_type::expected_char (current, c);
        return false;
      }
    }

    inline bool try_parse__any_of_2 (char c0, char c1, char & r)
    {
      if (eos ())
      {
        context_type::expected_char (current, c0);
        context_type::expected_char (current, c1);
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
        context_type::expected_char (current, c0);
        context_type::expected_char (current, c1);
        return false;
      }
    }

    inline bool try_consume__token (string_type const & tk) throw ()
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

    static constexpr double pow10 (double d) throw ()
    {
      return std::pow (10.0, d);
    }

    bool try_parse__fraction (double & r)
    {
      if (try_consume__char ('.'))
      {
        auto scurrent = current;
        auto uf = 0.0;
        if (try_parse__uint (uf))
        {
          // TODO: table optimize
          r = uf * pow10 (static_cast<double> (scurrent - current));
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
      auto exp = ' ';
      if (try_parse__any_of_2 ('e', 'E', exp))
      {
        auto sign = '+';
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
          unexpected_token (current, tokens.token__new_line);
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
                wchar_t result = 0;

                for (auto iter = 0U; iter < 4U; ++iter)
                {
                  if (eos ())
                  {
                    return raise__hex_digit () || raise__eos ();
                  }

                  result = result << 4;
                  adv ();
                  auto c = ch ();
                  switch (c)
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
                    result += c - '0';
                    break;
                  case 'A':
                  case 'B':
                  case 'C':
                  case 'D':
                  case 'E':
                  case 'F':
                    result += c - 'A' + 10;
                    break;
                  case 'a':
                  case 'b':
                  case 'c':
                  case 'd':
                  case 'e':
                  case 'f':
                    result += c - 'a' + 10;
                    break;
                  default:
                    return raise__hex_digit ();
                  }
                }

                context_type::push_wchar_t (result);
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
        &&  context_type::member_key (current_string)
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

    bool try_parse__json ()
    {
      return
            consume__white_space ()
        &&  try_parse__root_value ()
        &&  try_parse__eos ()
        ;
    }
  };

  template<typename TContext>
  json_tokens<typename TContext::string_type> json_parser<TContext>::tokens;

} }
