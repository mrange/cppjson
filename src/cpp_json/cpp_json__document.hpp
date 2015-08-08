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
#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <tuple>

#include "cpp_json__parser.hpp"

namespace cpp_json { namespace document
{
  using string_type           = std::wstring            ;
  using strings_type          = std::vector<string_type>;
  using char_type             = string_type::value_type ;
  using iter_type             = char_type const *       ;

  struct json_element__null   ;
  struct json_element__bool   ;
  struct json_element__number ;
  struct json_element__string ;
  struct json_element__array  ;
  struct json_element__object ;
  struct json_element__error  ;

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
    virtual void visit (json_element__object  & v) = 0;
    virtual void visit (json_element__array   & v) = 0;
    virtual void visit (json_element__error   & v) = 0;
  };

  struct json_element
  {
    using ptr = std::shared_ptr<json_element> ;

    json_element ()           = default;
    virtual ~json_element ()  = default;

    CPP_JSON__NO_COPY_MOVE (json_element);

    virtual std::size_t   size      () const                          = 0;
    virtual ptr           at        (std::size_t idx) const           = 0;

    virtual ptr           get       (string_type const & name) const  = 0;
    virtual strings_type  names     () const                          = 0;

    virtual bool          is_error  () const                          = 0;
    virtual bool          is_scalar () const                          = 0;

    virtual bool          is_null   () const                          = 0;
    virtual bool          as_bool   () const                          = 0;
    virtual double        as_number () const                          = 0;
    virtual string_type   as_string () const                          = 0;

    virtual void          apply     (json_element_visitor & v)        = 0;
  };

  struct json_element__error : json_element
  {
    using tptr  = std::shared_ptr<json_element__error> ;

    std::size_t size () const override
    {
      return 0;
    }

    ptr at (std::size_t /*idx*/) const override
    {
      return std::make_shared<json_element__error> ();
    }

    ptr get (string_type const & /*name*/) const  override
    {
      return std::make_shared<json_element__error> ();
    }
    strings_type names () const override
    {
      return strings_type ();
    }

    bool is_error () const override
    {
      return true;
    }

    bool is_scalar () const override
    {
      return false;
    }

    bool is_null () const override
    {
      return false;
    }
    bool as_bool () const override
    {
      return false;
    }
    double as_number () const override
    {
      return 0.0;
    }
    string_type as_string () const override
    {
      // TODO:
      return L"";
    }

    void apply (json_element_visitor & v) override
    {
      v.visit (*this);
    }
  };

  struct json_element__value : json_element
  {
    std::size_t size () const override
    {
      return 0;
    }

    ptr at (std::size_t /*idx*/) const override
    {
      return std::make_shared<json_element__error> ();
    }

    ptr get (string_type const & /*name*/) const  override
    {
      return std::make_shared<json_element__error> ();
    }
    strings_type names () const override
    {
      return strings_type ();
    }

    bool is_error () const override
    {
      return false;
    }

    bool is_scalar () const override
    {
      return true;
    }
  };

  struct json_element__null : json_element__value
  {
    using tptr  = std::shared_ptr<json_element__null> ;


    bool is_null () const override
    {
      return true;
    }
    bool as_bool () const override
    {
      return false;
    }
    double as_number () const override
    {
      return 0.0;
    }
    string_type as_string () const override
    {
      return L"null";
    }

    void apply (json_element_visitor & v) override
    {
      v.visit (*this);
    }
  };

  struct json_element__bool : json_element__value
  {
    using tptr  = std::shared_ptr<json_element__bool> ;

    bool value;

    json_element__bool ()
      : value (false)
    {
    }

    bool is_null () const override
    {
      return false;
    }
    bool as_bool () const override
    {
      return value;
    }
    double as_number () const override
    {
      return value ? 1.0 : 0.0;
    }
    string_type as_string () const override
    {
      return value ? L"true" : L"false";
    }

    void apply (json_element_visitor & v) override
    {
      v.visit (*this);
    }
  };

  struct json_element__number : json_element__value
  {
    using tptr  = std::shared_ptr<json_element__number> ;

    double value;

    json_element__number ()
      : value (0.0)
    {
    }

    bool is_null () const override
    {
      return false;
    }
    bool as_bool () const override
    {
      return value != 0.0;
    }
    double as_number () const override
    {
      return value;
    }
    string_type as_string () const override
    {
      constexpr auto bsz = 64U;
      wchar_t buffer[bsz];
      std::swprintf (buffer, bsz, L"%G", value);

      return buffer;
    }

    void apply (json_element_visitor & v) override
    {
      v.visit (*this);
    }
  };

  struct json_element__string : json_element__value
  {
    using tptr  = std::shared_ptr<json_element__string> ;

    string_type value;

    bool is_null () const override
    {
      return false;
    }
    bool as_bool () const override
    {
      return !value.empty ();
    }
    double as_number () const override
    {
      return std::stod (value);
    }
    string_type as_string () const override
    {
      return value;
    }

    void apply (json_element_visitor & v) override
    {
      v.visit (*this);
    }
  };

  struct json_element__array : json_element
  {
    using tptr  = std::shared_ptr<json_element__array>  ;

    std::vector<json_element::ptr> value;

    std::size_t size () const override
    {
      return value.size ();
    }

    ptr at (std::size_t idx) const override
    {
      if (idx < value.size ())
      {
        return value[idx];
      }
      else
      {
        return std::make_shared<json_element__error> ();
      }
    }

    ptr get (string_type const & /*name*/) const  override
    {
      return std::make_shared<json_element__error> ();
    }
    strings_type names () const override
    {
      return strings_type ();
    }

    bool is_error  () const override
    {
      return false;
    }

    bool is_scalar () const override
    {
      return false;
    }

    bool is_null () const override
    {
      return false;
    }
    bool as_bool () const override
    {
      return false;
    }
    double as_number () const override
    {
      return 0.0;
    }
    string_type as_string () const override
    {
      return L"";
    }

    void apply (json_element_visitor & v) override
    {
      v.visit (*this);
    }
  };

  struct json_element__object : json_element
  {
    using tptr  = std::shared_ptr<json_element__object> ;

    std::vector<std::tuple<string_type, json_element::ptr>> value;

    std::size_t size () const override
    {
      return value.size ();
    }

    ptr at (std::size_t idx) const override
    {
      if (idx < value.size ())
      {
        return std::get<1> (value[idx]);
      }
      else
      {
        return std::make_shared<json_element__error> ();
      }
    }

    ptr get (string_type const & name) const  override
    {
      for (auto && kv : value)
      {
        if (std::get<0> (kv) == name)
        {
          return std::get<1> (kv);
        }
      }

      return std::make_shared<json_element__error> ();
    }
    strings_type names () const override
    {
      strings_type result;
      result.reserve (value.size ());

      for (auto && kv : value)
      {
        result.push_back (std::get<0> (kv));
      }

      return result;
    }

    bool is_error  () const override
    {
      return false;
    }

    bool is_scalar () const override
    {
      return false;
    }

    bool is_null () const override
    {
      return false;
    }
    bool as_bool () const override
    {
      return false;
    }
    double as_number () const override
    {
      return 0.0;
    }
    string_type as_string () const override
    {
      return L"";
    }

    void apply (json_element_visitor & v) override
    {
      v.visit (*this);
    }
  };

  namespace details
  {
    constexpr auto default_size = 16U         ;

    struct json_non_printable_chars
    {
      using non_printable_char  = std::array<char_type          , 8 >;
      using non_printable_chars = std::array<non_printable_char , 32>;

      json_non_printable_chars ()
      {
        for (auto iter = 0U; iter < table.size (); ++iter)
        {
          auto && v = table[iter];
          std::fill (v.begin (), v.end (), 0);

          auto fmt = L"\\u%04x";

          switch (iter)
          {
          case '\b':
            fmt = L"\\b";
            break;
          case '\f':
            fmt = L"\\f";
            break;
          case '\n':
            fmt = L"\\n";
            break;
          case '\r':
            fmt = L"\\r";
            break;
          case '\t':
            fmt = L"\\t";
            break;
          }

          std::swprintf (v.data (), v.size (), fmt, iter);
        }
      }

      inline void append (string_type & s, char_type ch) noexcept
      {
        if (ch < table.size ())
        {
          auto p = table[ch].data ();
          while (*p)
          {
            s += *p++;
          }
        }
        else
        {
          s += ch;
        }
      }

    private:
      non_printable_chars table;
    };

    struct json_element_visitor__to_string : json_element_visitor
    {
      static json_non_printable_chars non_printable_chars;

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
          non_printable_chars.append (value, c);
          break;
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
          std::swprintf (buffer, bsz, L"%G", v.value);
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

      void visit (json_element__error  & v) override
      {
        str (v.as_string ());
      }
    };

    json_non_printable_chars json_element_visitor__to_string::non_printable_chars;

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

    struct builder_json_context
    {
      using string_type = string_type ;
      using char_type   = char_type   ;
      using iter_type   = iter_type   ;

      string_type                             current_string  ;

      std::vector<json_element_context::ptr>  element_context ;

      builder_json_context ()
      {
        current_string.reserve (default_size);
        element_context.push_back (std::make_shared<json_element_context__root> ());
      }

      CPP_JSON__NO_COPY_MOVE (builder_json_context);

      inline void expected_char (std::size_t /*pos*/, char_type /*ch*/) noexcept
      {
      }

      inline void expected_chars (std::size_t /*pos*/, string_type const & /*chs*/) noexcept
      {
      }

      inline void expected_token (std::size_t /*pos*/, string_type const & /*token*/) noexcept
      {
      }

      inline void unexpected_token (std::size_t /*pos*/, string_type const & /*token*/) noexcept
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

      inline string_type const & get_string () noexcept
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

      inline string_type const & get_string () noexcept
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

  // Parses a JSON string into a JSON document 'result' if successful.
  //  'pos' indicates the first non-consumed character (which may lay beyond the last character in the input string)
  bool parse (string_type const & json, std::size_t & pos, json_element::ptr & result)
  {
    auto begin  = json.c_str ()       ;
    auto end    = begin + json.size ();
    cpp_json::parser::json_parser<details::builder_json_context> jp (begin, end);

    if (jp.try_parse__json ())
    {
      pos     = static_cast<std::size_t> (jp.current - begin);
      result  = jp.root ();
      return true;
    }
    else
    {
      pos = static_cast<std::size_t> (jp.current - begin);
      result.reset ();
      return false;
    }
  }

  // Parses a JSON string into a JSON document 'result' if successful.
  //  If parse fails 'error' contains an error description.
  //  'pos' indicates the first non-consumed character (which may lay beyond the last character in the input string)
  bool parse (string_type const & json, std::size_t & pos, json_element::ptr & result, string_type & error)
  {
    auto begin  = json.c_str ()       ;
    auto end    = begin + json.size ();
    cpp_json::parser::json_parser<details::builder_json_context> jp (begin, end);

    if (jp.try_parse__json ())
    {
      pos     = static_cast<std::size_t> (jp.current - begin);
      result  = jp.root ();
      return true;
    }
    else
    {
      pos = static_cast<std::size_t> (jp.current - begin);
      result.reset ();

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
        std::swprintf (spos, bsz, L"%zd", pos);
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

      append (L"Expected: "   , expected);
      append (L"Unexpected: " , unexpected);

      error = std::move (msg);

      return false;
    }
  }

  // Creates a string from a JSON document
  //  Note: JSON root element is expected to be either an array or object value, if 'json' argument
  //  is neither to_string will return a string that is not valid JSON.
  string_type to_string (json_element::ptr const & json)
  {
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
