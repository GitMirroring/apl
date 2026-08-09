/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright © 2008-2026  Dr. Jürgen Sauermann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/** @file
*/

#include <string.h>
#include <unordered_set>

#include "Bif_F12_ELEMENT.hh"
#include "ConstCell_P.hh"
#include "LvalCell.hh"
#include "Value.hh"
#include "Workspace.hh"

// primitive function instance
//
Bif_F12_ELEMENT   Bif_F12_ELEMENT  ::fun;    // ϵ

//════════════════════════════════════════════════════════════════════════════
Token
Bif_F12_ELEMENT::eval_AB(cValue_R A, cValue_R B) const
{
   // return Z←A ϵ B. Z[i] is 1 iff A[i] = B[j] for some j
   //
const double qct = Workspace::get_CT();
Value_P Z(A.get_shape(), LOC);

const ShapeItem len_A = A.element_count();
   if (len_A == 0)
      {
        Z->set_default(B, LOC);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   // INT64 × INT64 fast path: hash set lookup, BOOL result
   if (A.get_ravel_type() == RPT_INT64 && B.get_ravel_type() == RPT_INT64)
      {
        const int64_t * pA = A.cravel_int64();
        const int64_t * pB = B.cravel_int64();
        const ShapeItem len_B = B.element_count();

        std::unordered_set<int64_t> set_B;
        set_B.reserve(len_B * 2);
        loop(b, len_B)   set_B.insert(pB[b]);

        uint64_t * pZ = reinterpret_cast<uint64_t *>(&Z->get_wfirst());
        const ShapeItem words = (len_A + 63) / 64;
        loop(w, words)   pZ[w] = 0;

        loop(a, len_A)
            {
              if (set_B.count(pA[a]))
                 pZ[a >> 6] |= uint64_t(1) << (a & 63);
            }

        Z->commit_ravel_Bool(len_A);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   for (ConstRavel_P a(A, true); +a; ++a)
       {
         APL_Integer found_a_in_B = 0;
         for (ConstRavel_P b(B, true); +b; ++b)
             {
               if (a->equal(*b, qct))   { found_a_in_B = 1;   break; }
             }

         Z->next_ravel_Int(found_a_in_B);
       }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//════════════════════════════════════════════════════════════════════════════
Value_P
Bif_F12_ELEMENT::do_eval_B(cValue_R B)
{
   // enlist
   //
   // lrm p. 118, ⍴⍴Z = 1, ⍴Z = number of simple scalars in B
   //
   if (B.element_count() == 0)   // empty argument
      {
        Cell cache_C0;
        const Cell & C0 = B.get_cproto(cache_C0);
        if (C0.is_numeric())
           {
             Value_P Z(1, LOC);
             Z->next_ravel_Int(0);
             Z->check_value(LOC);
             return Z;
           }

        if (C0.is_character_cell())
           {
             Value_P Z(1, LOC);
             Z->next_ravel_Char(UNI_SPACE);
             Z->check_value(LOC);
             return Z;
           }

        if (C0.is_lval_cell())
           {
             // (∈⍬)←value is a noop
             //
             Value_P Z(ShapeItem(0), LOC);
             new (&Z->get_wproto()) LvalCell(0, 0);
             Z->check_value(LOC);
             return Z;
           }

        if (Value_P v = C0.try_pointer_value())
            {
             return do_eval_B(*v);
            }


        // not reached
        //
        FIXME;
      }

   // fast path: B is already packed (no PointerCells); ∊B is a packed vector
   // with the same packing as B — just memcpy the raw packed bytes.
   //
   if (B.is_packed())
      {
        const ShapeItem count = B.element_count();
        Value_P Z(count, LOC);
        uint8_t * pZ = reinterpret_cast<uint8_t *>(&Z->get_wfirst());
        const ShapeItem ebytes = B.packed_bytes_per_item();
        const ShapeItem nbytes = ebytes > 0 ? count * ebytes
                                            : (count + 7) / 8;   // bool: bits
        memcpy(pZ, B.cravel_packed(), nbytes);
        Z->commit_ravel_like(B, count);
        Z->check_value(LOC);
        return Z;
      }

const ShapeItem len_Z = B.get_enlist_count();

   // B contains no simple scalars (e.g. ∊⊂''): recurse into the first
   // enclosed value to determine the prototype and return an empty result.
   //
   if (len_Z == 0)
      {
        loop(c, B.element_count())
           {
             if (Value_P v = B.try_pointer_value(c))
                return do_eval_B(*v);
           }
        FIXME;
      }

Value_P Z(len_Z, LOC);

   if (B.get_lval_cellowner())
      {
        B.enlist_left(*Z);
      }
   else
      {
        B.enlist_right(*Z);
        Z->try_pack(true);   // result has no PointerCells; pack regardless of size
      }

   Assert(len_Z);   // cannot be empty
   Z->check_value(LOC);
   return Z;
}
//════════════════════════════════════════════════════════════════════════════
