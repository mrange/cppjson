# ----------------------------------------------------------------------------------------------
# Copyright 2015 Mårten Rånge
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ----------------------------------------------------------------------------------------------

require 'carnelian/executor'

$namespace  = ["cpp_json", "document"]
$unions     =
  [
    {
      name:   "json_value"                                                      ,
      variants:
        [
          {
            type:           "unit"                                              ,
            name:           "error_value"                                       ,
          }                                                                     ,
          {
            type:           "unit"                                              ,
            name:           "null_value"                                        ,
          }                                                                     ,
          {
            type:           "bool"                                              ,
            name:           "bool_value"                                        ,
          }                                                                     ,
          {
            type:           "double"                                            ,
            name:           "number_value"                                      ,
          }                                                                     ,
          {
            type:           "std::wstring"                                      ,
            name:           "string_value"                                      ,
          }                                                                     ,
          {
            type:           "std::vector<json_value>"                           ,
            name:           "array_value"                                       ,
          }                                                                     ,
          {
            type:           "std::vector<std::tuple<std::wstring, json_value>>" ,
            name:           "object_value"                                      ,
          }                                                                     ,
        ]                                                                       ,
    }                                                                           ,
  ]

CarnelianExecutor.build_metaprogram_to_file   "cpp_union.mp", "cpp_union.hpp.rb"
CarnelianExecutor.execute_metaprogram_to_file "cpp_union.mp", "../cpp_json/cpp_json__unions.hpp"
