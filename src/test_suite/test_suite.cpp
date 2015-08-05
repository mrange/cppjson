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

#include <iostream>

int main()
{
  using namespace cpp_json::standard;

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

  return 0;
}

