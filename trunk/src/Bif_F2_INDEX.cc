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

#include "Bif_F2_INDEX.hh"
#include "IndexExpr.hh"
#include "Value.hh"
#include "Workspace.hh"

// primitive function instance
//
Bif_F2_INDEX      Bif_F2_INDEX     ::fun;    // ⌷

//════════════════════════════════════════════════════════════════════════════
Token
Bif_F2_INDEX::eval_AB(cValue_R A, cValue_R B) const
{
   if (A.get_rank() > 1)   RANK_ERROR;

const ShapeItem ec_A = A.element_count();
   if (ec_A != B.get_rank())   RANK_ERROR;

   // index_expr is in reverse order!
   //
IndexExpr index_expr(ASS_none, LOC);
   loop(a, ec_A)
      {
         const Cell & cell = A.get_cravel(ec_A - a - 1);
         if (cell.is_pointer_cell())
            {
              Value_P val = CLONE_P(cell.get_pointer_value(), LOC);
              if (val->compute_depth() > 1)   DOMAIN_ERROR;
              index_expr.add_index(val);
            }
        else
            {
              const APL_Integer I = cell.get_near_int();
              if (I < 0)   DOMAIN_ERROR;
              index_expr.add_index(IntScalar(I, LOC));
            }
      }

   // the index() functions do set_default() and check_value(),
   // so we return immediately without doint it again.
   //
   index_expr.quad_io = Workspace::get_IO();

   if (index_expr.is_axis())   // [ ] or [ axis ]
      {
        Value_P single_index = index_expr.extract_axis();
        Value_P Z = B.index(*single_index);
        return Token(TOK_APL_VALUE1, Z);
      }
   else
      {
        Value_P Z = B.index(index_expr);
        return Token(TOK_APL_VALUE1, Z);
      }
}
//────────────────────────────────────────────────────────────────────────────
Token
Bif_F2_INDEX::eval_AXB(cValue_R A, cValue_R X, cValue_R B) const
{
   if (A.get_rank() > 1)   RANK_ERROR;

const AxesBitmap axes_X = X.to_bitmap("⌷[X] B", B.get_rank());

const ShapeItem ec_A = A.element_count();
   if (ec_A != X.element_count())   RANK_ERROR;
   if (ec_A > B.get_rank())              RANK_ERROR;

   // construct an IndexExpr in index (= parse-) order (i.e. the index_expr[0]
   // corresponds to the lasr axis ¯1↑⍴B of B). We therefore move backwards
   // from the end of A resp. X.
   //
IndexExpr index_expr(ASS_none, LOC);   // start with an empty IndexExpr
   index_expr.quad_io = Workspace::get_IO();

ShapeItem a = ec_A;   // index_expr[0] ←→  B[;;;b]
   for (sAxis b = B.get_rank() - 1; b >= 0; --b)
       {
         if (!(axes_X & 1 << b))   // Axis  b was not in X: elided idx
            {
              index_expr.add_index(Value_P());   // add elided index
              continue;
            }

         const Cell & cell_A = A.get_cravel(--a);
          if (cell_A.is_pointer_cell())
             {
               Value_P val = CLONE_P(cell_A.get_pointer_value(), LOC);
               if (val->compute_depth() > 1)   DOMAIN_ERROR;
              index_expr.add_index(val);
             }
         else   // single index
             {
               const APL_Integer I = cell_A.get_near_int();
               if (I < 0)   DOMAIN_ERROR;
               Value_P val = IntScalar(I, LOC);
              index_expr.add_index(val);
             }
       }

   if (index_expr.is_axis())   // Z←B[x] or Z←B[]
      {
        Value_P single_index = index_expr.extract_axis();
        Value_P Z = B.index(*single_index);

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }
   else                        // Z←B[x;...]
      {
        Value_P Z = B.index(index_expr);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }
}
//════════════════════════════════════════════════════════════════════════════
