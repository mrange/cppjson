require 'carnelian/runtime'
 class Hash
     def get (key, defaultValue=nil)
         value = self[key]
         return value unless value == nil
         return defaultValue || ("KEY_'%s'_NOT_FOUND" % key.to_s)
     end
 end
def generate_document (document)
    document.write '// ----------------------------------------------------------------------------------------------'
    document.new_line
    document.write '// Copyright 2015 Mårten Rånge'
    document.new_line
    document.write '//'
    document.new_line
    document.write '// Licensed under the Apache License, Version 2.0 (the "License");'
    document.new_line
    document.write '// you may not use this file except in compliance with the License.'
    document.new_line
    document.write '// You may obtain a copy of the License at'
    document.new_line
    document.write '//'
    document.new_line
    document.write '//     http://www.apache.org/licenses/LICENSE-2.0'
    document.new_line
    document.write '//'
    document.new_line
    document.write '// Unless required by applicable law or agreed to in writing, software'
    document.new_line
    document.write '// distributed under the License is distributed on an "AS IS" BASIS,'
    document.new_line
    document.write '// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.'
    document.new_line
    document.write '// See the License for the specific language governing permissions and'
    document.new_line
    document.write '// limitations under the License.'
    document.new_line
    document.write '// ----------------------------------------------------------------------------------------------'
    document.new_line
    document.new_line
    document.write 'namespace '
    document.write ($namespace)
    document.new_line
    document.write '{'
    document.new_line
     for union in $unions
       union_name  = union.get :name
       variants    = union.get(:variants, [])
    
    document.new_line
    document.write '  struct '
    document.write (union_name)
    document.new_line
    document.write '  {'
    document.new_line
    document.write '    '
    document.write (union_name)
    document.write ' () noexcept'
    document.new_line
    document.write '      vt (value_type::empty_value)'
    document.new_line
    document.write '    {'
    document.new_line
    document.write '    }'
    document.new_line
    document.new_line
    document.write '    ~'
    document.write (union_name)
    document.write ' () noexcept'
    document.new_line
    document.write '    {'
    document.new_line
    document.write '      switch (vt)'
    document.new_line
    document.write '      {'
    document.new_line
    document.write '      default:'
    document.new_line
    document.write '      case value_type::empty_value:'
    document.new_line
    document.write '        break;'
    document.new_line
       for variant in variants
         variant_type = variant.get :type
         variant_name = variant.get :name
    document.write '      case value_type::'
    document.write (variant_name)
    document.write ':'
    document.new_line
         if variant_type != "unit" then
    document.write '        destroy ('
    document.write (variant_name)
    document.write ');'
    document.new_line
         end
    document.write '        break;'
    document.new_line
       end
    document.write '      }'
    document.new_line
    document.write '      vt = value_type::empty_value;'
    document.new_line
    document.write '    }'
    document.new_line
    document.new_line
    document.write '    '
    document.write (union_name)
    document.write ' ('
    document.write (union_name)
    document.write ' && v) noexcept'
    document.new_line
    document.write '      : vt (v.vt)'
    document.new_line
    document.write '    {'
    document.new_line
    document.write '      switch (vt)'
    document.new_line
    document.write '      {'
    document.new_line
    document.write '      default:'
    document.new_line
    document.write '      case value_type::empty_value:'
    document.new_line
    document.write '        break;'
    document.new_line
       for variant in variants
         variant_type = variant.get :type
         variant_name = variant.get :name
    document.write '      case value_type::'
    document.write (variant_name)
    document.write ':'
    document.new_line
         if variant_type != "unit" then
    document.write '        new (&'
    document.write (variant_name)
    document.write ') '
    document.write (variant_type)
    document.write ' (std::move (v.'
    document.write (variant_name)
    document.write '));'
    document.new_line
         end
    document.write '        break;'
    document.new_line
       end
    document.write '      }'
    document.new_line
    document.write '      v.vt = value_type::empty_value;'
    document.new_line
    document.write '    }'
    document.new_line
    document.new_line
    document.write '    '
    document.write (union_name)
    document.write ' ('
    document.write (union_name)
    document.write ' const & v)'
    document.new_line
    document.write '      : vt (v.vt)'
    document.new_line
    document.write '    {'
    document.new_line
    document.write '      switch (vt)'
    document.new_line
    document.write '      {'
    document.new_line
    document.write '      default:'
    document.new_line
    document.write '      case value_type::empty_value:'
    document.new_line
    document.write '        break;'
    document.new_line
       for variant in variants
         variant_type = variant.get :type
         variant_name = variant.get :name
    document.write '      case value_type::'
    document.write (variant_name)
    document.write ':'
    document.new_line
         if variant_type != "unit" then
    document.write '        new (&'
    document.write (variant_name)
    document.write ') '
    document.write (variant_type)
    document.write ' (v.'
    document.write (variant_name)
    document.write ');'
    document.new_line
         end
    document.write '        break;'
    document.new_line
       end
    document.write '      }'
    document.new_line
    document.write '    }'
    document.new_line
    document.new_line
    document.write '    '
    document.write (union_name)
    document.write ' & operator= ('
    document.write (union_name)
    document.write ' && v) noexcept'
    document.new_line
    document.write '    {'
    document.new_line
    document.write '      if (this == &v)'
    document.new_line
    document.write '      {'
    document.new_line
    document.write '        return *this;'
    document.new_line
    document.write '      }'
    document.new_line
    document.new_line
    document.write '      ~'
    document.write (union_name)
    document.write ' ();'
    document.new_line
    document.new_line
    document.write '      switch (vt)'
    document.new_line
    document.write '      {'
    document.new_line
    document.write '      default:'
    document.new_line
    document.write '      case value_type::empty_value:'
    document.new_line
    document.write '        break;'
    document.new_line
       for variant in variants
         variant_type = variant.get :type
         variant_name = variant.get :name
    document.write '      case value_type::'
    document.write (variant_name)
    document.write ':'
    document.new_line
         if variant_type != "unit" then
    document.write '        new (&'
    document.write (variant_name)
    document.write ') '
    document.write (variant_type)
    document.write ' (std::move (v.'
    document.write (variant_name)
    document.write '));'
    document.new_line
         end
    document.write '        break;'
    document.new_line
       end
    document.write '      }'
    document.new_line
    document.new_line
    document.write '      vt = v.vt;'
    document.new_line
    document.write '      v.vt = value_type::empty_value;'
    document.new_line
    document.write '    }'
    document.new_line
    document.new_line
    document.write '    '
    document.write (union_name)
    document.write ' & operator= ('
    document.write (union_name)
    document.write ' const & v)'
    document.new_line
    document.write '    {'
    document.new_line
    document.write '      if (this == &v)'
    document.new_line
    document.write '      {'
    document.new_line
    document.write '        return *this;'
    document.new_line
    document.write '      }'
    document.new_line
    document.new_line
    document.write '      '
    document.write (union_name)
    document.write ' copy (v);'
    document.new_line
    document.new_line
    document.write '      return *this = std::move (copy);'
    document.new_line
    document.write '    }'
    document.new_line
    document.new_line
    document.write '    static '
    document.write (union_name)
    document.write ' empty_value ()'
    document.new_line
    document.write '    {'
    document.new_line
    document.write '      return '
    document.write (union_name)
    document.write ' ();'
    document.new_line
    document.write '    }'
    document.new_line
    document.new_line
       for variant in variants
         variant_type = variant.get :type
         variant_name = variant.get :name
         if variant_type != "unit" then
    document.write '    inline static '
    document.write (union_name)
    document.write ' '
    document.write (variant_name)
    document.write ' ('
    document.write (variant_type)
    document.write ' && v)'
    document.new_line
    document.write '    {'
    document.new_line
    document.write '      '
    document.write (union_name)
    document.write ' u;'
    document.new_line
    document.write '      u.vt = value_type::'
    document.write (variant_name)
    document.write ';'
    document.new_line
    document.write '      new (&u.'
    document.write (variant_name)
    document.write ') '
    document.write (variant_type)
    document.write ' (std::move (v.'
    document.write (variant_name)
    document.write '));'
    document.new_line
    document.write '      return u;'
    document.new_line
    document.write '    }'
    document.new_line
    document.new_line
    document.write '    inline static '
    document.write (union_name)
    document.write ' '
    document.write (variant_name)
    document.write ' ('
    document.write (variant_type)
    document.write ' const & v)'
    document.new_line
    document.write '    {'
    document.new_line
    document.write '      '
    document.write (union_name)
    document.write ' u;'
    document.new_line
    document.write '      u.vt = value_type::'
    document.write (variant_name)
    document.write ';'
    document.new_line
    document.write '      new (&u.'
    document.write (variant_name)
    document.write ') '
    document.write (variant_type)
    document.write ' (v.'
    document.write (variant_name)
    document.write ');'
    document.new_line
    document.write '      return u;'
    document.new_line
    document.write '    }'
    document.new_line
    document.new_line
         else
    document.write '    inline static '
    document.write (union_name)
    document.write ' '
    document.write (variant_name)
    document.write ' ()'
    document.new_line
    document.write '    {'
    document.new_line
    document.write '      '
    document.write (union_name)
    document.write ' u;'
    document.new_line
    document.write '      u.vt = value_type::'
    document.write (variant_name)
    document.write ';'
    document.new_line
    document.write '      return u;'
    document.new_line
    document.write '    }'
    document.new_line
    document.new_line
         end
       end
    document.write '  private:'
    document.new_line
    document.new_line
    document.write '    template<typename T>'
    document.new_line
    document.write '    static void destroy (T & v) noexcept'
    document.new_line
    document.write '    {'
    document.new_line
    document.write '      v.~T ();'
    document.new_line
    document.write '    }'
    document.new_line
    document.new_line
    document.write '    enum class value_type : std::uint8_t'
    document.new_line
    document.write '    {'
    document.new_line
    document.write '      empty_value,'
    document.new_line
       for variant in variants
         variant_type = variant.get :type
         variant_name = variant.get :name
    document.write '      '
    document.write (variant_name)
    document.write ','
    document.new_line
       end
    document.write '    };'
    document.new_line
    document.new_line
    document.write '    union'
    document.new_line
    document.write '    {'
    document.new_line
       for variant in variants
         variant_type = variant.get :type
         variant_name = variant.get :name
         if variant_type != "unit" then
    document.write '      '
    document.write (variant_type)
    document.write ' '
    document.write (variant_name)
    document.write ';'
    document.new_line
         end
       end
    document.write '    };'
    document.new_line
    document.new_line
    document.write '    value_type vt;'
    document.new_line
    document.write '  };'
    document.new_line
    document.new_line
     end
    document.write '}'
    document.new_line
    document.new_line
    document.new_line
end

document = CarnelianRuntime.create_document

generate_document document

document.get_lines
