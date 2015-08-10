@@@ metaprogram
@@@ extension hpp
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

namespace @@=$namespace=@@
{
@@> for union in $unions
@@>   union_name  = union.get :name
@@>   variants    = union.get(:variants, [])
@@>

  struct @@=union_name=@@
  {
    @@=union_name=@@ () noexcept
      vt (value_type::empty_value)
    {
    }

    ~@@=union_name=@@ () noexcept
    {
      switch (vt)
      {
      default:
      case value_type::empty_value:
        break;
@@>   for variant in variants
@@>     variant_type = variant.get :type
@@>     variant_name = variant.get :name
      case value_type::@@=variant_name=@@:
@@>     if variant_type != "unit" then
        destroy (@@=variant_name=@@);
@@>     end
        break;
@@>   end
      }
      vt = value_type::empty_value;
    }

    @@=union_name=@@ (@@=union_name=@@ && v) noexcept
      : vt (v.vt)
    {
      switch (vt)
      {
      default:
      case value_type::empty_value:
        break;
@@>   for variant in variants
@@>     variant_type = variant.get :type
@@>     variant_name = variant.get :name
      case value_type::@@=variant_name=@@:
@@>     if variant_type != "unit" then
        new (&@@=variant_name=@@) @@=variant_type=@@ (std::move (v.@@=variant_name=@@));
@@>     end
        break;
@@>   end
      }
      v.vt = value_type::empty_value;
    }

    @@=union_name=@@ (@@=union_name=@@ const & v)
      : vt (v.vt)
    {
      switch (vt)
      {
      default:
      case value_type::empty_value:
        break;
@@>   for variant in variants
@@>     variant_type = variant.get :type
@@>     variant_name = variant.get :name
      case value_type::@@=variant_name=@@:
@@>     if variant_type != "unit" then
        new (&@@=variant_name=@@) @@=variant_type=@@ (v.@@=variant_name=@@);
@@>     end
        break;
@@>   end
      }
    }

    @@=union_name=@@ & operator= (@@=union_name=@@ && v) noexcept
    {
      if (this == &v)
      {
        return *this;
      }

      ~@@=union_name=@@ ();

      switch (vt)
      {
      default:
      case value_type::empty_value:
        break;
@@>   for variant in variants
@@>     variant_type = variant.get :type
@@>     variant_name = variant.get :name
      case value_type::@@=variant_name=@@:
@@>     if variant_type != "unit" then
        new (&@@=variant_name=@@) @@=variant_type=@@ (std::move (v.@@=variant_name=@@));
@@>     end
        break;
@@>   end
      }

      vt = v.vt;
      v.vt = value_type::empty_value;
    }

    @@=union_name=@@ & operator= (@@=union_name=@@ const & v)
    {
      if (this == &v)
      {
        return *this;
      }

      @@=union_name=@@ copy (v);

      return *this = std::move (copy);
    }

    static @@=union_name=@@ empty_value ()
    {
      return @@=union_name=@@ ();
    }

@@>   for variant in variants
@@>     variant_type = variant.get :type
@@>     variant_name = variant.get :name
@@>     if variant_type != "unit" then
    inline static @@=union_name=@@ @@=variant_name=@@ (@@=variant_type=@@ && v)
    {
      @@=union_name=@@ u;
      u.vt = value_type::@@=variant_name=@@;
      new (&u.@@=variant_name=@@) @@=variant_type=@@ (std::move (v.@@=variant_name=@@));
      return u;
    }

    inline static @@=union_name=@@ @@=variant_name=@@ (@@=variant_type=@@ const & v)
    {
      @@=union_name=@@ u;
      u.vt = value_type::@@=variant_name=@@;
      new (&u.@@=variant_name=@@) @@=variant_type=@@ (v.@@=variant_name=@@);
      return u;
    }

@@>     else
    inline static @@=union_name=@@ @@=variant_name=@@ ()
    {
      @@=union_name=@@ u;
      u.vt = value_type::@@=variant_name=@@;
      return u;
    }

@@>     end
@@>   end
  private:

    template<typename T>
    static void destroy (T & v) noexcept
    {
      v.~T ();
    }

    enum class value_type : std::uint8_t
    {
      empty_value,
@@>   for variant in variants
@@>     variant_type = variant.get :type
@@>     variant_name = variant.get :name
      @@=variant_name=@@,
@@>   end
    };

    union
    {
@@>   for variant in variants
@@>     variant_type = variant.get :type
@@>     variant_name = variant.get :name
@@>     if variant_type != "unit" then
      @@=variant_type=@@ @@=variant_name=@@;
@@>     end
@@>   end
    };

    value_type vt;
  };

@@> end
}

@@+ class Hash
@@+     def get (key, defaultValue=nil)
@@+         value = self[key]
@@+         return value unless value == nil
@@+         return defaultValue || ("KEY_'%s'_NOT_FOUND" % key.to_s)
@@+     end
@@+ end

