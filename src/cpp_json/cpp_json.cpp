#include "stdafx.h"

#include "../cppjson/cpp_json__standard.hpp"

#include <iostream>

int main()
{
  using namespace cpp_json::standard;

//  std::string json = R"([null, 123,-1.23E2,"Test\tHello", true,false, [true,null],[],{}, {"x":true}])";
//  std::string json = R"({:null})";
//  std::string json = R"([)";
  std::string json = R"(["\u0041bc"])";

  std::size_t       pos   ;
  json_element::ptr result;
  std::string       error ;

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

