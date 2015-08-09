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

#include "../cpp_json/cpp_json__document.hpp"

#include <codecvt>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>

#if _MSC_VER >= 1900
# define CPP_JSON__FILESYSTEM // VS 2015 supports a preliminary version of filesystem
#endif

#ifdef CPP_JSON__FILESYSTEM
#include <filesystem>

using namespace std::tr2::sys;
#endif

#define TEST_EQ(expected, actual) test_eq (__FILE__, __LINE__, expected, #expected, actual, #actual)

namespace
{
  std::uint32_t errors = 0;
  std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_converter;

  std::string to_utf8 (std::wstring const & s)
  {
    return utf8_converter.to_bytes(s);
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

        auto json_document  = read_file<string_type::value_type> (json_file_path);

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

        std::size_t       pos   ;
        json_element::ptr result;

        auto json_document  = read_file<string_type::value_type> (json_file_path);

        if (json_document::parse (json_document, pos, result))
        {
          auto serialized = json_document::to_string (result);

          std::size_t       ipos    ;
          json_element::ptr iresult ;

          if (json_document::parse (serialized, ipos, iresult))
          {
            auto iserialized = json_document::to_string (iresult);

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
#endif

  void manual_test_cases ()
  {
    std::cout << "Running 'manual_test_cases'..." << std::endl;

    using namespace cpp_json::document;

    std::vector<string_type> test_cases =
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
      std::size_t       pos   ;
      json_element::ptr result;
      string_type       error ;

      std::wcout << "TEST_CASE: "<< test_case << std::endl;

      if (json_document::parse (test_case, pos, result, error))
      {
        auto json2 = json_document::to_string (result);
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

    //                               0    1 2   3    4     5  6    7     8           9  10 11
    string_type json_document = LR"([null,0,125,1.25,"125","",true,false,[true,null],[],{},{"xyz":true,"zyx":null}])";

    std::size_t       pos   ;
    json_element::ptr result;

    if (json_document::parse (json_document, pos, result))
    {
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
        TEST_EQ (oracle.as_string , to_utf8 (actual->as_string ()));
      }

      {
        auto actual     = result->at (8);
        auto true_value = actual->at (0);
        auto null_value = actual->at (1);

        TEST_EQ (2        , actual->size ());
        TEST_EQ (true     , true_value->as_bool ());
        TEST_EQ ("true"   , to_utf8 (true_value->as_string ()));
        TEST_EQ (false    , null_value->as_bool ());
        TEST_EQ ("null"   , to_utf8 (null_value->as_string ()));
      }

      {
        auto actual     = result->at (11);
        auto true_value = actual->at (0);
        auto null_value = actual->at (1);

        TEST_EQ (2        , actual->size ());
        TEST_EQ (true     , true_value->as_bool ());
        TEST_EQ ("true"   , to_utf8 (true_value->as_string ()));
        TEST_EQ (false    , null_value->as_bool ());
        TEST_EQ ("null"   , to_utf8 (null_value->as_string ()));
      }

      {
        auto actual     = result->at (11);
        auto names      = actual->names ();
        auto true_value = actual->get (L"xyz");
        auto null_value = actual->get (L"zyx");

        strings_type oracle = {L"xyz", L"zyx"};

        TEST_EQ (2        , actual->size ());
        TEST_EQ (true     , names == oracle);
        TEST_EQ (true     , true_value->as_bool ());
        TEST_EQ ("true"   , to_utf8 (true_value->as_string ()));
        TEST_EQ (false    , null_value->as_bool ());
        TEST_EQ ("null"   , to_utf8 (null_value->as_string ()));
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

int main (int /*argc*/, char const * * argvs)
{
  try
  {
    std::cout << "Starting test_suite..." << std::endl;

    auto exe = argvs[0];
    CPP_JSON__ASSERT (exe);

#ifdef CPP_JSON__FILESYSTEM
    generate_test_results (exe);
    process_test_cases (exe);
#endif
    manual_test_cases ();
    document_test_cases ();

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
