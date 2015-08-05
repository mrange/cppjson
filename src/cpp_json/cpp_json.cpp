#include "stdafx.h"

#include "../cppjson/default_parser.hpp"

#include <iostream>

int main()
{
//  std::string json = R"([null, 123,-1.23E2,"Test\tHello", true,false, [true,null],[],{}, {"x":true}])";
//  std::string json = R"({:null})";
  std::string json = R"([)";

  std::size_t                         pos   ;
  default_cpp_json::json_element::ptr result;
  std::string                         error ;

  if (default_cpp_json::parse (json, pos, result, error))
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

