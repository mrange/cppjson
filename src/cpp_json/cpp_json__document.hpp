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
#include <cwchar>
#include <cstdio>
#include <deque>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <tuple>

#include "cpp_json__parser.hpp"

namespace cpp_json { namespace document
{
  using doc_string_type   = std::wstring                ;
  using doc_strings_type  = std::vector<doc_string_type>;
  using doc_char_type     = doc_string_type::value_type ;
  using doc_iter_type     = doc_char_type const *       ;

  constexpr auto default_size = 16U;

  namespace details
  {
    struct json_element__null   ;
    struct json_element__bool   ;
    struct json_element__number ;
    struct json_element__string ;
    struct json_element__array  ;
    struct json_element__object ;
    struct json_element__error  ;

    struct json_document__impl  ;
  }

  // Implement json_element_visitor to traverse the JSON DOM using 'apply' method
  struct json_element_visitor
  {
    using ptr = std::shared_ptr<json_element_visitor> ;

    json_element_visitor ()           = default;
    virtual ~json_element_visitor ()  = default;

    CPP_JSON__NO_COPY_MOVE (json_element_visitor);

    virtual bool visit (details::json_element__null   const & v) = 0;
    virtual bool visit (details::json_element__bool   const & v) = 0;
    virtual bool visit (details::json_element__number const & v) = 0;
    virtual bool visit (details::json_element__string const & v) = 0;
    virtual bool visit (details::json_element__object const & v) = 0;
    virtual bool visit (details::json_element__array  const & v) = 0;
    virtual bool visit (details::json_element__error  const & v) = 0;
  };

  struct json_element
  {
    using ptr = json_element const *;

    json_element ()           = default;
    virtual ~json_element ()  = default;

    CPP_JSON__NO_COPY_MOVE (json_element);

    // Returns the number of children (object/array)
    virtual std::size_t       size      () const                              = 0;
    // Returns the child at index (object/array)
    //  if out of bounds returns an error DOM element
    virtual ptr               at        (std::size_t idx) const               = 0;

    // Returns the child with name (object)
    //  if not found returns an error DOM element
    virtual ptr               get       (doc_string_type const & name) const  = 0;
    // Returns all member names (object)
    //  May contain duplicates, is in order
    virtual doc_strings_type  names     () const                              = 0;

    // Returns true if DOM element represents an error
    virtual bool              is_error  () const                              = 0;
    // Returns true if DOM element represents an scalar
    virtual bool              is_scalar () const                              = 0;

    // Returns true if DOM element represents a null value
    virtual bool              is_null   () const                              = 0;
    // Converts the value to a boolean value
    virtual bool              as_bool   () const                              = 0;
    // Converts the value to a double value
    virtual double            as_number () const                              = 0;
    // Converts the value to a string value
    virtual doc_string_type   as_string () const                              = 0;

    // Applies the JSON element visitor to the element
    virtual bool              apply     (json_element_visitor & v) const      = 0;
  };

  struct json_document
  {
    using ptr = std::shared_ptr<json_document>  ;

    json_document ()          = default;
    virtual ~json_document () = default;

    CPP_JSON__NO_COPY_MOVE (json_document);

    virtual json_element::ptr root () const = 0;

    // Parses a JSON string into a JSON document 'result' if successful.
    //  'pos' indicates the first non-consumed character (which may lay beyond the last character in the input string)
    static bool parse (doc_string_type const & json, std::size_t & pos, json_document::ptr & result);

    // Parses a JSON string into a JSON document 'result' if successful.
    //  If parse fails 'error' contains an error description.
    //  'pos' indicates the first non-consumed character (which may lay beyond the last character in the input string)
    static bool parse (doc_string_type const & json, std::size_t & pos, json_document::ptr & result, doc_string_type & error);

    // Creates a string from a JSON document
    //  Note: JSON root element is expected to be either an array or object value, if 'json' argument
    //  is neither to_string will return a string that is not valid JSON.
    static doc_string_type to_string (json_document::ptr const & json);
  };


  namespace details
  {
    using array_members   = std::vector<json_element::ptr>                              ;
    using object_members  = std::vector<std::tuple<doc_string_type, json_element::ptr>> ;

    struct utils
    {
      static void to_string (doc_string_type & value, double d)
      {
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
          std::swprintf (buffer, bsz, L"%G", d);
          value += buffer;
        }
      }
    };

    struct json_non_printable_chars
    {
      using non_printable_char  = std::array<doc_char_type      , 8 >;
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

      static json_non_printable_chars const & get ()
      {
        // TODO: Race condition issue (until compilers widely support magic statics)
        static json_non_printable_chars non_printable_chars;
        return non_printable_chars;
      }

      inline void append (doc_string_type & s, doc_char_type ch) const noexcept
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

    struct json_element__base : json_element
    {
      json_document__impl const * doc;

      explicit json_element__base (json_document__impl const * doc)
        : doc (doc)
      {
      }

      ptr error_element () const;
    };

    struct json_element__scalar : json_element__base
    {
      explicit json_element__scalar (json_document__impl const * doc)
        : json_element__base (doc)
      {
      }

      std::size_t size () const override
      {
        return 0;
      }

      ptr at (std::size_t /*idx*/) const override
      {
        return error_element ();
      }

      ptr get (doc_string_type const & /*name*/) const override
      {
        return error_element ();
      }

      doc_strings_type names () const override
      {
        return doc_strings_type ();
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

    struct json_element__null : json_element__scalar
    {
      explicit json_element__null (json_document__impl const * doc)
        : json_element__scalar (doc)
      {
      }

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
      doc_string_type as_string () const
      {
        return L"null";
      }

      bool apply (json_element_visitor & v) const
      {
        return v.visit (*this);
      }
    };

    struct json_element__bool : json_element__scalar
    {
      bool const value;

      explicit json_element__bool (json_document__impl const * doc, bool v)
        : json_element__scalar (doc)
        , value (v)
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
      doc_string_type as_string () const override
      {
        return value
          ? L"true"
          : L"false"
          ;
      }

      bool apply (json_element_visitor & v) const
      {
        return v.visit (*this);
      }
    };

    struct json_element__number : json_element__scalar
    {
      double const value;

      explicit json_element__number (json_document__impl const * doc, double v)
        : json_element__scalar (doc)
        , value (v)
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
      doc_string_type as_string () const override
      {
        doc_string_type result;
        result.reserve (default_size);
        utils::to_string (result, value);
        return result;
      }

      bool apply (json_element_visitor & v) const
      {
        return v.visit (*this);
      }
    };

    struct json_element__string : json_element__scalar
    {
      doc_string_type const value;

      explicit json_element__string (json_document__impl const * doc, doc_string_type v)
        : json_element__scalar (doc)
        , value (std::move (v))
      {
      }

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
        doc_char_type * e = nullptr;
        return std::wcstof (value.c_str (), &e);
      }
      doc_string_type as_string () const override
      {
        return value;
      }

      bool apply (json_element_visitor & v) const
      {
        return v.visit (*this);
      }
    };

    struct json_element__container : json_element__base
    {
      explicit json_element__container (json_document__impl const * doc)
        : json_element__base (doc)
      {
      }

      bool is_error () const override
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
      doc_string_type as_string () const override
      {
        return doc_string_type ();
      }
    };

    struct json_element__array : json_element__container
    {
      std::size_t const begin ;
      std::size_t const end   ;

      explicit json_element__array  (json_document__impl const * doc, std::size_t b, std::size_t e)
        : json_element__container   (doc)
        , begin                     (b)
        , end                       (e)
      {
        CPP_JSON__ASSERT (b <= e);
      }

      std::size_t size () const override
      {
        return end - begin;
      }

      ptr at (std::size_t idx) const override;

      ptr get (doc_string_type const & /*name*/) const override
      {
        return error_element ();
      }

      doc_strings_type names () const override
      {
        return doc_strings_type ();
      }

      bool apply (json_element_visitor & v) const
      {
        return v.visit (*this);
      }
    };

    struct json_element__object : json_element__container
    {
      std::size_t const begin ;
      std::size_t const end   ;

      explicit json_element__object (json_document__impl const * doc, std::size_t b, std::size_t e)
        : json_element__container   (doc)
        , begin                     (b)
        , end                       (e)
      {
        CPP_JSON__ASSERT (b <= e);
      }

      std::size_t size () const override
      {
        return end - begin;
      }
      ptr at (std::size_t idx) const override;

      ptr get (doc_string_type const & name) const override;
      doc_strings_type names () const override;

      bool apply (json_element_visitor & v) const
      {
        return v.visit (*this);
      }
    };

    struct json_element__error : json_element__base
    {
      explicit json_element__error (json_document__impl const * doc)
        : json_element__base (doc)
      {
      }

      std::size_t size () const override
      {
        return 0;
      }

      ptr at (std::size_t /*idx*/) const override
      {
        return error_element ();
      }

      ptr get (doc_string_type const & /*name*/) const override
      {
        return error_element ();
      }

      doc_strings_type names () const override
      {
        return doc_strings_type ();
      }

      bool is_error () const override
      {
        return true;
      }
      bool is_scalar () const override
      {
        return true;
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
      doc_string_type as_string () const override
      {
        return L"\"error\"";
      }

      bool apply (json_element_visitor & v) const
      {
        return v.visit (*this);
      }
    };

    struct json_document__impl : json_document
    {
      using tptr = std::shared_ptr<json_document__impl>   ;

      json_element__null  const         null_value        ;
      json_element__bool  const         true_value        ;
      json_element__bool  const         false_value       ;
      json_element__error const         error_value       ;

      std::deque<json_element__number>  number_values     ;
      std::deque<json_element__string>  string_values     ;
      std::deque<json_element__object>  object_values     ;
      std::deque<json_element__array >  array_values      ;

      std::deque<json_element::ptr>     all_array_members ;
      std::deque<std::tuple<
          doc_string_type
        , json_element::ptr
        >>                              all_object_members;

      json_element::ptr                 root_value        ;

      json_document__impl ()
        : null_value  (this)
        , true_value  (this, true)
        , false_value (this, false)
        , error_value (this)
        , root_value  (&null_value)
      {
      }

      json_element::ptr root () const override
      {
        return root_value;
      }

      details::json_element__number * create_number (double v)
      {
        number_values.emplace_back (this, v);
        return & number_values.back ();
      }

      details::json_element__string * create_string (doc_string_type && v)
      {
        string_values.emplace_back (this, std::move (v));
        return & string_values.back ();
      }

      details::json_element__array * create_array (array_members & members)
      {
        auto b = all_array_members.size ();

        for (auto && m : members)
        {
          all_array_members.push_back (std::move (m));
        }

        auto e = all_array_members.size ();

        members.clear ();

        array_values.emplace_back (this, b, e);

        return & array_values.back ();
      }

      details::json_element__object * create_object (object_members & members)
      {
        auto b = all_object_members.size ();

        for (auto && m : members)
        {
          all_object_members.push_back (std::move (m));
        }

        auto e = all_object_members.size ();

        members.clear ();

        object_values.emplace_back (this, b, e);

        return & object_values.back ();
      }

    };

    struct json_element_visitor__to_string : json_element_visitor
    {
      doc_string_type value;

      inline void ch (doc_char_type c)
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
          json_non_printable_chars::get ().append (value, c);
          break;
        }
      }

      inline void str (doc_string_type const & s)
      {
        value += L'"';
        for (auto && c : s)
        {
          ch (c);
        }
        value += L'"';
      }

      bool visit (json_element__null    const & /*v*/) override
      {
        value += L"null";

        return true;
      }

      bool visit (json_element__bool    const & v) override
      {
        value += (v.value ? L"true" : L"false");

        return true;
      }

      bool visit (json_element__number  const & v) override
      {
        utils::to_string (value, v.value);

        return true;
      }

      bool visit (json_element__string  const & v) override
      {
        str (v.value);

        return true;
      }

      bool visit (json_element__array   const & v) override
      {
        value += L'[';
        auto b = v.begin;
        auto e = b + v.size ();
        for (auto iter = b; iter < e; ++iter)
        {
          if (iter > b)
          {
            value += L", ";
          }

          auto && c = v.doc->all_array_members[iter];
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

        return true;
      }

      bool visit (json_element__object  const & v) override
      {
        value += L'{';
        auto b = v.begin;
        auto e = b + v.size ();
        for (auto iter = b; iter < e; ++iter)
        {
          if (iter > b)
          {
            value += L", ";
          }

          auto && kv  = v.doc->all_object_members[iter];

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

        return true;
      }

      bool visit (json_element__error const & v) override
      {
        str (v.as_string ());

        return true;
      }
    };

    struct json_element_context : std::enable_shared_from_this<json_element_context>
    {
      using ptr   = std::shared_ptr<json_element_context> ;
      using ptrs  = std::vector<ptr> ;

      json_document__impl & document;

      json_element_context (json_document__impl & doc)
        : document (doc)
      {
      }
      virtual ~json_element_context ()  = default;

      CPP_JSON__NO_COPY_MOVE (json_element_context);

      virtual bool              add_value       (json_element::ptr const & json ) = 0;
      virtual bool              set_key         (doc_string_type && key         ) = 0;
      virtual json_element::ptr create_element  (
          ptrs & array_contexts
        , ptrs & object_contexts
        ) = 0;
    };

    using json_element_contexts = json_element_context::ptrs;

    struct json_element_context__root : json_element_context
    {
      json_element_context__root (json_document__impl & doc)
        : json_element_context (doc)
      {
      }

      virtual bool add_value (json_element::ptr const & json) override
      {
        CPP_JSON__ASSERT (json);
        document.root_value = json;

        return true;
      }

      virtual bool set_key (doc_string_type && /*key*/) override
      {
        CPP_JSON__ASSERT (false);

        return true;
      }

      virtual json_element::ptr create_element (
          json_element_contexts & /*array_contexts */
        , json_element_contexts & /*object_contexts*/
        ) override
      {
        return document.root_value;
      }

    };

    struct json_element_context__array : json_element_context
    {
      array_members values;

      json_element_context__array (json_document__impl & doc)
        : json_element_context (doc)
      {
      }

      virtual bool add_value (json_element::ptr const & json) override
      {
        CPP_JSON__ASSERT (json);
        values.push_back (json);

        return true;
      }

      virtual bool set_key (doc_string_type && /*key*/) override
      {
        CPP_JSON__ASSERT (false);

        return true;
      }

      virtual json_element::ptr create_element (
          json_element_contexts & array_contexts
        , json_element_contexts & /*object_contexts*/
        ) override
      {
        auto result = document.create_array (values);
        array_contexts.push_back (shared_from_this ());
        return result;
      }

    };

    struct json_element_context__object : json_element_context
    {
      doc_string_type key   ;
      object_members  values;

      json_element_context__object (json_document__impl & doc)
        : json_element_context (doc)
      {
      }

      virtual bool add_value (json_element::ptr const & json) override
      {
        CPP_JSON__ASSERT (json);
        values.push_back (std::make_tuple (std::move (key), json));

        return true;
      }

      virtual bool set_key (doc_string_type && k) override
      {
        key = std::move (k);

        return true;
      }

      virtual json_element::ptr create_element (
          json_element_contexts & /*array_contexts */
        , json_element_contexts & object_contexts
        ) override
      {
        auto result = document.create_object (values);
        object_contexts.push_back (shared_from_this ());
        return result;
      }

    };

    struct builder_json_context
    {
      using string_type     = doc_string_type ;
      using char_type       = doc_char_type   ;
      using iter_type       = doc_iter_type   ;

      json_document__impl::tptr document      ;

      std::vector<char_type>  current_string  ;

      json_element_contexts   element_context ;
      json_element_contexts   array_contexts  ;
      json_element_contexts   object_contexts ;

      builder_json_context ()
        : document (std::make_shared<json_document__impl> ())
      {
        current_string.reserve  (default_size);
        element_context.reserve (default_size);
        element_context.reserve (default_size);
        element_context.reserve (default_size);
        element_context.push_back (std::make_shared<json_element_context__root> (*document));
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

      inline string_type get_string () noexcept
      {
        return string_type (current_string.begin (), current_string.end ());
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

        auto && element = back->create_element (array_contexts, object_contexts);
        CPP_JSON__ASSERT (element);

        next->add_value (element);

        return true;
      }

      bool array_begin ()
      {
        if (array_contexts.empty ())
        {
          element_context.push_back (std::make_shared<json_element_context__array> (*document));
        }
        else
        {
          element_context.push_back (array_contexts.back ());
          array_contexts.pop_back ();
        }
        return true;
      }

      bool array_end ()
      {
        return pop ();
      }

      bool object_begin ()
      {
        if (object_contexts.empty ())
        {
          element_context.push_back (std::make_shared<json_element_context__object> (*document));
        }
        else
        {
          element_context.push_back (object_contexts.back ());
          object_contexts.pop_back ();
        }
        return true;
      }

      bool member_key (string_type && s)
      {
        CPP_JSON__ASSERT (!element_context.empty ());
        auto && back = element_context.back ();
        CPP_JSON__ASSERT (back);
        back->set_key (std::move (s));

        return true;
      }

      bool object_end ()
      {
        return pop ();
      }

      bool bool_value (bool b)
      {
        auto v = b
          ? &document->true_value
          : &document->false_value
          ;

        CPP_JSON__ASSERT (!element_context.empty ());
        auto && back = element_context.back ();
        CPP_JSON__ASSERT (back);
        back->add_value (v);

        return true;
      }

      bool null_value ()
      {
        auto v = &document->null_value;

        CPP_JSON__ASSERT (!element_context.empty ());
        auto && back = element_context.back ();
        CPP_JSON__ASSERT (back);
        back->add_value (v);

        return true;
      }

      bool string_value (string_type && s)
      {
        auto v = document->create_string (std::move (s));

        CPP_JSON__ASSERT (!element_context.empty ());
        auto && back = element_context.back ();
        CPP_JSON__ASSERT (back);
        back->add_value (v);

        return true;
      }

      bool number_value (double d)
      {
        auto v = document->create_number (d);

        CPP_JSON__ASSERT (!element_context.empty ());
        auto && back = element_context.back ();
        CPP_JSON__ASSERT (back);
        back->add_value (v);

        return true;
      }

    };

    struct error_json_context
    {
      using string_type = doc_string_type ;
      using char_type   = doc_char_type   ;
      using iter_type   = doc_iter_type   ;

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
} }
