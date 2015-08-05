#include "stdafx.h"

#include "../cppjson/cpp_json__standard.hpp"

#include <iostream>

int main()
{
  using namespace cpp_json::standard;

//  string_type json = R"([null, 123,-1.23E2,"Test\tHello", true,false, [true,null],[],{}, {"x":true}])";
//  string_type json = R"({:null})";
//  string_type json = R"([)";
  string_type json = R"(["\u0041bc"])";

  std::size_t       pos   ;
  json_element::ptr result;
  string_type       error ;

  if (parse (json, pos, result, error))
  {
    std::cout 
      << "SUCCESS: Pos: " << pos << std::endl;
  }
  else
  {
    std::cout 
      << "FAILURE: Pos: " << pos << " Error: " << error << std::endl;
  }

  return 0;
}

