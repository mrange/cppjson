#include "stdafx.h"

#include "../cppjson/cpp_json__standard.hpp"

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

