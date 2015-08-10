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

#include <type_traits>

namespace cpp_json
{
namespace document
{

  enum class json_value_type : std::uint8_t
  {
    vt__empty_value,
    vt__error_value,
    vt__null_value,
    vt__bool_value,
    vt__number_value,
    vt__string_value,
    vt__array_value,
    vt__object_value,
  };

  struct json_value
  {
    json_value () noexcept
      : vt (json_value_type::vt__empty_value)
    {
    }

    ~json_value () noexcept
    {
      switch (vt)
      {
      default:
      case json_value_type::vt__empty_value:
        break;
      case json_value_type::vt__error_value:
        break;
      case json_value_type::vt__null_value:
        break;
      case json_value_type::vt__bool_value:
        destroy (v__bool_value);
        break;
      case json_value_type::vt__number_value:
        destroy (v__number_value);
        break;
      case json_value_type::vt__string_value:
        destroy (v__string_value);
        break;
      case json_value_type::vt__array_value:
        destroy (v__array_value);
        break;
      case json_value_type::vt__object_value:
        destroy (v__object_value);
        break;
      }
      vt = json_value_type::vt__empty_value;
    }

    json_value (json_value && v) noexcept
      : vt (v.vt)
    {
      switch (vt)
      {
      default:
      case json_value_type::vt__empty_value:
        break;
      case json_value_type::vt__error_value:
        break;
      case json_value_type::vt__null_value:
        break;
      case json_value_type::vt__bool_value:
        new (&v__bool_value) bool (std::move (v.v__bool_value));
        break;
      case json_value_type::vt__number_value:
        new (&v__number_value) double (std::move (v.v__number_value));
        break;
      case json_value_type::vt__string_value:
        new (&v__string_value) std::wstring (std::move (v.v__string_value));
        break;
      case json_value_type::vt__array_value:
        new (&v__array_value) std::vector<json_value> (std::move (v.v__array_value));
        break;
      case json_value_type::vt__object_value:
        new (&v__object_value) std::vector<std::tuple<std::wstring, json_value>> (std::move (v.v__object_value));
        break;
      }
      v.vt = json_value_type::vt__empty_value;
    }

    json_value (json_value const & v)
      : vt (v.vt)
    {
      switch (vt)
      {
      default:
      case json_value_type::vt__empty_value:
        break;
      case json_value_type::vt__error_value:
        break;
      case json_value_type::vt__null_value:
        break;
      case json_value_type::vt__bool_value:
        new (&v__bool_value) bool (v.v__bool_value);
        break;
      case json_value_type::vt__number_value:
        new (&v__number_value) double (v.v__number_value);
        break;
      case json_value_type::vt__string_value:
        new (&v__string_value) std::wstring (v.v__string_value);
        break;
      case json_value_type::vt__array_value:
        new (&v__array_value) std::vector<json_value> (v.v__array_value);
        break;
      case json_value_type::vt__object_value:
        new (&v__object_value) std::vector<std::tuple<std::wstring, json_value>> (v.v__object_value);
        break;
      }
    }

    json_value & operator= (json_value && v) noexcept
    {
      if (this == &v)
      {
        return *this;
      }

      destroy (*this);

      switch (vt)
      {
      default:
      case json_value_type::vt__empty_value:
        break;
      case json_value_type::vt__error_value:
        break;
      case json_value_type::vt__null_value:
        break;
      case json_value_type::vt__bool_value:
        new (&v__bool_value) bool (std::move (v.v__bool_value));
        break;
      case json_value_type::vt__number_value:
        new (&v__number_value) double (std::move (v.v__number_value));
        break;
      case json_value_type::vt__string_value:
        new (&v__string_value) std::wstring (std::move (v.v__string_value));
        break;
      case json_value_type::vt__array_value:
        new (&v__array_value) std::vector<json_value> (std::move (v.v__array_value));
        break;
      case json_value_type::vt__object_value:
        new (&v__object_value) std::vector<std::tuple<std::wstring, json_value>> (std::move (v.v__object_value));
        break;
      }

      vt = v.vt;
      v.vt = json_value_type::vt__empty_value;
    }

    json_value & operator= (json_value const & v)
    {
      if (this == &v)
      {
        return *this;
      }

      json_value copy (v);

      return *this = std::move (copy);
    }

    void swap (json_value & v) noexcept
    {
      std::swap (*this, v);
    }

    json_value_type get__type () const noexcept
    {
      return vt;
    }

    bool is__empty_value () const
    {
      return vt == json_value_type::vt__empty_value;
    }

    static json_value empty_value ()
    {
      return json_value ();
    }

    bool is__error_value () const
    {
      return vt == json_value_type::vt__error_value;
    }

    static json_value error_value ()
    {
      json_value u;
      u.vt = json_value_type::vt__error_value;
      return u;
    }

    bool is__null_value () const
    {
      return vt == json_value_type::vt__null_value;
    }

    static json_value null_value ()
    {
      json_value u;
      u.vt = json_value_type::vt__null_value;
      return u;
    }

    bool is__bool_value () const
    {
      return vt == json_value_type::vt__bool_value;
    }

    static json_value bool_value (bool && v)
    {
      json_value u;
      u.vt = json_value_type::vt__bool_value;
      new (&u.v__bool_value) bool (std::move (v));
      return u;
    }

    static json_value bool_value (bool const & v)
    {
      json_value u;
      u.vt = json_value_type::vt__bool_value;
      new (&u.v__bool_value) bool (v);
      return u;
    }

    bool get__bool_value (bool & v) const
    {
      if (vt == json_value_type::vt__bool_value)
      {
        v = v__bool_value;

        return true;
      }
      else
      {
        return false;
      }
    }

    void set__bool_value (bool && v) noexcept
    {
      *this = json_value::bool_value (std::move (v));
    }

    void set__bool_value (bool const & v)
    {
      *this = json_value::bool_value (v);
    }
    bool is__number_value () const
    {
      return vt == json_value_type::vt__number_value;
    }

    static json_value number_value (double && v)
    {
      json_value u;
      u.vt = json_value_type::vt__number_value;
      new (&u.v__number_value) double (std::move (v));
      return u;
    }

    static json_value number_value (double const & v)
    {
      json_value u;
      u.vt = json_value_type::vt__number_value;
      new (&u.v__number_value) double (v);
      return u;
    }

    bool get__number_value (double & v) const
    {
      if (vt == json_value_type::vt__number_value)
      {
        v = v__number_value;

        return true;
      }
      else
      {
        return false;
      }
    }

    void set__number_value (double && v) noexcept
    {
      *this = json_value::number_value (std::move (v));
    }

    void set__number_value (double const & v)
    {
      *this = json_value::number_value (v);
    }
    bool is__string_value () const
    {
      return vt == json_value_type::vt__string_value;
    }

    static json_value string_value (std::wstring && v)
    {
      json_value u;
      u.vt = json_value_type::vt__string_value;
      new (&u.v__string_value) std::wstring (std::move (v));
      return u;
    }

    static json_value string_value (std::wstring const & v)
    {
      json_value u;
      u.vt = json_value_type::vt__string_value;
      new (&u.v__string_value) std::wstring (v);
      return u;
    }

    bool get__string_value (std::wstring & v) const
    {
      if (vt == json_value_type::vt__string_value)
      {
        v = v__string_value;

        return true;
      }
      else
      {
        return false;
      }
    }

    void set__string_value (std::wstring && v) noexcept
    {
      *this = json_value::string_value (std::move (v));
    }

    void set__string_value (std::wstring const & v)
    {
      *this = json_value::string_value (v);
    }
    bool is__array_value () const
    {
      return vt == json_value_type::vt__array_value;
    }

    static json_value array_value (std::vector<json_value> && v)
    {
      json_value u;
      u.vt = json_value_type::vt__array_value;
      new (&u.v__array_value) std::vector<json_value> (std::move (v));
      return u;
    }

    static json_value array_value (std::vector<json_value> const & v)
    {
      json_value u;
      u.vt = json_value_type::vt__array_value;
      new (&u.v__array_value) std::vector<json_value> (v);
      return u;
    }

    bool get__array_value (std::vector<json_value> & v) const
    {
      if (vt == json_value_type::vt__array_value)
      {
        v = v__array_value;

        return true;
      }
      else
      {
        return false;
      }
    }

    void set__array_value (std::vector<json_value> && v) noexcept
    {
      *this = json_value::array_value (std::move (v));
    }

    void set__array_value (std::vector<json_value> const & v)
    {
      *this = json_value::array_value (v);
    }
    bool is__object_value () const
    {
      return vt == json_value_type::vt__object_value;
    }

    static json_value object_value (std::vector<std::tuple<std::wstring, json_value>> && v)
    {
      json_value u;
      u.vt = json_value_type::vt__object_value;
      new (&u.v__object_value) std::vector<std::tuple<std::wstring, json_value>> (std::move (v));
      return u;
    }

    static json_value object_value (std::vector<std::tuple<std::wstring, json_value>> const & v)
    {
      json_value u;
      u.vt = json_value_type::vt__object_value;
      new (&u.v__object_value) std::vector<std::tuple<std::wstring, json_value>> (v);
      return u;
    }

    bool get__object_value (std::vector<std::tuple<std::wstring, json_value>> & v) const
    {
      if (vt == json_value_type::vt__object_value)
      {
        v = v__object_value;

        return true;
      }
      else
      {
        return false;
      }
    }

    void set__object_value (std::vector<std::tuple<std::wstring, json_value>> && v) noexcept
    {
      *this = json_value::object_value (std::move (v));
    }

    void set__object_value (std::vector<std::tuple<std::wstring, json_value>> const & v)
    {
      *this = json_value::object_value (v);
    }
  private:
    template<typename T>
    static void destroy (T & v) noexcept
    {
      v.~T ();
    }

    union
    {
      bool v__bool_value;
      double v__number_value;
      std::wstring v__string_value;
      std::vector<json_value> v__array_value;
      std::vector<std::tuple<std::wstring, json_value>> v__object_value;
    };

    json_value_type vt;
  };

} // document
} // cpp_json

