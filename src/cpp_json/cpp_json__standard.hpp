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

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <tuple>

#include "cpp_json__parser.hpp"

namespace cpp_json { namespace standard
{
  using string_type           = std::wstring            ;
  using char_type             = string_type::value_type ;
  using iter_type             = char_type const *       ;

  struct json_element__null   ;
  struct json_element__bool   ;
  struct json_element__number ;
  struct json_element__string ;
  struct json_element__array  ;
  struct json_element__object ;

  struct json_element_visitor
  {
    using ptr = std::shared_ptr<json_element_visitor> ;

    json_element_visitor ()           = default;
    virtual ~json_element_visitor ()  = default;

    CPP_JSON__NO_COPY_MOVE (json_element_visitor);

    virtual void visit (json_element__null    & v) = 0;
    virtual void visit (json_element__bool    & v) = 0;
    virtual void visit (json_element__number  & v) = 0;
    virtual void visit (json_element__string  & v) = 0;
    virtual void visit (json_element__array   & v) = 0;
    virtual void visit (json_element__object  & v) = 0;
  };

  struct json_element
  {
    using ptr = std::shared_ptr<json_element> ;

    json_element ()           = default;
    virtual ~json_element ()  = default;

    CPP_JSON__NO_COPY_MOVE (json_element);

    virtual void apply (json_element_visitor & v) = 0;
  };

  struct json_element__null : json_element
  {
    using tptr  = std::shared_ptr<json_element__null> ;

    void apply (json_element_visitor & v) override
    {
      v.visit (*this);
    }
  };

  struct json_element__bool : json_element
  {
    using tptr  = std::shared_ptr<json_element__bool> ;

    bool value;

    json_element__bool ()
      : value (false)
    {
    }

    void apply (json_element_visitor & v) override
    {
      v.visit (*this);
    }
  };

  struct json_element__number : json_element
  {
    using tptr  = std::shared_ptr<json_element__number> ;

    double value;

    json_element__number ()
      : value (0.0)
    {
    }

    void apply (json_element_visitor & v) override
    {
      v.visit (*this);
    }
  };

  struct json_element__string : json_element
  {
    using tptr  = std::shared_ptr<json_element__string> ;

    string_type value;

    void apply (json_element_visitor & v) override
    {
      v.visit (*this);
    }
  };

  struct json_element__array : json_element
  {
    using tptr  = std::shared_ptr<json_element__array>  ;

    std::vector<json_element::ptr> value;

    void apply (json_element_visitor & v) override
    {
      v.visit (*this);
    }
  };

  struct json_element__object : json_element
  {
    using tptr  = std::shared_ptr<json_element__object> ;

    std::vector<std::tuple<string_type, json_element::ptr>> value;

    void apply (json_element_visitor & v) override
    {
      v.visit (*this);
    }
  };

  namespace details
  {
    constexpr auto default_size = 16U         ;

    struct json_element_visitor__to_string : json_element_visitor
    {
      string_type value;

      inline void ch (char_type c)
      {
        switch (c)
        {
        case '\"':
          value += L"\\\"";
          break;
        case '\\':
          value += L"\\\\";
          break;
        case '/':
          value += L"\\/";
          break;
        default:
          /* TODO:
          if (c >= 0 && c < non_printable_chars.size ())
          {
            result << non_printable_chars[c];
          }
          else
          {
          }
          */
          value += c;
        }
      }

      inline void str (string_type const & s)
      {
        value += L'"';
        for (auto && c : s)
        {
          ch (c);
        }
        value += L'"';
      }

      void visit (json_element__null    & /*v*/) override
      {
        value += L"null";
      }

      void visit (json_element__bool    & v) override
      {
        value += (v.value ? L"true" : L"false");
      }

      void visit (json_element__number  & v) override
      {
        auto d = v.value;
        if (std::isnan (d))
        {
          value += L"\"NaN\"";
        }
        else if (std::isinf (d) && d < 0)
        {
          value += L"\"-Inf\"";
        }
        else if (std::isinf (d))
        {
          value += L"\"+Inf\"";
        }
        else
        {
          constexpr auto bsz = 64U;
          wchar_t buffer[bsz];
          swprintf (buffer, bsz, L"%G", v.value);
          value += buffer;
        }
      }

      void visit (json_element__string  & v) override
      {
        str (v.value);
      }

      void visit (json_element__array   & v) override
      {
        value += L'[';
        auto sz = v.value.size ();
        for (auto iter = 0U; iter < sz; ++iter)
        {
          if (iter > 0U)
          {
            value += L", ";
          }

          auto && c = v.value[iter];
          if (c)
          {
            c->apply (*this);
          }
          else
          {
            value += L"null";
          }
        }
        value += L']';
      }

      void visit (json_element__object  & v) override
      {
        value += L'{';
        auto sz = v.value.size ();
        for (auto iter = 0U; iter < sz; ++iter)
        {
          if (iter > 0U)
          {
            value += L", ";
          }

          auto && kv  = v.value[iter];

          auto && k   = std::get<0> (kv);
          auto && c   = std::get<1> (kv);

          str (k);

          value += L':';

          if (c)
          {
            c->apply (*this);
          }
          else
          {
            value += L"null";
          }
        }
        value += L'}';
      }
    };

    struct json_element_context
    {
      using ptr = std::shared_ptr<json_element_context> ;

      json_element_context ()           = default;
      virtual ~json_element_context ()  = default;

      CPP_JSON__NO_COPY_MOVE (json_element_context);

      virtual bool              add_value    (json_element::ptr const & json )  = 0;
      virtual bool              set_key      (string_type const & key        )  = 0;
      virtual json_element::ptr get_element  ()                                 = 0;
    };

    struct json_element_context__root : json_element_context
    {
      json_element::ptr value;

      json_element_context__root ()
        : value (std::make_shared<json_element__null> ())
      {
      }

      virtual bool add_value (json_element::ptr const & json) override
      {
        CPP_JSON__ASSERT (json);
        value = json;

        return true;
      }

      virtual bool set_key (string_type const & /*key*/) override
      {
        CPP_JSON__ASSERT (false);

        return true;
      }

      virtual json_element::ptr get_element () override
      {
        CPP_JSON__ASSERT (value);

        return value;
      }

    };

    struct json_element_context__array : json_element_context
    {
      json_element__array::tptr value;

      json_element_context__array ()
        : value (std::make_shared<json_element__array> ())
      {
      }

      virtual bool add_value (json_element::ptr const & json) override
      {
        CPP_JSON__ASSERT (value);
        value->value.push_back (json);

        return true;
      }

      virtual bool set_key (string_type const & /*key*/) override
      {
        CPP_JSON__ASSERT (false);

        return true;
      }

      virtual json_element::ptr get_element () override
      {
        return value;
      }

    };

    struct json_element_context__object : json_element_context
    {
      json_element__object::tptr value;

      string_type key;

      json_element_context__object ()
        : value (std::make_shared<json_element__object> ())
      {
      }

      virtual bool add_value (json_element::ptr const & json) override
      {
        CPP_JSON__ASSERT (value);
        value->value.push_back (std::make_tuple (std::move (key), json));

        return true;
      }

      virtual bool set_key (string_type const & k) override
      {
        key = k;

        return true;
      }

      virtual json_element::ptr get_element () override
      {
        return value;
      }

    };

    struct default_json_context
    {
      using string_type = string_type ;
      using char_type   = char_type   ;
      using iter_type   = iter_type   ;

      string_type                             current_string  ;

      std::vector<json_element_context::ptr>  element_context ;

      default_json_context ()
      {
        current_string.reserve (default_size);
        element_context.push_back (std::make_shared<json_element_context__root> ());
      }

      CPP_JSON__NO_COPY_MOVE (default_json_context);

      inline void expected_char (std::size_t /*pos*/, char_type /*ch*/) throw ()
      {
      }

      inline void expected_chars (std::size_t /*pos*/, string_type const & /*chs*/) throw ()
      {
      }

      inline void expected_token (std::size_t /*pos*/, string_type const & /*token*/) throw ()
      {
      }

      inline void unexpected_token (std::size_t /*pos*/, string_type const & /*token*/) throw ()
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

      inline void push_wchar_t (wchar_t ch)
      {
        current_string.push_back (ch);
      }

      inline string_type const & get_string () throw ()
      {
        return current_string;
      }

      template<typename T>
      inline bool push ()
      {
        element_context.push_back (std::make_shared<T> ());
        return true;
      }

      inline bool pop ()
      {
        CPP_JSON__ASSERT (!element_context.empty ());
        auto back = element_context.back ();
        CPP_JSON__ASSERT (back);

        element_context.pop_back ();

        CPP_JSON__ASSERT (!element_context.empty ());
        auto && next = element_context.back ();
        CPP_JSON__ASSERT (next);

        auto && element = back->get_element ();
        CPP_JSON__ASSERT (element);

        next->add_value (element);

        return true;
      }

      bool array_begin ()
      {
        return push<json_element_context__array> ();
      }

      bool array_end ()
      {
        return pop ();
      }

      bool object_begin ()
      {
        return push<json_element_context__object> ();
      }

      bool member_key (string_type const & s)
      {
        CPP_JSON__ASSERT (!element_context.empty ());
        auto && back = element_context.back ();
        CPP_JSON__ASSERT (back);
        back->set_key (s);

        return true;
      }

      bool object_end ()
      {
        return pop ();
      }

      bool bool_value (bool b)
      {
        auto v = std::make_shared<json_element__bool> ();
        v->value = b;

        CPP_JSON__ASSERT (!element_context.empty ());
        auto && back = element_context.back ();
        CPP_JSON__ASSERT (back);
        back->add_value (v);

        return true;
      }

      bool null_value ()
      {
        auto v = std::make_shared<json_element__null> ();

        CPP_JSON__ASSERT (!element_context.empty ());
        auto && back = element_context.back ();
        CPP_JSON__ASSERT (back);
        back->add_value (v);

        return true;
      }

      bool string_value (string_type const & s)
      {
        auto v = std::make_shared<json_element__string> ();
        v->value = s;

        CPP_JSON__ASSERT (!element_context.empty ());
        auto && back = element_context.back ();
        CPP_JSON__ASSERT (back);
        back->add_value (v);

        return true;
      }

      bool number_value (double d)
      {
        auto v = std::make_shared<json_element__number> ();
        v->value = d;

        CPP_JSON__ASSERT (!element_context.empty ());
        auto && back = element_context.back ();
        CPP_JSON__ASSERT (back);
        back->add_value (v);

        return true;
      }

      json_element::ptr root () const
      {
        CPP_JSON__ASSERT (!element_context.empty ());
        CPP_JSON__ASSERT (element_context.front ());
        return element_context.front ()->get_element ();
      }

    };

    struct error_json_context
    {
      using string_type = string_type ;
      using char_type   = char_type   ;
      using iter_type   = iter_type   ;

      std::size_t               error_pos     ;
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

      CPP_JSON__NO_COPY_MOVE (error_json_context);

      void expected_char (std::size_t pos, char_type ch)
      {
        if (error_pos == pos)
        {
          exp_chars.push_back (ch);
        }
      }

      void expected_chars (std::size_t pos, string_type const & chs)
      {
        if (error_pos == pos)
        {
          for (auto && ch : chs)
          {
            expected_char (pos, ch);
          }
        }
      }

      void expected_token (std::size_t pos, string_type const & token)
      {
        if (error_pos == pos)
        {
          exp_tokens.push_back (token);
        }
      }

      void unexpected_token (std::size_t pos, string_type const & token)
      {
        if (error_pos == pos)
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

      inline void push_wchar_t (wchar_t /*ch*/)
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
  }

  bool parse (string_type const & json, std::size_t & pos, json_element::ptr & result, string_type & error)
  {
    auto begin  = json.c_str ()       ;
    auto end    = begin + json.size ();
    cpp_json::parser::json_parser<details::default_json_context> jp (begin, end);

    if (jp.try_parse__json ())
    {
      pos     = static_cast<std::size_t> (jp.current - begin);
      result  = jp.root ();
      return true;
    }
    else
    {
      pos = static_cast<std::size_t> (jp.current - begin);

      cpp_json::parser::json_parser<details::error_json_context> ejp (begin, end);
      ejp.error_pos = jp.pos ();
      auto eresult  = ejp.try_parse__json ();
      CPP_JSON__ASSERT (!eresult);

      auto sz = ejp.exp_chars.size () + ejp.exp_tokens.size ();

      std::vector<string_type> expected   = std::move (ejp.exp_tokens);
      std::vector<string_type> unexpected = std::move (ejp.unexp_tokens);

      expected.reserve (sz);

      for (auto ch : ejp.exp_chars)
      {
        details::error_json_context::char_type token[] = {'\'', ch, '\'', 0};
        expected.push_back (string_type (token));
      }

      std::sort (expected.begin (), expected.end ());
      std::sort (unexpected.begin (), unexpected.end ());

      expected.erase (std::unique (expected.begin (), expected.end ()), expected.end ());
      unexpected.erase (std::unique (unexpected.begin (), unexpected.end ()), unexpected.end ());

      string_type msg;

      auto newline = [&msg] ()
      {
        msg += L'\n';
      };

      msg += L"Failed to parse input as JSON";

      newline ();
      for (auto && c : json)  // TODO: Add json window
      {
        if (c < ' ')
        {
          msg += ' ';
        }
        else
        {
          msg += c;
        }
      }

      newline ();
      for (auto iter = 0U; iter < pos; ++iter)
      {
        msg += L'-';
      }
      msg += L"^ Pos: ";

      {
        constexpr auto bsz = 12U;
        wchar_t spos[bsz]  = {};
        swprintf (spos, bsz, L"%zd", pos);
        msg += spos;
      }

      auto append = [&msg, &newline] (auto && prepend, auto && vs)
      {
        if (!vs.empty ())
        {
          newline ();
          msg += prepend;

          auto sz = vs.size();
          for (auto iter = 0U; iter < sz; ++iter)
          {
            if (iter == 0U)
            {
            }
            else if (iter + 1U == sz)
            {
              msg += L" or ";
            }
            else
            {
              msg += L", ";
            }
            msg += vs[iter];
          }
        }
      };

      append (L"EXPECTED: "   , expected);
      append (L"UNEXPECTED: " , unexpected);

      error = std::move (msg);

      return false;
    }
  }

  string_type to_string (json_element::ptr const & json)
  {
    // TODO: Make sure return valid JSON document at all times
    if (json)
    {
      details::json_element_visitor__to_string visitor;

      json->apply (visitor);

      return std::move (visitor.value);
    }
    else
    {
      return L"null";
    }
  }

} }
