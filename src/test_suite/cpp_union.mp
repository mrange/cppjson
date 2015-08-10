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

#include <type_traits>

@@> for ns in $namespace
namespace @@=ns=@@
{
@@> end
@@> for union in $unions
@@>   union_name  = union.get :name
@@>   variants    = union.get(:variants, [])
@@>

  enum class @@=union_name=@@_type : std::uint8_t
  {
    vt__empty_value,
@@>   for variant in variants
@@>     variant_type = variant.get :type
@@>     variant_name = variant.get :name
    vt__@@=variant_name=@@,
@@>   end
  };

  struct @@=union_name=@@
  {
    @@=union_name=@@ () noexcept
      : vt (@@=union_name=@@_type::vt__empty_value)
    {
    }

    ~@@=union_name=@@ () noexcept
    {
      switch (vt)
      {
      default:
      case @@=union_name=@@_type::vt__empty_value:
        break;
@@>   for variant in variants
@@>     variant_type = variant.get :type
@@>     variant_name = variant.get :name
      case @@=union_name=@@_type::vt__@@=variant_name=@@:
@@>     if variant_type != "unit" then
        destroy (v__@@=variant_name=@@);
@@>     end
        break;
@@>   end
      }
      vt = @@=union_name=@@_type::vt__empty_value;
    }

    @@=union_name=@@ (@@=union_name=@@ && v) noexcept
      : vt (v.vt)
    {
      switch (vt)
      {
      default:
      case @@=union_name=@@_type::vt__empty_value:
        break;
@@>   for variant in variants
@@>     variant_type = variant.get :type
@@>     variant_name = variant.get :name
      case @@=union_name=@@_type::vt__@@=variant_name=@@:
@@>     if variant_type != "unit" then
        new (&v__@@=variant_name=@@) @@=variant_type=@@ (std::move (v.v__@@=variant_name=@@));
@@>     end
        break;
@@>   end
      }
      v.vt = @@=union_name=@@_type::vt__empty_value;
    }

    @@=union_name=@@ (@@=union_name=@@ const & v)
      : vt (v.vt)
    {
      switch (vt)
      {
      default:
      case @@=union_name=@@_type::vt__empty_value:
        break;
@@>   for variant in variants
@@>     variant_type = variant.get :type
@@>     variant_name = variant.get :name
      case @@=union_name=@@_type::vt__@@=variant_name=@@:
@@>     if variant_type != "unit" then
        new (&v__@@=variant_name=@@) @@=variant_type=@@ (v.v__@@=variant_name=@@);
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

      destroy (*this);

      switch (v.vt)
      {
      default:
      case @@=union_name=@@_type::vt__empty_value:
        break;
@@>   for variant in variants
@@>     variant_type = variant.get :type
@@>     variant_name = variant.get :name
      case @@=union_name=@@_type::vt__@@=variant_name=@@:
@@>     if variant_type != "unit" then
        new (&v__@@=variant_name=@@) @@=variant_type=@@ (std::move (v.v__@@=variant_name=@@));
@@>     end
        break;
@@>   end
      }

      vt = v.vt;
      v.vt = @@=union_name=@@_type::vt__empty_value;

      return *this;
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

    void swap (@@=union_name=@@ & v) noexcept
    {
      std::swap (*this, v);
    }

    @@=union_name=@@_type get__type () const noexcept
    {
      return vt;
    }

    bool is__empty_value () const
    {
      return vt == @@=union_name=@@_type::vt__empty_value;
    }

    static @@=union_name=@@ empty_value ()
    {
      return @@=union_name=@@ ();
    }

@@>   for variant in variants
@@>     variant_type = variant.get :type
@@>     variant_name = variant.get :name
    bool is__@@=variant_name=@@ () const
    {
      return vt == @@=union_name=@@_type::vt__@@=variant_name=@@;
    }

@@>     if variant_type != "unit" then
    static @@=union_name=@@ @@=variant_name=@@ (@@=variant_type=@@ && v)
    {
      @@=union_name=@@ u;
      u.vt = @@=union_name=@@_type::vt__@@=variant_name=@@;
      new (&u.v__@@=variant_name=@@) @@=variant_type=@@ (std::move (v));
      return u;
    }

    static @@=union_name=@@ @@=variant_name=@@ (@@=variant_type=@@ const & v)
    {
      @@=union_name=@@ u;
      u.vt = @@=union_name=@@_type::vt__@@=variant_name=@@;
      new (&u.v__@@=variant_name=@@) @@=variant_type=@@ (v);
      return u;
    }

    bool get__@@=variant_name=@@ (@@=variant_type=@@ & v) const
    {
      if (vt == @@=union_name=@@_type::vt__@@=variant_name=@@)
      {
        v = v__@@=variant_name=@@;

        return true;
      }
      else
      {
        return false;
      }
    }

    void set__@@=variant_name=@@ (@@=variant_type=@@ && v) noexcept
    {
      *this = @@=union_name=@@::@@=variant_name=@@ (std::move (v));
    }

    void set__@@=variant_name=@@ (@@=variant_type=@@ const & v)
    {
      *this = @@=union_name=@@::@@=variant_name=@@ (v);
    }
@@>     else
    static @@=union_name=@@ @@=variant_name=@@ ()
    {
      @@=union_name=@@ u;
      u.vt = @@=union_name=@@_type::vt__@@=variant_name=@@;
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

    union
    {
@@>   for variant in variants
@@>     variant_type = variant.get :type
@@>     variant_name = variant.get :name
@@>     if variant_type != "unit" then
      @@=variant_type=@@ v__@@=variant_name=@@;
@@>     end
@@>   end
    };

    @@=union_name=@@_type vt;
  };

@@> end
@@> for ns in $namespace.reverse
} // @@=ns=@@
@@> end

@@+ class Hash
@@+     def get (key, defaultValue=nil)
@@+         value = self[key]
@@+         return value unless value == nil
@@+         return defaultValue || ("KEY_'%s'_NOT_FOUND" % key.to_s)
@@+     end
@@+ end

