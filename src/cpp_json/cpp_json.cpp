#include "stdafx.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

#define CPP_JSON__NOEXCEPT throw ()

namespace cpp_json
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

    json_parser (iter_type begin, iter_type end) throw ()
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
              // TODO:
              return raise__escapes ();
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

    bool try_parse__string_impl ()
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

}

namespace default_cpp_json
{
  constexpr auto default_size = 16U         ;

  using string_type           = std::string ;

  struct default_json_context
  {
    using string_type = string_type ;
    using char_type   = char        ;
    using iter_type   = char const* ;

    string_type current_string      ;

    inline default_json_context ()
    {
      current_string.reserve (default_size);
    }

    inline void expected_char (iter_type /*current*/, char_type /*ch*/) throw ()
    {
    }

    inline void expected_chars (iter_type /*current*/, string_type const & /*chs*/) throw ()
    {
    }

    inline void expected_token (iter_type /*current*/, string_type const & /*token*/) throw ()
    {
    }

    inline void unexpected_token (iter_type /*current*/, string_type const & /*token*/) throw ()
    {
    }

    inline void clear_string ()
    {
      current_string.clear ();
    }

    inline void push_char (char_type ch)
    {
      current_string.push_back (ch);
    }

    inline string_type const & get_string () throw ()
    {
      return current_string;
    }

    bool array_begin ()
    {
      std::cout << "ARRAY_BEGIN" << std::endl;
      return true;
    }

    bool array_end ()
    {
      std::cout << "ARRAY_END" << std::endl;
      return true;
    }

    bool object_begin ()
    {
      std::cout << "OBJECT_BEGIN" << std::endl;
      return true;
    }

    bool member_key (string_type const & s)
    {
      std::cout << "MEMBER_KEY: " << s << std::endl;
      return true;
    }

    bool object_end ()
    {
      std::cout << "OBJECT_END" << std::endl;
      return true;
    }

    bool bool_value (bool b)
    {
      std::cout << "BOOL_VALUE: " << (b ? "true" : "false") << std::endl;
      return true;
    }

    bool null_value ()
    {
      std::cout << "NULL_VALUE" << std::endl;
      return true;
    }

    bool string_value (string_type const & s)
    {
      std::cout << "STRING_VALUE: " << s << std::endl;
      return true;
    }

    bool number_value (double d)
    {
      std::cout << "DOUBLE_VALUE: " << d << std::endl;
      return true;
    }


  };

  struct error_json_context
  {
    using char_type   = char                ;
    using string_type = std::string         ;
    using iter_type   = char const*         ;

    iter_type                 error_pos     ;
    string_type               current_string;

    std::vector<char_type>    exp_chars     ;
    std::vector<string_type>  exp_tokens    ;
    std::vector<string_type>  unexp_tokens  ;

    error_json_context ()
      : error_pos (0)
    {
      exp_chars.reserve    (default_size);
      exp_tokens.reserve   (default_size);
      unexp_tokens.reserve (default_size);
    }

    void expected_char (iter_type current, char_type ch)
    {
      if (error_pos == current)
      {
        exp_chars.push_back (ch);
      }
    }

    void expected_chars (iter_type current, string_type const & chs)
    {
      if (error_pos == current)
      {
        for (auto && ch : chs)
        {
          expected_char (current, ch);
        }
      }
    }

    void expected_token (iter_type current, string_type const & token)
    {
      if (error_pos == current)
      {
        exp_tokens.push_back (token);
      }
    }

    void unexpected_token (iter_type current, string_type const & token)
    {
      if (error_pos == current)
      {
        unexp_tokens.push_back (token);
      }
    }

    inline void clear_string ()
    {
    }

    inline void push_char (char_type /*ch*/)
    {
    }

    inline string_type const & get_string () throw ()
    {
      return current_string;
    }

    inline bool array_begin ()
    {
      return true;
    }

    inline bool array_end ()
    {
      return true;
    }

    inline bool object_begin ()
    {
      return true;
    }

    inline bool member_key (string_type const & /*s*/)
    {
      return true;
    }

    inline bool object_end ()
    {
      return true;
    }

    inline bool bool_value (bool /*b*/)
    {
      return true;
    }

    inline bool null_value ()
    {
      return true;
    }

    inline bool string_value (string_type const & /*s*/)
    {
      return true;
    }

    inline bool number_value (double /*d*/)
    {
      return true;
    }
  };

  bool parse (string_type const & json)
  {
    auto begin  = json.c_str ()       ;
    auto end    = begin + json.size ();
    cpp_json::json_parser<default_json_context> jp (begin, end);

    if (jp.try_parse__json ())
    {
      return true;
    }
    else
    {
      cpp_json::json_parser<error_json_context> ejp (begin, end);
      ejp.error_pos = jp.current;
      ejp.try_parse__json ();

      auto sz = ejp.exp_chars.size () + ejp.exp_tokens.size ();

      std::vector<string_type> expected   = std::move (ejp.exp_tokens);
      std::vector<string_type> unexpected = std::move (ejp.unexp_tokens);

      expected.reserve (sz);

      for (auto ch : ejp.exp_chars)
      {
        char token[] = {'\'', ch, '\'', 0};
        expected.push_back (string_type (token));
      }

      std::sort (expected.begin (), expected.end ());
      std::sort (unexpected.begin (), unexpected.end ());

      expected.erase (std::unique (expected.begin (), expected.end ()), expected.end ());
      unexpected.erase (std::unique (unexpected.begin (), unexpected.end ()), unexpected.end ());

      std::cout << "ERROR_POS: " << (ejp.error_pos - begin) << std::endl;

      for (auto && e : expected)
      {
        std::cout << "EXPECTED:" << e << std::endl;
      }

      for (auto && ue : unexpected)
      {
        std::cout << "UNEXPECTED:" << ue << std::endl;
      }

      return false;
    }
  }

}

int main()
{
//  std::string json = R"([null, 123,-1.23E2,"Test\tHello", true,false, [true,null],[],{}, {"x":true}])";
  std::string json = R"({:null})";

  if (default_cpp_json::parse (json))
  {
    std::cout << "SUCCESS!" << std::endl;
  }
  else
  {
    std::cout << "FAILURE!" << std::endl;
  }

  return 0;
}

