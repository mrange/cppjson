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

#pragma once

#include <emmintrin.h>

#include "cpp_json__parser.hpp"

namespace cpp_json { namespace parser
{
  namespace details
  {
    template<>
    struct find_special_char<wchar_t const *>
    {
      using iter_type = wchar_t const *;

      static iter_type find (iter_type begin, iter_type end) 
      {
        return begin;
      }
    };

  }

} }
