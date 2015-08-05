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
#include <iostream>
#include <fstream>
#include <sstream>

int main (int argc, char const * * argvs)
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
      if (!json_file_path.has_extension ())
      {
        continue;
      }

      auto json_file_ext = json_file_path.extension ().string ();
      if (json_file_ext != ".json")
      {
        continue;
      }

      std::cout << "Processing: " << json_file_path << std::endl;

      stringstream_type               json;
      std::basic_ifstream<char_type>  json_stream (json_file_path);
      string_type                     json_line;

      while (std::getline (json_stream, json_line))
      {
        json << json_line << std::endl;
      }

      auto json_document = json.str ();

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