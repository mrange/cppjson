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

#include "../cpp_json/cpp_json__standard.hpp"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>

namespace
{
  struct result_json_context
  {
    using string_type         = std::wstring            ;
    using char_type           = string_type::value_type ;
    using iter_type           = char_type const *       ;
    using stringstream_type   = std::wstringstream      ;

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
          non_printable_chars.push_back (L"\\b");
          break;
        case '\f':
          non_printable_chars.push_back (L"\\f");
          break;
        case '\n':
          non_printable_chars.push_back (L"\\n");
          break;
        case '\r':
          non_printable_chars.push_back (L"\\r");
          break;
        case '\t':
          non_printable_chars.push_back (L"\\t");
          break;
        default:
          stringstream_type ss;
          ss << "\\u" << std::hex << std::setfill (L'0') << std::setw (4) << iter;
          non_printable_chars.push_back (ss.str ());
          break;
        }
      }

    }

    CPP_JSON__NO_COPY_MOVE (result_json_context);

    void write_char (char_type ch)
    {
      switch (ch)
      {
      case '\"':
        result << L"\\\"";
        break;
      case '\\':
        result << L"\\\\";
        break;
      case '/':
        result << L"\\/";
        break;
      default:
        if (ch >= 0 && ch < non_printable_chars.size ())
        {
          result << non_printable_chars[ch];
        }
        else
        {
          result << ch;
        }
      }
    }

    void write_string (string_type const & s)
    {
      for (auto && ch : s)
      {
        write_char (ch);
      }
    }

    inline void expected_char (std::size_t pos, char_type ch) throw ()
    {
      result << L"ExpectedChar     : " << pos << L", ";
      write_char (ch);
      result << std::endl;
    }

    inline void expected_chars (std::size_t pos, string_type const & chs) throw ()
    {
      for (auto && ch : chs)
      {
        expected_char (pos, ch);
      }
    }

    inline void expected_token (std::size_t pos, string_type const & token) throw ()
    {
      // TODO: Escape
      result << L"ExpectedToken    : " << pos << L", ";
      write_string (token);
      result << std::endl;
    }

    inline void unexpected_token (std::size_t pos, string_type const & token) throw ()
    {
      result << L"UnexpectedToken  : " << pos << L", ";
      write_string (token);
      result << std::endl;
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

    bool array_begin ()
    {
      result << L"Array            : begin" << std::endl;
      return true;
    }

    bool array_end ()
    {
      result << L"Array            : end" << std::endl;
      return true;
    }

    bool object_begin ()
    {
      result << L"Object           : begin" << std::endl;
      return true;
    }

    bool member_key (string_type const & s)
    {
      result << L"MemberKey        : ";
      write_string (s);
      result << std::endl;
      return true;
    }

    bool object_end ()
    {
      result << L"Object           : end" << std::endl;
      return true;
    }

    bool bool_value (bool b)
    {
      result << L"BoolValue        : " << (b ? L"true" : L"false") << std::endl;
      return true;
    }

    bool null_value ()
    {
      result << L"NullValue        : null" << std::endl;
      return true;
    }

    bool string_value (string_type const & s)
    {
      result << L"StringValue      : ";
      write_string (s);
      result << std::endl;
      return true;
    }

    bool number_value (double d)
    {
      result << L"NumberValue      : " << d << std::endl;
      return true;
    }

  };
}

int main (int /*argc*/, char const * * argvs)
{
  using namespace cpp_json::standard;
  using namespace std::tr2::sys     ;

  try
  {
    auto current          = path (argvs[0]);

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

      auto result_file_path = result_path;
      result_file_path.append (json_file_name.string () + ".result");

      std::cout << "Processing: " << json_file_name << std::endl;

      stringstream_type               json;
      std::basic_ifstream<char_type>  json_stream (json_file_path);
      string_type                     json_line;

      while (std::getline (json_stream, json_line))
      {
        json << json_line << std::endl;
      }

      auto json_document  = json.str ();
      auto json_begin     = json_document.c_str ();
      auto json_end       = json_begin + json_document.size ();

      cpp_json::parser::json_parser<result_json_context> jp (json_begin, json_end);

      jp.result
        << L"TestCase         : " << json_file_name.string () << std::endl;

      auto presult  = jp.try_parse__json ();
      auto ppos     = jp.pos ();

      jp.result
        << L"ParsePosition    : " << ppos << std::endl
        << L"ParseResult      : " << (presult ? L"true" : L"false") << std::endl;

      std::basic_ofstream<char_type>  result_stream (result_file_path);

      result_stream << jp.result.str ();
    }

    std::cout << "DONE!" << std::endl;

    return 0;
  }
  catch (...)
  {
    std::cout << "EXCEPTION!" << std::endl;
  }
}


#ifdef DDD

  string_type json = LR"([null, 123,-1.23E2,"Test\tHello", true,false, [true,null],[],{}, {"x":true}])";
//  string_type json = LR"({:null})";
//  string_type json = LR"([)";
//  string_type json = LR"(["\u004Abc"])";

  std::size_t       pos   ;
  json_element::ptr result;
  string_type       error ;

  if (parse (json, pos, result, error))
  {
    auto json2 = to_string (result);
    std::wcout
      << L"SUCCESS: Pos: " << pos << L" Json: " << json2 << std::endl;
  }
  else
  {
    std::wcout
      << L"FAILURE: Pos: " << pos << L" Error: " << error << std::endl;
  }


#endif
