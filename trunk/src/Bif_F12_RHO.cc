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

#include "Bif_F12_RHO.hh"
#include "StateIndicator.hh"
#include "Value.hh"
#include "Workspace.hh"

// primitive function instance
//
Bif_F12_RHO       Bif_F12_RHO      ::fun;    // ⍴

//════════════════════════════════════════════════════════════════════════════
Token
Bif_F12_RHO::eval_AB(cValue_R A, cValue_R B) const
{
#ifdef cfg_PERFORMANCE_COUNTERS_WANTED
const uint64_t start_1 = cycle_counter();
#endif

const Shape shape_Z(A, 0);

   // check that shape_Z is positive
   //
   loop(r, shape_Z.get_rank())
      {
        if (shape_Z.get_shape_item(r) < 0)   DOMAIN_ERROR;
      }

const ShapeItem len_Z = shape_Z.get_volume();

   if (DO_RT_A_RHO_B                      &&
       len_Z <= B.element_count()         &&   // 1.   Z is not longer than B
       B.get_owner_count() == 1           &&   // 2.   B is a temporary value
       (len_Z > 0 || !B.is_packed())      &&   // 3.   empty+packed: init_type would corrupt proto slot
       this == Workspace::SI_top()->get_prefix().get_dyadic_fun())   // 4. below
      {
        /* Optimization of Z←A⍴B. At this point:

          1. Z is not longer than B, and
          2. B has only 1 owner: the prefix (who will discard it after we
             return). Previously the check was == 2 when B arrived as Value_P
             (prefix + our local copy); now B arrives as const cValue * so
             the prefix is the sole owner.
          3. A⍴B was called from a reduction rule (Prefix::reduce_A_F_B())

           We will give up our ownership on return below, and
           Prefix::reduce_A_F_B prefix will Prefix::pop_args_push_result()
           and hence give up its ownership, causing B to be erased.

           That means that B will no longer be used and that, instead of
           of copying B into a new Z and then erasing B, we can reshape B
           in place and return the reshaped B.

           return Token(TOK_APL_VALUE1, vB); below will take ownership of B
           so that Prefix::reduce_A_F_B() won't erase B.
         */
        Log(LOG_optimization) CERR << "optimizing A⍴B" << endl;

Value * vB = static_cast<Value *>(const_cast<cValue *>(&B));

        // release the no longer used cells of B after shape_Z.
        //
        const ShapeItem len_B = B.element_count();   // all Cells
        ShapeItem rest = len_Z;                       // Cells remaining
        if (rest == 0)   // Z is empty
           {
             rest = 1;
             if (B.is_pointer_cell(0))
                {
                  B.get_pointer_value(0)->to_type(false);
                }
             else
                {
                   vB->get_wproto().init_type(B.get_cproto(), *vB, LOC);
                }
           }

        // release the Cells after Z (packed ravels have no per-cell heap allocations)
        if (!B.is_packed())
           while (rest < len_B)   vB->release(rest++, LOC);

        vB->set_shape(shape_Z);

#ifdef cfg_PERFORMANCE_COUNTERS_WANTED
const uint64_t end_1 = cycle_counter();
   Performance::fs_F12_RHO_AB.add_sample(end_1 - start_1,
                                         vB->nz_element_count());
#endif

        OptmizationStatistics::count(OPTI_RT_A_RHO_B);
        return Token(TOK_APL_VALUE1, Value_P(vB, LOC));
      }

#ifdef cfg_PERFORMANCE_COUNTERS_WANTED
Token ret = do_reshape(shape_Z, B);
const uint64_t end_1 = cycle_counter();
   Performance::fs_F12_RHO_AB.add_sample(end_1 - start_1,
                                         shape_Z.get_volume());
   return ret;
#else
   return do_reshape(shape_Z, B);
#endif
}
//────────────────────────────────────────────────────────────────────────────
Token
Bif_F12_RHO::eval_B(cValue_R B) const
{
const sRank rank = B.get_rank();
Value_P Z(rank, LOC);

   {
     int64_t * pZ = reinterpret_cast<int64_t *>(&Z->get_wfirst());
     loop(r, rank)   pZ[r] = B.get_shape_item(r);
     Z->commit_ravel_Int64(rank);
   }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//────────────────────────────────────────────────────────────────────────────
Token
Bif_F12_RHO::do_reshape(const Shape & shape_Z, const cValue & B)
{
const ShapeItem len_B = B.element_count();

Value_P Z(shape_Z, LOC);
const ShapeItem len_Z = Z->element_count();

   if (len_B == 0)   // empty B: use prototype of B for all cells of Z
      {
        loop(z, len_Z)   Z->next_ravel_Proto(B.get_cproto());
      }
   else
      {
        const ShapeItem bpi = B.packed_bytes_per_item();   // 0 for CELLS or BOOL
        if (bpi > 0 && len_Z > 0)
           {
             // packed ravel: copy B cycling over Z with memcpy
             const char * pB = static_cast<const char *>(B.cravel_packed());
             char * pZ = reinterpret_cast<char *>(&Z->get_wfirst());
             ShapeItem done = 0;
             while (done < len_Z)
                 {
                   const ShapeItem chunk = len_Z - done < len_B ? len_Z - done : len_B;
                   memcpy(pZ + done * bpi, pB, chunk * bpi);
                   done += chunk;
                 }
             Z->commit_ravel_like(B, len_Z);
           }
        else if (B.is_bool_packed() && len_Z > 0)
           {
             // BOOL fast path: cyclic bit copy without exploding B
             const uint64_t * pB = B.cravel_bool();
             uint64_t * pZ = reinterpret_cast<uint64_t *>(&Z->get_wfirst());
             const ShapeItem words_Z = (len_Z + 63) / 64;
             memset(pZ, 0, words_Z * sizeof(uint64_t));
             ShapeItem sb = 0;
             for (ShapeItem i = 0; i < len_Z; ++i)
                {
                  pZ[i >> 6] |= ((pB[sb >> 6] >> (sb & 63)) & 1) << (i & 63);
                  if (++sb == len_B) sb = 0;
                }
             Z->commit_ravel_Bool(len_Z);
           }
        else
           {
             loop(z, len_Z)
               {
                 Z->next_ravel_Cell(B.get_cravel(z % len_B));
               }
           }
      }

   Z->set_default(B, LOC);
   Z->check_value(LOC);
   Z->try_pack();
   return Token(TOK_APL_VALUE1, Z);
}
//════════════════════════════════════════════════════════════════════════════
