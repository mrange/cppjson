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

#include "cpp_json__value_g.hpp"

namespace cpp_json
{
namespace document
{
  struct default_values
  {
    static default_values const & get () 
    {
      static default_values v;
      return v;
    }

    bool                                              const bool_value    ;
    double                                            const number_value  ;
    std::wstring                                      const string_value  ;
    std::vector<json_value>                           const array_value   ;
    std::vector<std::tuple<std::wstring, json_value>> const object_value  ;

    json_value                                        const error_value   ;

    std::wstring                                      const empty_value_s ;
    std::wstring                                      const error_value_s ;
    std::wstring                                      const true_value_s  ;
    std::wstring                                      const false_value_s ;

    default_values ()
      : bool_value    (false                      )
      , number_value  (0.0                        )
      , error_value   (json_value::error_value () )
      , empty_value_s (L"null"                    )
      , error_value_s (L"error"                   )
      , true_value_s  (L"true"                    )
      , false_value_s (L"false"                   )
    {
    }

  };

  std::size_t json_value::size () const
  {
    switch (get__type ())
    {
    default:
    case json_value_type::vt__empty_value:
    case json_value_type::vt__error_value:
    case json_value_type::vt__bool_value:
    case json_value_type::vt__number_value:
    case json_value_type::vt__string_value:
      return 0;
    case json_value_type::vt__array_value:
      return get__array_value (default_values::get ().array_value).size ();
    case json_value_type::vt__object_value:
      return get__object_value (default_values::get ().object_value).size ();
    }
  }

  json_value const & json_value::at (std::size_t idx) const
  {
    switch (get__type ())
    {
    default:
    case json_value_type::vt__empty_value:
    case json_value_type::vt__error_value:
    case json_value_type::vt__bool_value:
    case json_value_type::vt__number_value:
    case json_value_type::vt__string_value:
      return default_values::get ().error_value;
    case json_value_type::vt__array_value:
      {
        auto && vs = get__array_value (default_values::get ().array_value);
        if (idx < vs.size ())
        {
          return vs[idx];
        }
        else
        {
          return default_values::get ().error_value;
        }
      }
    case json_value_type::vt__object_value:
      {
        auto && vs = get__object_value (default_values::get ().object_value);
        if (idx < vs.size ())
        {
          return std::get<1> (vs[idx]);
        }
        else
        {
          return default_values::get ().error_value;
        }
      }
    }
  }

/*
  json_value & json_value::at (std::size_t idx)
  {
    switch (get__type ())
    {
    default:
    case json_value_type::vt__empty_value:
    case json_value_type::vt__error_value:
    case json_value_type::vt__bool_value:
    case json_value_type::vt__number_value:
    case json_value_type::vt__string_value:
      return default_values::get ().error_value;
    case json_value_type::vt__array_value:
      {
        auto && vs = get__array_value (default_values::get ().array_value);
        if (idx < vs.size ())
        {
          return vs[idx];
        }
        else
        {
          return default_values::get ().error_value;
        }
      }
    case json_value_type::vt__object_value:
      {
        auto && vs = get__object_value (default_values::get ().object_value);
        if (idx < vs.size ())
        {
          return std::get<1> (vs[idx]);
        }
        else
        {
          return default_values::get ().error_value;
        }
      }
    }
  }
*/
  json_value const & json_value::get (std::wstring const & name) const
  {
    switch (get__type ())
    {
    default:
    case json_value_type::vt__empty_value:
    case json_value_type::vt__error_value:
    case json_value_type::vt__bool_value:
    case json_value_type::vt__number_value:
    case json_value_type::vt__string_value:
    case json_value_type::vt__array_value:
      return default_values::get ().error_value;
    case json_value_type::vt__object_value:
      {
        auto && vs = get__object_value (default_values::get ().object_value);
        for (auto && v : vs)
        {
          if (std::get<0> (v) == name)
          {
            return std::get<1> (v);
          }
        }

        return default_values::get ().error_value;
      }
    }
  }
/*
  json_value & json_value::get (std::wstring const & name)
  {
    switch (get__type ())
    {
    default:
    case json_value_type::vt__empty_value:
    case json_value_type::vt__error_value:
    case json_value_type::vt__bool_value:
    case json_value_type::vt__number_value:
    case json_value_type::vt__string_value:
    case json_value_type::vt__array_value:
      return default_values::get ().error_value;
    case json_value_type::vt__object_value:
      {
        auto && vs = get__object_value (default_values::get ().object_value);
        for (auto && v : vs)
        {
          if (std::get<0> (v) == name)
          {
            return std::get<1> (v);
          }
        }

        return default_values::get ().error_value;
      }
    }
  }
*/

  std::vector<std::wstring> json_value::names () const
  {
    std::vector<std::wstring> result;

    auto && vs = get__object_value (default_values::get ().object_value);

    result.reserve (vs.size ());
    for (auto && v : vs)
    {
      result.push_back (std::get<0> (v));
    }

    return result;
  }

  bool json_value::is_scalar () const
  {
    switch (get__type ())
    {
    default:
    case json_value_type::vt__empty_value:
    case json_value_type::vt__error_value:
    case json_value_type::vt__bool_value:
    case json_value_type::vt__number_value:
    case json_value_type::vt__string_value:
      return true;
    case json_value_type::vt__array_value:
    case json_value_type::vt__object_value:
      return false;
    }
  }

  bool json_value::as_bool () const
  {
    switch (get__type ())
    {
    default:
    case json_value_type::vt__empty_value:
    case json_value_type::vt__error_value:
      return false;
    case json_value_type::vt__bool_value:
      return get__bool_value (false);
    case json_value_type::vt__number_value:
      return get__number_value (0.0) != 0.0;
    case json_value_type::vt__string_value:
      return !get__string_value (default_values::get ().string_value).empty ();
    case json_value_type::vt__array_value:
    case json_value_type::vt__object_value:
      return false; // TODO:
    }
  }

  double json_value::as_number () const
  {
    switch (get__type ())
    {
    default:
    case json_value_type::vt__empty_value:
    case json_value_type::vt__error_value:
      return 0.0;
    case json_value_type::vt__bool_value:
      return get__bool_value (false) ? 1.0 : 0.0;
    case json_value_type::vt__number_value:
      return get__number_value (0.0);
    case json_value_type::vt__string_value:
      {
        auto && s  = get__string_value (default_values::get ().string_value);
        wchar_t * e = nullptr;
        return std::wcstof (s.c_str (), &e);
      }
    case json_value_type::vt__array_value:
    case json_value_type::vt__object_value:
      return 0.0; // TODO:
    }
  }

  std::wstring json_value::as_string () const
  {
    switch (get__type ())
    {
    default:
    case json_value_type::vt__empty_value:
      return default_values::get ().empty_value_s;
    case json_value_type::vt__error_value:
      return default_values::get ().error_value_s;
    case json_value_type::vt__bool_value:
      return 
          get__bool_value (false) 
        ? default_values::get ().true_value_s
        : default_values::get ().false_value_s
        ;
    case json_value_type::vt__number_value:
      return default_values::get ().empty_value_s;
    case json_value_type::vt__string_value:
      return get__string_value (default_values::get ().string_value);
    case json_value_type::vt__array_value:
    case json_value_type::vt__object_value:
      return default_values::get ().string_value; // TODO:
    }
  }


} // document
} // cpp_json

