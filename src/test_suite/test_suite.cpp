----------------------------------------------------------------------------------------------
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

#include "../cpp_json/cpp_json__document.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>

#include <deque>
#include <set>

#if _MSC_VER >= 1900
# define CPP_JSON__FILESYSTEM // VS 2015 supports a preliminary version of filesystem
#endif

#ifdef CPP_JSON__FILESYSTEM
#include <filesystem>

using namespace std::tr2::sys;
#endif

#define TEST_EQ(expected, actual) test_eq (__FILE__, __LINE__, expected, #expected, actual, #actual)

void perf__parse_json_callback  (std::wstring const & json_document);
void perf__parse_json_document  (std::wstring const & json_document);
void perf__jsoncpp_document     (std::string const & json_document);

namespace
{
  namespace jd
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

      virtual bool visit (details::json_element__null    & v) = 0;
      virtual bool visit (details::json_element__bool    & v) = 0;
      virtual bool visit (details::json_element__number  & v) = 0;
      virtual bool visit (details::json_element__string  & v) = 0;
      virtual bool visit (details::json_element__object  & v) = 0;
      virtual bool visit (details::json_element__array   & v) = 0;
      virtual bool visit (details::json_element__error   & v) = 0;
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
      virtual bool              apply     (json_element_visitor & v)        = 0;
    };

    struct json_document
    {
      using ptr = std::shared_ptr<json_document>  ;

      json_document ()          = default;
      virtual ~json_document () = default;

      CPP_JSON__NO_COPY_MOVE (json_document);

      virtual json_element::ptr root () const = 0;
    };


    namespace details
    {
      using array_members   = std::vector<json_element::ptr>                              ;
      using object_members  = std::vector<std::tuple<doc_string_type, json_element::ptr>> ;

      void to_string (doc_string_type & value, double d)
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

        bool is_null   () const override
        {
          return true;
        }
        bool as_bool   () const override
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

        bool apply (json_element_visitor & v)
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

        bool is_null   () const override
        {
          return false;
        }
        bool as_bool   () const override
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

        bool apply (json_element_visitor & v)
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

        bool is_null   () const override
        {
          return false;
        }
        bool as_bool   () const override
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
          to_string (result, value);
          return result;
        }

        bool apply (json_element_visitor & v)
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

        bool is_null   () const override
        {
          return false;
        }
        bool as_bool   () const override
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

        bool apply (json_element_visitor & v)
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
        bool is_null   () const override
        {
          return false;
        }
        bool as_bool   () const override
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

        explicit json_element__array (json_document__impl const * doc, std::size_t b, std::size_t e)
          : json_element__container (doc)
          , begin                   (b)
          , end                     (e)
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

        bool apply (json_element_visitor & v)
        {
          return v.visit (*this);
        }
      };

      struct json_element__object : json_element__container
      {
        std::size_t const begin ;
        std::size_t const end   ;

        explicit json_element__object (json_document__impl const * doc, std::size_t b, std::size_t e)
          : json_element__container (doc)
          , begin                   (b)
          , end                     (e)
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

        bool apply (json_element_visitor & v)
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

        bool is_null   () const override
        {
          return false;
        }
        bool as_bool   () const override
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

        bool apply (json_element_visitor & v)
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

        details::json_element__string * create_string (doc_string_type v)
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

      json_element::ptr json_element__base::error_element () const
      {
        return &doc->error_value;
      }

      json_element::ptr json_element__array::at (std::size_t idx) const
      {
        if (idx < size ())
        {
          return doc->all_array_members[idx + begin];
        }
        else
        {
          return error_element ();
        }
      }

      json_element::ptr json_element__object::at (std::size_t idx) const
      {
        if (idx < size ())
        {
          return std::get<1> (doc->all_object_members[idx + begin]);
        }
        else
        {
          return error_element ();
        }
      }

      json_element::ptr json_element__object::get (doc_string_type const & name) const
      {
        for (auto i = begin; i < end; ++i)
        {
          auto && kv = doc->all_object_members[i];
          if (std::get<0> (kv) == name)
          {
            return std::get<1> (kv);
          }
        }
        return error_element ();
      }

      doc_strings_type json_element__object::names () const
      {
        doc_strings_type result;
        result.reserve (size ());
        for (auto i = begin; i < end; ++i)
        {
          auto && kv = doc->all_object_members[i];
          result.push_back (std::get<0> (kv));
        }
        return result;
      }


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

        virtual bool              add_value       (json_element::ptr const & json )  = 0;
        virtual bool              set_key         (doc_string_type const & key     ) = 0;
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

        virtual bool set_key (doc_string_type const & /*key*/) override
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

        virtual bool set_key (doc_string_type const & /*key*/) override
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

        virtual bool set_key (doc_string_type const & k) override
        {
          key = k;

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

        string_type           current_string    ;

        json_element_contexts element_context   ;
        json_element_contexts array_contexts    ;
        json_element_contexts object_contexts   ;

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

        inline void push_chars (iter_type begin, iter_type end)
        {
          current_string.insert (current_string.end (), begin, end);
        }

        inline void push_wchar_t (wchar_t ch)
        {
          current_string.push_back (ch);
        }

        inline string_type const & get_string () noexcept
        {
          return current_string;
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

        bool string_value (string_type const & s)
        {
          auto v = document->create_string (s);

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
    }

    struct json_parser
    {
      // Parses a JSON string into a JSON document 'result' if successful.
      //  'pos' indicates the first non-consumed character (which may lay beyond the last character in the input string)
      static bool parse (doc_string_type const & json, std::size_t & pos, json_document::ptr & result)
      {
        auto begin  = json.c_str ()       ;
        auto end    = begin + json.size ();
        cpp_json::parser::json_parser<details::builder_json_context> jp (begin, end);

        if (jp.try_parse__json ())
        {
          pos     = jp.pos ();
          result  = jp.document;
          return true;
        }
        else
        {
          pos = jp.pos ();
          result.reset ();
          return false;
        }
      }
    };

  }

  std::uint32_t errors = 0;

  std::string to_ascii (std::wstring const & s)
  {
    std::string result;
    result.reserve (s.size ());
    for (auto && wch : s)
    {
      auto ch = static_cast<char> (wch);
      CPP_JSON__ASSERT (ch == wch);
      result.push_back (ch);
    }
    return result;
  }

  template<typename TPredicate>
  long long time_it (std::size_t count, TPredicate predicate)
  {
    predicate ();

    auto then = std::chrono::high_resolution_clock::now ();

    for (auto iter = 0U; iter < count; ++iter)
    {
      predicate ();
    }

    auto now = std::chrono::high_resolution_clock::now ();

    auto diff = now - then;

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (diff);
    return ms.count ();
  }

  template<typename TExpected, typename TActual>
  bool test_eq (
      char const *  file_name
    , int           line_no
    , TExpected &&  expected
    , const char *  sexpected
    , TActual &&    actual
    , const char *  sactual
    )
   {
    CPP_JSON__ASSERT (file_name);
    CPP_JSON__ASSERT (sexpected);
    CPP_JSON__ASSERT (sactual);

    if (expected == actual)
    {
      return true;
    }
    else
    {
      ++errors;
      std::cout
        << file_name
        << "("
        << line_no
        << "): EQ - "
        << sexpected
        << "{"
        << std::forward<TExpected> (expected)
        << "} == "
        << sactual
        << "{"
        << std::forward<TActual> (actual)
        << "}"
        << std::endl
        ;

      return false;
    }
  }

  struct result_json_context
  {
    using string_type         = std::string             ;
    using char_type           = string_type::value_type ;
    using iter_type           = char_type const *       ;
    using stringstream_type   = std::stringstream       ;

    string_type               current_string            ;
    stringstream_type         result                    ;
    std::vector<string_type>  non_printable_chars       ;

    result_json_context ()
    {
      non_printable_chars.reserve (32U);
      for (auto iter = 0U; iter < 32U; ++iter)
      {
        switch (iter)
        {
        case '\b':
          non_printable_chars.push_back ("\\b");
          break;
        case '\f':
          non_printable_chars.push_back ("\\f");
          break;
        case '\n':
          non_printable_chars.push_back ("\\n");
          break;
        case '\r':
          non_printable_chars.push_back ("\\r");
          break;
        case '\t':
          non_printable_chars.push_back ("\\t");
          break;
        default:
          stringstream_type ss;
          ss << "\\u" << std::hex << std::setfill ('0') << std::setw (4) << iter;
          non_printable_chars.push_back (ss.str ());
          break;
        }
      }

    }

    CPP_JSON__NO_COPY_MOVE (result_json_context);

    void write_char (char_type ch)
    {
      if (ch >= 0 && ch < non_printable_chars.size ())
      {
        result << non_printable_chars[ch];
      }
      else
      {
        result << ch;
      }
    }

    void write_string (string_type const & s)
    {
      for (auto && ch : s)
      {
        write_char (ch);
      }
    }

    void expected_char (std::size_t pos, char_type ch) noexcept
    {
      result << "ExpectedChar     : " << pos << ", ";
      write_char (ch);
      result << std::endl;
    }

    void expected_chars (std::size_t pos, string_type const & chs) noexcept
    {
      for (auto && ch : chs)
      {
        expected_char (pos, ch);
      }
    }

    void expected_token (std::size_t pos, string_type const & token) noexcept
    {
      result << "ExpectedToken    : " << pos << ", ";
      write_string (token);
      result << std::endl;
    }

    void unexpected_token (std::size_t pos, string_type const & token) noexcept
    {
      result << "UnexpectedToken  : " << pos << ", ";
      write_string (token);
      result << std::endl;
    }

    void clear_string ()
    {
      current_string.clear ();
    }

    void push_char (char_type ch)
    {
      current_string.push_back (ch);
    }

    void push_chars (iter_type begin, iter_type end)
    {
      current_string.insert (current_string.end (), begin, end);
    }

    void push_wchar_t (wchar_t ch)
    {
      if (ch > 127)
      {
        constexpr auto bsz = 8;
        char buffer[bsz] = {};
        std::snprintf (buffer, bsz, "\\u%04x", ch);
        current_string += buffer;
      }
      else
      {
        current_string.push_back (static_cast<char> (ch));
      }
    }

    string_type const & get_string () noexcept
    {
      return current_string;
    }

    bool array_begin ()
    {
      result << "Array            : begin" << std::endl;
      return true;
    }

    bool array_end ()
    {
      result << "Array            : end" << std::endl;
      return true;
    }

    bool object_begin ()
    {
      result << "Object           : begin" << std::endl;
      return true;
    }

    bool member_key (string_type const & s)
    {
      result << "MemberKey        : ";
      write_string (s);
      result << std::endl;
      return true;
    }

    bool object_end ()
    {
      result << "Object           : end" << std::endl;
      return true;
    }

    bool bool_value (bool b)
    {
      result << "BoolValue        : " << (b ? "true" : "false") << std::endl;
      return true;
    }

    bool null_value ()
    {
      result << "NullValue        : null" << std::endl;
      return true;
    }

    bool string_value (string_type const & s)
    {
      result << "StringValue      : ";
      write_string (s);
      result << std::endl;
      return true;
    }

    bool number_value (double d)
    {
      result
        << "NumberValue      : "
        << std::setprecision(std::numeric_limits<double>::digits10)
        << d
        << std::endl
        ;
      return true;
    }

  };

#ifdef CPP_JSON__FILESYSTEM
  template<typename TChar>
  std::basic_string<TChar> read_file (path const & p)
  {
    std::basic_stringstream<TChar>  content         ;
    std::basic_string<TChar>        line            ;
    std::basic_ifstream<TChar>      input_stream (p);

    while (std::getline (input_stream, line))
    {
      content << line << std::endl;
    }

    return content.str ();
  }

  using visit_test_case = std::function<
    void (
        std::string const & file_name
      , path        const & json_file_path
      , path        const & result_file_path
      )>;

  void visit_all_test_cases (char const * exe, visit_test_case visit)
  {
    CPP_JSON__ASSERT (exe);
    CPP_JSON__ASSERT (visit);

    auto current          = path (exe);

    auto test_cases_path = current
      .parent_path ()
      .parent_path ()
      .parent_path ()
      .parent_path ()
      .append ("test_cases")
      ;

    auto json_path    = test_cases_path;
    json_path.append ("json");

    auto result_path  = test_cases_path;
    result_path.append ("result");

    directory_iterator end;

    for (directory_iterator iter (json_path); iter != end; ++iter)
    {
      auto && entry = *iter;
      if (entry.status ().type () != file_type::regular)
      {
        continue;
      }

      auto json_file_path = entry.path ();

      if (!json_file_path.has_filename ())
      {
        continue;
      }
      auto json_file_name = json_file_path.filename ();


      if (!json_file_path.has_extension ())
      {
        continue;
      }
      auto json_file_ext = json_file_path.extension ().string ();

      if (json_file_ext != ".json")
      {
        continue;
      }

      auto file_name        = json_file_name.string ();
      auto result_file_path = result_path;
      result_file_path.append (file_name + ".result");

      visit (file_name, json_file_path, result_file_path);
    }
  }

  void generate_test_results (char const * exe)
  {
    std::cout << "Running 'generate_test_results'..." << std::endl;

    visit_all_test_cases (
        exe
      , [] (
          std::string const & file_name
        , path        const & json_file_path
        , path        const & result_file_path
        )
      {
        std::cout << "Processing: " << file_name << std::endl;

        using stringstream_type = result_json_context::stringstream_type;
        using string_type       = result_json_context::string_type;

        auto json_document  = read_file<char> (json_file_path);

        auto json_begin     = json_document.c_str ();
        auto json_end       = json_begin + json_document.size ();

        cpp_json::parser::json_parser<result_json_context> jp (json_begin, json_end);

        jp.result
          << "TestCase         : " << file_name << std::endl;

        auto presult  = jp.try_parse__json ();
        auto ppos     = jp.pos ();

        jp.result
          << "ParsePosition    : " << ppos << std::endl
          << "ParseResult      : " << (presult ? "true" : "false") << std::endl
          ;

        std::ofstream result_stream (result_file_path);

        auto result = jp.result.str ();

        result_stream << result;
      });
  }

  void process_test_cases (char const * exe)
  {
    std::cout << "Running 'process_test_cases'..." << std::endl;

    using namespace cpp_json::document;

    visit_all_test_cases (
        exe
      , [] (
          std::string const & file_name
        , path        const & json_file_path
        , path        const & /*result_file_path*/
        )
      {
        std::cout << "Processing: " << file_name << std::endl;

        std::size_t         pos     ;
        json_document::ptr  document;

        auto json_document  = read_file<wchar_t> (json_file_path);

        if (json_document::parse (json_document, pos, document))
        {
          auto serialized = json_document::to_string (document);

          std::size_t         ipos      ;
          json_document::ptr  idocument ;

          if (json_document::parse (serialized, ipos, idocument))
          {
            auto iserialized  = json_document::to_string (idocument);

            if (iserialized != serialized)
            {
              ++errors;
              std::cout
                << "FAILURE: Serialized documents don't match" << std::endl;
            }
          }
          else
          {
            ++errors;
            std::cout
              << "FAILURE: Failed to parse serialized json document" << std::endl;
          }
        }
        else
        {
          // Failure cases are ignored
        }
      });
  }

  void performance_test_cases (char const * exe, int c)
  {
    std::cout << "Running 'performance_test_cases'..." << std::endl;

    using namespace cpp_json::document;

    auto count = c > 0 ? c : 1000;

    visit_all_test_cases (
        exe
      , [count] (
          std::string const & file_name
        , path        const & json_file_path
        , path        const & /*result_file_path*/
        )
      {
        std::size_t       pos   ;
        json_element::ptr result;

        auto json_wdocument  = read_file<wchar_t> (json_file_path);
        if (json_wdocument.size () < 10000U)
        {
          return;
        }

        auto json_adocument  = read_file<char> (json_file_path);


        std::cout << "Processing: " << file_name << std::endl;

        auto time__cpp_json_callback = time_it (count, [&json_wdocument] () { perf__parse_json_callback (json_wdocument); });
        std::cout << "cpp_json_callback: Milliseconds: " << time__cpp_json_callback << std::endl;

        auto time__cpp_json_document = time_it (count, [&json_wdocument] () { perf__parse_json_document (json_wdocument); });
        std::cout << "cpp_json_document: Milliseconds: " << time__cpp_json_document << std::endl;

        //auto time__jsoncpp_document = time_it (count, [&json_adocument] () { perf__jsoncpp_document (json_adocument); });
        //std::cout << "jsoncpp_document: Milliseconds: " << time__jsoncpp_document << std::endl;

      });
  }
#endif

  void manual_test_cases ()
  {
    std::cout << "Running 'manual_test_cases'..." << std::endl;

    using namespace cpp_json::document;

    std::vector<doc_string_type> test_cases =
      {
        LR"([null, 123,-123E100,"Test\tHello", true,false, [true,null],[],{}, {"x":true}])" ,
        LR"(["\b\f\n\r\t\u0010"])"                                                          ,
        LR"(
{
  "x":null,
  "y":3
})"                                                                                         ,
        LR"(["\u004Abc"])"                                                                  ,
        LR"({:null})"                                                                       ,
        LR"([)"                                                                             ,
        LR"({"abc":})"                                                                      ,
        LR"(
{
  "x":null,
  })"                                                                                       ,
      };

    for (auto && test_case : test_cases)
    {
      std::size_t         pos     ;
      json_document::ptr  document;
      doc_string_type     error   ;

      std::wcout << "TEST_CASE: "<< test_case << std::endl;

      if (json_document::parse (test_case, pos, document, error))
      {
        auto json2  = json_document::to_string (document);
        std::wcout
          << L"SUCCESS: Pos: " << pos << L" Json: " << json2 << std::endl;
      }
      else
      {
        std::wcout
          << L"FAILURE: Pos: " << pos << L" Error: " << error << std::endl;
      }
    }
  }

  struct document_test_case
  {
    int         index     ;
    std::size_t size      ;
    bool        at        ;
    bool        get       ;
    bool        names     ;
    bool        is_error  ;
    bool        is_scalar ;
    bool        is_null   ;
    bool        as_bool   ;
    double      as_number ;
    std::string as_string ;
  };

  void document_test_cases ()
  {
    std::wcout << "Running 'document_test_cases'..." << std::endl;

    using namespace cpp_json::document;

    // TODO: Improve DOM testing

    //                                   0    1 2   3    4     5  6    7     8           9  10 11
    doc_string_type json_document = LR"([null,0,125,1.25,"125","",true,false,[true,null],[],{},{"xyz":true,"zyx":null}])";

    std::size_t             pos   ;
    jd::json_document::ptr  doc   ;

    if (jd::json_parser::parse (json_document, pos, doc))
    {
      CPP_JSON__ASSERT (doc);

      auto result = doc->root ();
      CPP_JSON__ASSERT (result);

      std::vector<document_test_case> oracles
      {
        {
          -1            , // index
          12            , // size
          true          , // at
          true          , // get
          true          , // names
          false         , // is_error
          false         , // is_scalar
          false         , // is_null
          false         , // as_bool
          0             , // as_number
          ""            , // as_string
        },
        {
          0             , // index
          0             , // size
          true          , // at
          true          , // get
          true          , // names
          false         , // is_error
          true          , // is_scalar
          true          , // is_null
          false         , // as_bool
          0             , // as_number
          "null"        , // as_string
        },
        {
          1             , // index
          0             , // size
          true          , // at
          true          , // get
          true          , // names
          false         , // is_error
          true          , // is_scalar
          false         , // is_null
          false         , // as_bool
          0             , // as_number
          "0"           , // as_string
        },
        {
          2             , // index
          0             , // size
          true          , // at
          true          , // get
          true          , // names
          false         , // is_error
          true          , // is_scalar
          false         , // is_null
          true          , // as_bool
          125           , // as_number
          "125"         , // as_string
        },
        {
          3             , // index
          0             , // size
          true          , // at
          true          , // get
          true          , // names
          false         , // is_error
          true          , // is_scalar
          false         , // is_null
          true          , // as_bool
          1.25          , // as_number
          "1.25"        , // as_string
        },
        {
          4             , // index
          0             , // size
          true          , // at
          true          , // get
          true          , // names
          false         , // is_error
          true          , // is_scalar
          false         , // is_null
          true          , // as_bool
          125           , // as_number
          "125"         , // as_string
        },
        {
          5             , // index
          0             , // size
          true          , // at
          true          , // get
          true          , // names
          false         , // is_error
          true          , // is_scalar
          false         , // is_null
          false         , // as_bool
          0             , // as_number
          ""            , // as_string
        },
        {
          6             , // index
          0             , // size
          true          , // at
          true          , // get
          true          , // names
          false         , // is_error
          true          , // is_scalar
          false         , // is_null
          true          , // as_bool
          1             , // as_number
          "true"        , // as_string
        },
        {
          7             , // index
          0             , // size
          true          , // at
          true          , // get
          true          , // names
          false         , // is_error
          true          , // is_scalar
          false         , // is_null
          false         , // as_bool
          0             , // as_number
          "false"       , // as_string
        },
        {
          8             , // index
          2             , // size
          true          , // at
          true          , // get
          true          , // names
          false         , // is_error
          false         , // is_scalar
          false         , // is_null
          false         , // as_bool
          0             , // as_number
          ""            , // as_string
        },
        {
          9             , // index
          0             , // size
          true          , // at
          true          , // get
          true          , // names
          false         , // is_error
          false         , // is_scalar
          false         , // is_null
          false         , // as_bool
          0             , // as_number
          ""            , // as_string
        },
        {
          10            , // index
          0             , // size
          true          , // at
          true          , // get
          true          , // names
          false         , // is_error
          false         , // is_scalar
          false         , // is_null
          false         , // as_bool
          0             , // as_number
          ""            , // as_string
        },
        {
          11            , // index
          2             , // size
          true          , // at
          true          , // get
          false         , // names
          false         , // is_error
          false         , // is_scalar
          false         , // is_null
          false         , // as_bool
          0             , // as_number
          ""            , // as_string
        },
      };

      auto size = result->size ();

      for (auto && oracle : oracles)
      {
        auto index = oracle.index;
        auto actual =
            index > -1
          ? result->at (index)
          : result
          ;

        std::cout << "Testing against index: " << index << std::endl;

        TEST_EQ (oracle.size      , actual->size ());
        TEST_EQ (oracle.at        , actual->at  (size)->is_error ());
        TEST_EQ (oracle.get       , actual->get (L"")->is_error ());
        TEST_EQ (oracle.names     , actual->names ().empty ());
        TEST_EQ (oracle.is_error  , actual->is_error ());
        TEST_EQ (oracle.is_scalar , actual->is_scalar ());
        TEST_EQ (oracle.is_null   , actual->is_null ());
        TEST_EQ (oracle.as_bool   , actual->as_bool ());
        TEST_EQ (oracle.as_number , actual->as_number ());
        TEST_EQ (oracle.as_string , to_ascii (actual->as_string ()));
      }

      {
        auto actual     = result->at (8);
        auto true_value = actual->at (0);
        auto null_value = actual->at (1);

        TEST_EQ (2        , actual->size ());
        TEST_EQ (true     , true_value->as_bool ());
        TEST_EQ ("true"   , to_ascii (true_value->as_string ()));
        TEST_EQ (false    , null_value->as_bool ());
        TEST_EQ ("null"   , to_ascii (null_value->as_string ()));
      }

      {
        auto actual     = result->at (11);
        auto true_value = actual->at (0);
        auto null_value = actual->at (1);

        TEST_EQ (2        , actual->size ());
        TEST_EQ (true     , true_value->as_bool ());
        TEST_EQ ("true"   , to_ascii (true_value->as_string ()));
        TEST_EQ (false    , null_value->as_bool ());
        TEST_EQ ("null"   , to_ascii (null_value->as_string ()));
      }

      {
        auto actual     = result->at (11);
        auto names      = actual->names ();
        auto true_value = actual->get (L"xyz");
        auto null_value = actual->get (L"zyx");

        doc_strings_type oracle = {L"xyz", L"zyx"};

        TEST_EQ (2        , actual->size ());
        TEST_EQ (true     , names == oracle);
        TEST_EQ (true     , true_value->as_bool ());
        TEST_EQ ("true"   , to_ascii (true_value->as_string ()));
        TEST_EQ (false    , null_value->as_bool ());
        TEST_EQ ("null"   , to_ascii (null_value->as_string ()));
      }
    }
    else
    {
      ++errors;
      std::cout
        << "FAILURE: Pos: " << pos << std::endl;
    }
  }

}

int main (int argc, char const * * argvs)
{
  try
  {
    std::cout << "Starting test_suite..." << std::endl;

    auto exe = argvs[0];
    CPP_JSON__ASSERT (exe);

    auto count = argc > 1
      ? atoi (argvs[1])
      : 0
      ;

#define CPP_JSON__PERFTEST

#ifndef CPP_JSON__PERFTEST
#ifdef CPP_JSON__FILESYSTEM
    generate_test_results (exe);
    process_test_cases (exe);
#endif
    manual_test_cases ();
    document_test_cases ();
#else
    performance_test_cases (exe, count);
#endif

    if (errors > 0)
    {
      std::cout << errors << " errors detected" << std::endl;
      return 997;
    }
    else
    {
      std::cout << "No errors detected" << std::endl;
      return 0;
    }
  }
  catch (std::exception const & ex)
  {
    std::cout << "EXCEPTION! " << ex.what () << std::endl;

    return 998;
  }
  catch (...)
  {
    std::cout << "EXCEPTION!" << std::endl;

    return 999;
  }
}
