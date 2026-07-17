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

#include "Bif_OPER1_REDUCE.hh"
#include "Bif_OPER1_SCAN.hh"
#include "LvalCell.hh"
#include "Macro.hh"
#include "Workspace.hh"

Bif_OPER1_SCAN    Bif_OPER1_SCAN ::fun;
Bif_OPER1_SCAN1   Bif_OPER1_SCAN1::fun;

//════════════════════════════════════════════════════════════════════════════
Token
Bif_SCAN::scan(Token & tok_LO, Value_P B, uAxis axis) const
{
   // if B is a scalar, then Z is B.
   //
   if (B->get_rank() == 0)      return Token(TOK_APL_VALUE1, CLONE_P(B, LOC));

   if (!tok_LO.is_function())
      {
        MORE_ERROR() << "f" << get_name() << " B : Bad left argument f"
                        " (expecting a function)";
        DOMAIN_ERROR;
      }

cFunction_P LO = tok_LO.get_function();

   if (!LO->has_result())
      {
        MORE_ERROR() << LO->get_name() << get_name() << " B : Bad function "
                     << LO->get_name() << " (expecting a result)";
        DOMAIN_ERROR;
      }

   if (LO->get_fun_valence() != 2)
      {
        MORE_ERROR() << LO->get_name() << get_name() << " B : Bad function "
                     << LO->get_name() << " (expecting a dyadic function)";
        SYNTAX_ERROR;
      }

   if (axis >= B->get_rank())   AXIS_ERROR;

const ShapeItem m_len = B->get_shape_item(axis);

   if (m_len == 0)      return Token(TOK_APL_VALUE1, CLONE_P(B, LOC));

   if (m_len == 1)
      {
        const Shape shape_Z = B->get_shape().without_axis(axis);
        return Bif_F12_RHO::do_reshape(shape_Z, *B);
      }

const Shape3 shape_Z3(B->get_shape(), axis);

Value_P Z(B->get_shape(), LOC);
ErrorCode (Cell::*assoc_f2)(Cell *, const Cell *) const = LO->get_assoc();

   if (assoc_f2)
      {
        // LO is an associative primitive scalar function.
        //
        ShapeItem bI = 0;
        ShapeItem z = 0;
        loop(h, shape_Z3.h())
        loop(m, shape_Z3.m())
        loop(l, shape_Z3.l())
            {
              if (m == 0)   // first item in scanned vector
                 {
                   Z->next_ravel_Cell(B->get_cravel(bI++));
                 }
              else          // subsequent item in scanned vector
                 {
                   const Cell & prev_Z = Z->get_cravel(z - shape_Z3.l());

                   Value_P AA(prev_Z, LOC);              // AA is Z[h; m-1; l]
                   Value_P BB(B->get_cravel(bI++), LOC); // BB is B[h; m  ; l]

                   Token tok = LO->eval_AB(*AA, *BB);
                   if (!tok.is_apl_val())   return tok;   // error in AA LO BB

                   Z->next_ravel_Cell(tok.get_apl_val()->get_cscalar());
                 }
              ++z;
            }

        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   // non-trivial reduce (len > 1)
   //
const Shape3 Z3(B->get_shape(), axis);
   if (LO->may_push_SI())   // user defined LO
      {
        Value_P X4(4, LOC);
        X4->next_ravel_Int(axis + Workspace::get_IO());
        X4->next_ravel_Int(Z3.h());
        X4->next_ravel_Int(Z3.m());
        X4->next_ravel_Int(Z3.l());
        X4->check_value(LOC);
        return Macro::get_macro(Macro::MAC_Z__LO_SCAN_X4_B)
                                ->eval_LXB(tok_LO, *X4, *B);
      }

   if (B->get_shape().is_empty())
      {
        // scan preserves B's shape (unlike reduce, which removes the
        // axis), and B being empty means there are no cells to compute
        // -- so unlike the reduce identity-fun path, there's no need to
        // consult LO's identity element (and no risk of a spurious
        // DOMAIN_ERROR for an LO that doesn't have one).
        //
        Value_P Z(B->get_shape(), LOC);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   return Bif_REDUCE::do_reduce(B->get_shape(), Z3, -1, LO, B, m_len);
}
//════════════════════════════════════════════════════════════════════════════
Token
Bif_SCAN::expand(cValue_R A, cValue_R B, uAxis axis)
{
   // turn scalar B into ,B
   //
Shape shape_B = B.get_shape();
   if (shape_B.get_rank() == 0)
      {
         shape_B.add_shape_item(1);
         axis = 0;
      }
   if (axis >= shape_B.get_rank())   INDEX_ERROR;

   if (A.get_rank() > 1)            RANK_ERROR;
   if (shape_B.get_rank() <= axis)   RANK_ERROR;

const ShapeItem ec_A = A.element_count();
ShapeItem ones_A = 0;
std::vector<ShapeItem> rep_counts;
   rep_counts.reserve(ec_A);
   loop(a, ec_A)
      {
        APL_Integer rep_A = A.get_near_int(a);
        rep_counts.push_back(rep_A);
        if      (rep_A == 0)        ;
        else if (rep_A == 1)        ++ones_A;
        else                        DOMAIN_ERROR;
      }

Shape shape_Z(shape_B);
   shape_Z.set_shape_item(axis, ec_A);
Value_P Z(shape_Z, LOC);

   if (ec_A == 0)   // (⍳0)/B : 
      {
        if (shape_B.get_shape_item(axis) > 1)   LENGTH_ERROR;

        Z->set_default(B, LOC);
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

const Shape3 shape_Z3(shape_Z, axis);

const bool lval = B.is_lval_cell(0);

ShapeItem inc_1 = shape_Z3.l();   // increment after result l items
ShapeItem inc_2 = 0;              // increment after result m*l items

   if (B.is_scalar() || (shape_B.get_shape_item(axis) == 1
                      && (shape_Z3.l() != 1)))
      {
         inc_1 = 0;
         inc_2 = shape_Z3.l();
      }
   else if (ones_A != shape_B.get_shape_item(axis))   LENGTH_ERROR;

ShapeItem bI = 0;
   loop(h, shape_Z3.h())
      {
        const ShapeItem fillI = bI;
        loop(m, rep_counts.size())
           {
             if (rep_counts[m] == 1)   // copy items from B
                {
                  loop(l, shape_Z3.l())   Z->next_ravel_Cell(B.get_cravel(bI + l));
                  bI += inc_1;
                }
             else                      // init items
                {
                  if (lval)
                     {
                       loop(l, shape_Z3.l())   Z->next_ravel_Lval(0, 0);
                     }
                  else
                     {
                       loop(l, shape_Z3.l())   Z->next_ravel_Proto(B.get_cravel(fillI + l));
                     }
                }
           }

        bI += inc_2;
      }

   Z->set_default(B, LOC);

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//════════════════════════════════════════════════════════════════════════════
Token
Bif_OPER1_SCAN::eval_AXB(cValue_R A, cValue_R X, cValue_R B) const
{
const sAxis axis = Value::get_single_axis(&X, B.get_rank());
   return expand(A, B, axis);
}
//════════════════════════════════════════════════════════════════════════════
Token
Bif_OPER1_SCAN::eval_LXB(Token & LO, cValue_R X, cValue_R B) const
{
const sAxis axis = Value::get_single_axis(&X, B.get_rank());
   return scan(LO, CLONE(&B, LOC), axis);
}
//════════════════════════════════════════════════════════════════════════════
Token
Bif_OPER1_SCAN1::eval_AXB(cValue_R A, cValue_R X, cValue_R B) const
{
const sAxis axis = Value::get_single_axis(&X, B.get_rank());
   return expand(A, B, axis);
}
//════════════════════════════════════════════════════════════════════════════
Token
Bif_OPER1_SCAN1::eval_LXB(Token & LO, cValue_R X, cValue_R B) const
{
const sAxis axis = Value::get_single_axis(&X, B.get_rank());
   return scan(LO, CLONE(&B, LOC), axis);
}
//════════════════════════════════════════════════════════════════════════════
