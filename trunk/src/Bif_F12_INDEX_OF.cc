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

#include <algorithm>
#include <utility>
#include <vector>
#include "Bif_F12_INDEX_OF.hh"
#include "Workspace.hh"

Bif_F12_INDEX_OF Bif_F12_INDEX_OF ::fun;    // ⍳

//════════════════════════════════════════════════════════════════════════════
/// search elements of B in A. ⍴Z is ⍴B, and elements of Z are indices of A.
//════════════════════════════════════════════════════════════════════════════
Token
Bif_F12_INDEX_OF::eval_AB(cValue_R A, cValue_R B) const
{
   // A⍳B (aka. Index of)
   //
const bool simple_result = A.is_scalar_or_vector();
const double qct = Workspace::get_CT();
const APL_Integer qio = Workspace::get_IO();

const ShapeItem len_A  = A.element_count();
const ShapeItem len_BZ = B.element_count();

   // INT × INT linear fast path: exact comparison (no ⎕CT), direct int64_t I/O.
   // Only taken when the linear search branch would run anyway (small A or B).
   if (simple_result
       && A.get_ravel_type() == RPT_INT64
       && B.get_ravel_type() == RPT_INT64
       && !(len_A >= 64 && len_BZ > 5))
      {
        const int64_t * pA = A.cravel_int64();
        const int64_t * pB = B.cravel_int64();

        Value_P Z(B.get_shape(), LOC);
        int64_t * pZ = reinterpret_cast<int64_t *>(&Z->get_wfirst());

        loop(bz, len_BZ)
            {
              ShapeItem z = len_A;
              const int64_t b = pB[bz];
              loop(a, len_A)   if (pA[a] == b) { z = a; break; }
              pZ[bz] = qio + z;
            }

        Z->commit_ravel_Int64(len_BZ);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   // INT64 × INT64 sorted fast path: O(N log N) build + O(M log N) lookups.
   // Only applicable when A is a vector (simple_result=true).
   if (simple_result
       && A.get_ravel_type() == RPT_INT64
       && B.get_ravel_type() == RPT_INT64
       && len_A >= 64 && len_BZ > 5)
      {
        const int64_t * pA = A.cravel_int64();
        const int64_t * pB = B.cravel_int64();

        // sort (value, original-index) pairs; equal values sort by index so
        // lower_bound finds the minimum (first) index for each value
        vector<pair<int64_t, ShapeItem>> sa(len_A);
        loop(a, len_A)   sa[a] = {pA[a], a};
        std::sort(sa.begin(), sa.end());

        Value_P Z(B.get_shape(), LOC);
        int64_t * pZ = reinterpret_cast<int64_t *>(&Z->get_wfirst());
        loop(bz, len_BZ)
            {
              const int64_t b = pB[bz];
              auto it = std::lower_bound(sa.begin(), sa.end(),
                                         make_pair(b, ShapeItem(-1)));
              pZ[bz] = (it != sa.end() && it->first == b)
                       ? it->second + qio
                       : len_A   + qio;
            }

        Z->commit_ravel_Int64(len_BZ);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

Value_P Z(B.get_shape(), LOC);

#if 1
   // ⎕RL←42 ◊ D←?100 100⍴100 ◊ T←⎕FIO ¯1 ◊ ⊣(⍳100) ⍳ D ◊ -T-⎕FIO ¯1

   if (len_A >= 64 && len_BZ > 5)
      {
        // array A is being searched len_BZ times. We therefore reduce the
        // per-item search time from O(len_A) to O(log(len_A)). That costs us
        // O(len_A × log(len_A)) for sorting A, but hopefully pays off if
        // len_BZ > log(len_A).
        //
        // We don't do that for too small A though, as to compensate for the
        // start-up cost of the sorting.
        //
        vector<ShapeItem> sorted_idx_A;
        Cell::sorted_indices(sorted_idx_A, A, SORT_ASCENDING, 1);
        loop(bz, len_BZ)
            {
              const APL_Integer z = find_B_in_sorted_A(A, sorted_idx_A,
                                                       B.get_cravel(bz), qct);

              if (simple_result)   Z->next_ravel_Int(qio + z);
              else if (z == len_A)   // not found: set result item to ⍬
                 {
                   Value_P zilde(ShapeItem(0), LOC);
                   Z->next_ravel_Pointer(zilde.get());
                 }
              else                   // element found (first at z (+⎕IO)
                 {
                   const Shape Sz = A.get_shape().offset_to_index(z, qio);
                   Value_P Vz(LOC, &Sz);
                   Z->next_ravel_Pointer(Vz.get());
                 }
            }
      }
   else
#endif
      {
        loop(bz, len_BZ)
            {
              const APL_Integer z = find_B_in_A(A, len_A,
                                               B.get_cravel(bz), qct);

              if (simple_result)   Z->next_ravel_Int(qio + z);
              else if (z == len_A)   // not found: set result item to ⍬
                 {
                   Value_P zilde(ShapeItem(0), LOC);
                   Z->next_ravel_Pointer(zilde.get());
                 }
              else                   // element found (first at z (+⎕IO)
                 {
                   const Shape Sz = A.get_shape().offset_to_index(z, qio);
                   Value_P Vz(LOC, &Sz);
                   Z->next_ravel_Pointer(Vz.get());
                 }
            }
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//════════════════════════════════════════════════════════════════════════════
Token
Bif_F12_INDEX_OF::eval_B(cValue_R B) const
{
   if (B.get_rank() > 1)   RANK_ERROR;

const APL_Integer qio = Workspace::get_IO();
const ShapeItem ec = B.element_count();

   if (ec == 1)
      {
        // interval (standard ⍳B with scalar or 1-element B)
        //
        const APL_Integer len = B.get_near_int(0);
        if (len < 0)   DOMAIN_ERROR;

        Value_P Z(len, LOC);

        {
          int64_t * pZ = reinterpret_cast<int64_t *>(&Z->get_wfirst());
          loop(z, len)   pZ[z] = qio + z;
          Z->commit_ravel_Int64(len);
        }

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   // generalized ⍳B a la Dyalog APL...
   //
   // ⍴B  ←→  ⍴⍴Z, B = ⍴Z
   if (ec == 0)
      {
        Value_P Z(LOC);
        Z->next_ravel_Pointer(Idx0(LOC).get());
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

Shape sh_Z(B, 0);
   loop(b, ec)   if (sh_Z.get_shape_item(b) < 0)   DOMAIN_ERROR;

   // at this point sh is correct and ⍳ cannot fail.
   //
Value_P Z(sh_Z, LOC);
const sRank rank_Z = Z->get_rank();
   loop(z, Z->element_count())
      {
        Value_P ZZ(rank_Z, LOC);
        ShapeItem N = z;
        ShapeItem * zz = ALLOCA(ShapeItem, rank_Z);
        loop(r, rank_Z)
            {
              const ShapeItem q = sh_Z.get_shape_item(ec - r - 1);
              zz[ec - r - 1] = N % q + qio;
              N /= q;
            }

        loop(r, rank_Z)   ZZ->next_ravel_Int(zz[r]);

        ZZ->check_value(LOC);
        Z->next_ravel_Pointer(ZZ.get());
      }

   if (Z->element_count() == 0)   // empty result
      {
        Value_P ZZ(rank_Z, LOC);   // ZZ←(⍴⍴Z)⍴0...
        while (ZZ->more())   ZZ->next_ravel_0();
        ZZ->check_value(LOC);

        new (&Z->get_wproto())   PointerCell(ZZ.get(), *Z); // ⊂(⍴⍴Z)⍴0
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//════════════════════════════════════════════════════════════════════════════
int
Bif_F12_INDEX_OF::bs_cmp(const Cell & cell, const ShapeItem & A,
                         const void * ctx)
{
const cValue * VA = reinterpret_cast<const cValue *>(ctx);
const Cell & cell_A = VA->get_cravel(A);

   if (cell_A.is_pointer_cell() && !cell.is_pointer_cell())   return COMP_LT;

const int ret = cell.compare(cell_A);
   return ret;
}
//────────────────────────────────────────────────────────────────────────────
ShapeItem
Bif_F12_INDEX_OF::find_B_in_sorted_A(const cValue & A,
                                     const vector<ShapeItem> & Idx_A,
                                     const Cell & cell_B, double qct)
{
const ShapeItem len_A = A.element_count();
   Assert(size_t(len_A) == Idx_A.size());
const ShapeItem * const posp =
      Heapsort<ShapeItem>::search<const Cell &>(cell_B, Idx_A,
                                                &bs_cmp, &A);
   if (!posp)   return len_A;   // cell_B was not found in ravel A

ShapeItem pos = Idx_A[posp - Idx_A.data()];   // A[pos] = cell_B within qct
   Assert(cell_B.equal(A.get_cravel(pos), qct));

   // A[pos] = cell_B, but there could be predecessors of pos that also
   // satisfy A[pos] = cell_B. Search neighbor with smallest index in A.
   //
ShapeItem ret = pos;
   for (const ShapeItem * posp1 = posp - 1; posp1 >= Idx_A.data(); --posp1)
       {
         ShapeItem pos1 = Idx_A[posp1 - Idx_A.data()];
         const Cell & C1 = A.get_cravel(pos1);
         if (!cell_B.equal(C1, qct))    break;
         if (ret > pos1)   ret = pos1;
       }

   for (const ShapeItem * posp2 = posp + 1; posp2 < (Idx_A.data() + len_A); ++posp2)
       {
         ShapeItem pos2 = Idx_A[posp2 - Idx_A.data()];
         const Cell & C2 = A.get_cravel(pos2);
         if (!cell_B.equal(C2, qct))    break;
         if (ret > pos2)   ret = pos2;
       }

   return ret;
}
//════════════════════════════════════════════════════════════════════════════

