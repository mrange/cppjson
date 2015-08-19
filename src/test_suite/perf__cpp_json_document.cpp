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
#include "../cpp_json/cpp_json__parser__sse2.hpp"

void perf__parse_json_document (std::wstring const & json_document)
{
  using namespace cpp_json::document;

  std::size_t         pos     ;
  json_document::ptr  document;

  auto presult = json_document::parse (json_document, pos, document);

  CPP_JSON__ASSERT (presult);
}
