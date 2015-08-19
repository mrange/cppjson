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

#include <intrin.h>
#include <emmintrin.h>

#include "cpp_json__parser.hpp"

namespace cpp_json { namespace parser
{
  namespace details
  {
    struct special_char
    {
      special_char (wchar_t ch)
      {
        for (auto i = 0U; i < 8U; ++i)
        {
          k.m128i_i16[i] = ch;
        }
      }

      __m128i k;
    };

    special_char special_char__slash  ('\\' );
    special_char special_char__quote  ('"'  );
    special_char special_char__nl     ('\n' );
    special_char special_char__cr     ('\r' );
    special_char special_char__space  (' '  );


    template<>
    struct find_special_char<wchar_t const *>
    {
      using iter_type = wchar_t const *;

      static inline iter_type find (iter_type begin, iter_type end) noexcept
      {
        auto pb = reinterpret_cast<std::intptr_t> (begin);
        auto pe = reinterpret_cast<std::intptr_t> (end);
        auto ab = ~0xF & pb;
        auto ae = ~0xF & (pe + 0xF);

        auto lb   = 0xF & pb;
        auto mask = 0xFFFFFFFF << (lb * 2);

        auto sse2_b = reinterpret_cast<__m128i *> (ab);
        auto sse2_e = reinterpret_cast<__m128i *> (ae);

        for (auto sse2_c = sse2_b; sse2_c < sse2_e; ++sse2_c)
        {
          auto v  = _mm_load_si128 (sse2_c);

/*
          auto a0 = _mm_cmpgt_epi16 (v, compare_with__slash.k);
          auto a  = _mm_movemask_epi8 (a0);
          if (a == 0xFFFF)
          {
            continue;
          }
*/
/*
          auto r0 =                     _mm_cmpeq_epi16 (v, compare_with__nl.k);
          auto r1 = _mm_or_si128  (r0,  _mm_cmpeq_epi16 (v, compare_with__cr.k));
*/
          auto r1 =                     _mm_cmplt_epi16 (v, special_char__space.k);
          auto r2 = _mm_or_si128  (r1,  _mm_cmpeq_epi16 (v, special_char__quote.k));
          auto r3 = _mm_or_si128  (r2,  _mm_cmpeq_epi16 (v, special_char__slash.k));

          auto r  = _mm_movemask_epi8 (r3);
          auto mr = r & mask;
          mask    = 0xFFFFFFFF;

          unsigned long idx;

          if (!_BitScanForward (&idx, mr))
          {
            continue;
          }

          return reinterpret_cast<iter_type> (sse2_c) + (idx >> 1);
        }

        return end;
      }
    };

  }

} }
