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

namespace MyNamespace
{

  struct json_value
  {
    json_value () noexcept
      vt (value_type::empty_value)
    {
    }

    ~json_value () noexcept
    {
      switch (vt)
      {
      default:
      case value_type::empty_value:
        break;
      case value_type::error_value:
        break;
      case value_type::null_value:
        break;
      case value_type::false_value:
        break;
      case value_type::true_value:
        break;
      case value_type::number_value:
        destroy (number_value);
        break;
      case value_type::string_value:
        destroy (string_value);
        break;
      case value_type::array_value:
        destroy (array_value);
        break;
      case value_type::object_value:
        destroy (object_value);
        break;
      }
      vt = value_type::empty_value;
    }

    json_value (json_value && v) noexcept
      : vt (v.vt)
    {
      switch (vt)
      {
      default:
      case value_type::empty_value:
        break;
      case value_type::error_value:
        break;
      case value_type::null_value:
        break;
      case value_type::false_value:
        break;
      case value_type::true_value:
        break;
      case value_type::number_value:
        new (&number_value) double (std::move (v.number_value));
        break;
      case value_type::string_value:
        new (&string_value) std::wstring (std::move (v.string_value));
        break;
      case value_type::array_value:
        new (&array_value) std::vector<json_value> (std::move (v.array_value));
        break;
      case value_type::object_value:
        new (&object_value) std::vector<std::tuple<std::wstring, json_value>> (std::move (v.object_value));
        break;
      }
      v.vt = value_type::empty_value;
    }

    json_value (json_value const & v)
      : vt (v.vt)
    {
      switch (vt)
      {
      default:
      case value_type::empty_value:
        break;
      case value_type::error_value:
        break;
      case value_type::null_value:
        break;
      case value_type::false_value:
        break;
      case value_type::true_value:
        break;
      case value_type::number_value:
        new (&number_value) double (v.number_value);
        break;
      case value_type::string_value:
        new (&string_value) std::wstring (v.string_value);
        break;
      case value_type::array_value:
        new (&array_value) std::vector<json_value> (v.array_value);
        break;
      case value_type::object_value:
        new (&object_value) std::vector<std::tuple<std::wstring, json_value>> (v.object_value);
        break;
      }
    }

    json_value & operator= (json_value && v) noexcept
    {
      if (this == &v)
      {
        return *this;
      }

      ~json_value ();

      switch (vt)
      {
      default:
      case value_type::empty_value:
        break;
      case value_type::error_value:
        break;
      case value_type::null_value:
        break;
      case value_type::false_value:
        break;
      case value_type::true_value:
        break;
      case value_type::number_value:
        new (&number_value) double (std::move (v.number_value));
        break;
      case value_type::string_value:
        new (&string_value) std::wstring (std::move (v.string_value));
        break;
      case value_type::array_value:
        new (&array_value) std::vector<json_value> (std::move (v.array_value));
        break;
      case value_type::object_value:
        new (&object_value) std::vector<std::tuple<std::wstring, json_value>> (std::move (v.object_value));
        break;
      }

      vt = v.vt;
      v.vt = value_type::empty_value;
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

    static json_value empty_value ()
    {
      return json_value ();
    }

    inline static json_value error_value ()
    {
      json_value u;
      u.vt = value_type::error_value;
      return u;
    }

    inline static json_value null_value ()
    {
      json_value u;
      u.vt = value_type::null_value;
      return u;
    }

    inline static json_value false_value ()
    {
      json_value u;
      u.vt = value_type::false_value;
      return u;
    }

    inline static json_value true_value ()
    {
      json_value u;
      u.vt = value_type::true_value;
      return u;
    }

    inline static json_value number_value (double && v)
    {
      json_value u;
      u.vt = value_type::number_value;
      new (&u.number_value) double (std::move (v.number_value));
      return u;
    }

    inline static json_value number_value (double const & v)
    {
      json_value u;
      u.vt = value_type::number_value;
      new (&u.number_value) double (v.number_value);
      return u;
    }

    inline static json_value string_value (std::wstring && v)
    {
      json_value u;
      u.vt = value_type::string_value;
      new (&u.string_value) std::wstring (std::move (v.string_value));
      return u;
    }

    inline static json_value string_value (std::wstring const & v)
    {
      json_value u;
      u.vt = value_type::string_value;
      new (&u.string_value) std::wstring (v.string_value);
      return u;
    }

    inline static json_value array_value (std::vector<json_value> && v)
    {
      json_value u;
      u.vt = value_type::array_value;
      new (&u.array_value) std::vector<json_value> (std::move (v.array_value));
      return u;
    }

    inline static json_value array_value (std::vector<json_value> const & v)
    {
      json_value u;
      u.vt = value_type::array_value;
      new (&u.array_value) std::vector<json_value> (v.array_value);
      return u;
    }

    inline static json_value object_value (std::vector<std::tuple<std::wstring, json_value>> && v)
    {
      json_value u;
      u.vt = value_type::object_value;
      new (&u.object_value) std::vector<std::tuple<std::wstring, json_value>> (std::move (v.object_value));
      return u;
    }

    inline static json_value object_value (std::vector<std::tuple<std::wstring, json_value>> const & v)
    {
      json_value u;
      u.vt = value_type::object_value;
      new (&u.object_value) std::vector<std::tuple<std::wstring, json_value>> (v.object_value);
      return u;
    }

  private:

    template<typename T>
    static void destroy (T & v) noexcept
    {
      v.~T ();
    }

    enum class value_type : std::uint8_t
    {
      empty_value,
      error_value,
      null_value,
      false_value,
      true_value,
      number_value,
      string_value,
      array_value,
      object_value,
    };

    union
    {
      double number_value;
      std::wstring string_value;
      std::vector<json_value> array_value;
      std::vector<std::tuple<std::wstring, json_value>> object_value;
    };

    value_type vt;
  };

}

