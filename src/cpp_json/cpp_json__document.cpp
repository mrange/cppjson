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

#include "cpp_json__document.hpp"

namespace cpp_json { namespace document
{
  namespace details
  {
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
  }

  bool json_document::parse (doc_string_type const & json, std::size_t & pos, json_document::ptr & result)
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

  bool json_document::parse (doc_string_type const & json, std::size_t & pos, json_document::ptr & result, doc_string_type & error)
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

      cpp_json::parser::json_parser<details::error_json_context> ejp (begin, end);
      ejp.error_pos = jp.pos ();
      auto eresult  = ejp.try_parse__json ();
      CPP_JSON__ASSERT (!eresult);

      auto sz = ejp.exp_chars.size () + ejp.exp_tokens.size ();

      std::vector<doc_string_type> expected   = std::move (ejp.exp_tokens);
      std::vector<doc_string_type> unexpected = std::move (ejp.unexp_tokens);

      expected.reserve (sz);

      for (auto ch : ejp.exp_chars)
      {
        details::error_json_context::char_type token[] = {'\'', ch, '\'', 0};
        expected.push_back (doc_string_type (token));
      }

      std::sort (expected.begin (), expected.end ());
      std::sort (unexpected.begin (), unexpected.end ());

      expected.erase (std::unique (expected.begin (), expected.end ()), expected.end ());
      unexpected.erase (std::unique (unexpected.begin (), unexpected.end ()), unexpected.end ());

      doc_string_type msg;

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

      auto append = [&msg, &newline] (wchar_t const * prepend, std::vector<doc_string_type> const & vs)
      {
        CPP_JSON__ASSERT (prepend);

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

  doc_string_type json_document::to_string (json_document::ptr const & json)
  {
    if (json)
    {
      details::json_element_visitor__to_string visitor;

      json->root ()->apply (visitor);

      return std::move (visitor.value);
    }
    else
    {
      return L"[]";
    }
  }
} }
