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

#include "Bif_F12_DOMINO.hh"
#include "Bif_F12_TAKE_DROP.hh"
#include "Bif_F1_EXECUTE.hh"
#include "Bif_OPER1_EACH.hh"
#include "Macro.hh"
#include "PointerCell.hh"
#include "UserFunction.hh"
#include "Workspace.hh"

Bif_OPER1_EACH Bif_OPER1_EACH::fun;

//════════════════════════════════════════════════════════════════════════════
Token
Bif_OPER1_EACH::eval_ALB(cValue_R A, Token & _LO, cValue_R B) const
{
   // dyadic EACH: call _LO for corresponding items of A and B

   if (!A.same_shape(B))
      {
        // if the shapes differ then either A or B must be a scalar or
        // 1-element vector.
        //
        if (A.get_rank() != B.get_rank())
           {
             if      (A.is_scalar_or_len1_vector())    ;   // OK
             else if (B.is_scalar_or_len1_vector())    ;   // OK
             else if (A.get_rank() != B.get_rank())
                {
                  MORE_ERROR() << "A f¨B: expecting ⍴⍴A = ⍴⍴B (or A or B a"
                                  " scalar/1-element vector); ⍴⍴A is "
                               << A.get_rank() << ", ⍴⍴B is " << B.get_rank();
                  RANK_ERROR;
                }
             else
                {
                  MORE_ERROR() << "A f¨B: expecting ⍴A = ⍴B (or A or B a"
                                  " scalar/1-element vector); ⍴A is "
                               << A.get_shape() << ", ⍴B is " << B.get_shape();
                  LENGTH_ERROR;
                }
           }
      }

cFunction_P LO = _LO.get_function();
   Assert1(LO);

   // for the ambiguous operators /. ⌿, \, and ⍀ is_operator() returns true,
   // which is incorrect in this context. We use get_signature() instead.
   //
   if ((LO->get_signature() & SIG_DYA) != SIG_DYA)   VALENCE_ERROR;

   if (A.is_empty() || B.is_empty())
      {
        if (!LO->has_result())   return Token(TOK_VOID);

        Value_P first_A = Bif_F12_TAKE::first(A);
        Value_P first_B = Bif_F12_TAKE::first(B);
        Shape shape_Z;   // will be ⍴A or ⍴B and therefore empty

        // is_scalar_or_len1_vector(), not is_scalar(): the general
        // (non-empty) conformance test a few lines above already accepts
        // a 1-element vector partner for scalar-extension, but this
        // empty-arg branch required a strict scalar -- ⍬+¨,5 gave
        // DOMAIN_ERROR (B=,5 is a 1-element vector, not scalar) though
        // the same B would extend fine against any non-empty A.
        if (A.is_empty())          shape_Z = A.get_shape();
        else if (!A.is_scalar_or_len1_vector())
           {
             MORE_ERROR() << "A f¨B: B is empty and A is not a scalar or"
                             " 1-element vector; ⍴A is " << A.get_shape();
             DOMAIN_ERROR;
           }

        if (B.is_empty())          shape_Z = B.get_shape();
        else if (!B.is_scalar_or_len1_vector())
           {
             MORE_ERROR() << "A f¨B: A is empty and B is not a scalar or"
                             " 1-element vector; ⍴B is " << B.get_shape();
             DOMAIN_ERROR;
           }

        // evaluate the fill function (lrm p. 245)
        //
        Value_P Z1;
        if (LO->is_defined() || LO->may_push_SI())
           {
             // the fill function of a defined function (or of a derived
             // function whose operand is defined, e.g. F⍤1 for a
             // ⎕FX-defined F) is the identity function, i.e. its right
             // argument -- calling eval_AB()/eval_B() on such an LO
             // pushes a real SI frame and returns TOK_SI_PUSHED, not a
             // value, so falling into the generic "else" branch below
             // returned that SI-pushed token as if it were Z1 (Bugs28
             // #35: (F⍤1)¨⍬ gave the scalar F 1 instead of an empty Z)
             //
             Z1 = first_B;
           }
        else if (LO->is_scalar_function())
           {
             // NOTE: even though eval_ALB is dyadic, we call the monadic
             //       fill function because it (and not the dyadic fill
             //       function handles the empty argument case).
             //
             Token tok_Z1 = LO->eval_fill_B(*(B.is_empty() ? first_B : first_A));
             if (tok_Z1.get_Class() != TC_VALUE)   return tok_Z1;
             Z1 = tok_Z1.get_apl_val();
           }
        else
           {
             // the fill function is the function itself
             //
             Token tok_Z1 = LO->eval_AB(*first_A, *first_B);
             if (tok_Z1.get_Class() != TC_VALUE)   return tok_Z1;
             Z1 = tok_Z1.get_apl_val();
           }

        Value_P Z(shape_Z, LOC);

        // Z1 is the prototype of the empty Z
        //
        if (Z1->is_simple_scalar())     // then ⊂Z1 is Z1
           {
             Cell cache;
             Z->set_ravel_Cell(0, Z1->get_cscalar(cache));
           }
        else   // need to enclose Z1
           {
             Z->set_ravel_Pointer(0, Z1.get());
           }

        Z->check_value(LOC);
        Z->to_type(true);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (LO->may_push_SI())   // user defined LO
      {
         const bool extend_A = A.is_scalar_or_len1_vector() && !B.is_scalar();
         const bool extend_B = B.is_scalar_or_len1_vector();

         Macro * macro = 0;
         if (LO->has_result())
            {
              if (extend_A)
                 {
                   macro = Macro::get_macro(extend_B
                                          ? Macro::MAC_Z__sA_LO_EACH_sB
                                          : Macro::MAC_Z__sA_LO_EACH_vB);
                }
             else
                {
                  macro = Macro::get_macro(extend_B
                                         ? Macro::MAC_Z__vA_LO_EACH_sB
                                         : Macro::MAC_Z__vA_LO_EACH_vB);
                }
            }
         else   // LO has no result, so we can ignore the shape of the result
            {
              if (extend_A && extend_B)
                 {
                   macro = Macro::get_macro(Macro::MAC_sA_LO_EACH_sB);
                 }
              else if (extend_B)
                 {
                   macro = Macro::get_macro(Macro::MAC_vA_LO_EACH_sB);
                 }
              else if (extend_A)
                 {
                   macro = Macro::get_macro(Macro::MAC_sA_LO_EACH_vB);
                 }
               else
                 {
                   macro = Macro::get_macro(Macro::MAC_vA_LO_EACH_vB);
                 }
            }

        if (extend_A && !A.is_scalar())        // 1-element non-scalar A
           {
             if (extend_B && !B.is_scalar())   // 1-element non-scalar B
                {
                  Value_P A1(LOC);   // A1 ← , A
                  Cell cache_A;
                  // was "A1->get_wscalar().init(...)": a non-static
                  // member call on get_wscalar()'s raw, not-yet-
                  // constructed storage -- UB per [basic.life], mirrors
                  // Bugs15 #12's Cell::init_type() finding. init_other()
                  // is the established pattern for this (called on the
                  // live source, destination passed as void*).
                  //
                  A.get_cscalar(cache_A).init_other(&A1->get_wscalar(),
                                                    *A1, LOC);
                  A1->check_value(LOC);

                  Value_P B1(LOC);   // B1 ← , B
                  Cell cache_B;
                  B.get_cscalar(cache_B).init_other(&B1->get_wscalar(),
                                                    *B1, LOC);
                  B1->check_value(LOC);

                  return macro->eval_ALB(*A1, _LO, *B1);
                }
             else
                {
                  Value_P A1(LOC);
                  Cell cache;
                  A.get_cfirst(cache).init_other(&A1->get_wscalar(),
                                                 *A1, LOC);
                  A1->check_value(LOC);

                  return macro->eval_ALB(*A1, _LO, B);
                }
           }
        else if (extend_B && !B.is_scalar())   // 1-element non-scalar B
           {
             Value_P B1(LOC);
             Cell cache;
             B.get_cfirst(cache).init_other(&B1->get_wscalar(), *B1, LOC);
             B1->check_value(LOC);

             return macro->eval_ALB(A, _LO, *B1);
           }
        else
           {
             return macro->eval_ALB(A, _LO, B);
           }
      }


   // use the same scheme as ScalarFunction::do_scalar_AB() to determine
   // the shape of the result. In order to detect conformity errors we compute
   // shape_Z even if no result is returned.
   //
const int inc_A = A.get_increment();
const int inc_B = B.get_increment();

const Shape * shape_Z = 0;
   if      (A.is_scalar())      shape_Z = &B.get_shape();
   else if (B.is_scalar())      shape_Z = &A.get_shape();
   else if (inc_A == 0)          shape_Z = &B.get_shape();
   else if (inc_B == 0)          shape_Z = &A.get_shape();
   else if (A.same_shape(B))   shape_Z = &B.get_shape();
   else   // error
      {
        if (!A.same_rank(B))
           {
             MORE_ERROR() << "A f¨B: expecting ⍴⍴A = ⍴⍴B (or A or B a"
                             " scalar); ⍴⍴A is " << A.get_rank()
                          << ", ⍴⍴B is " << B.get_rank();
             RANK_ERROR;
           }
        else
           {
             MORE_ERROR() << "A f¨B: expecting ⍴A = ⍴B (or A or B a"
                             " scalar); ⍴A is " << A.get_shape()
                          << ", ⍴B is " << B.get_shape();
             LENGTH_ERROR;
           }
      }

const ShapeItem len_Z = shape_Z->get_volume();
Value_P Z;
   if (LO->has_result())   Z = Value_P(*shape_Z, LOC);

   loop(z, len_Z)
      {
        Cell cache_A, cache_B;
        const Cell & cA = A.get_cravel(inc_A * z, cache_A);
        const Cell & cB = B.get_cravel(inc_B * z, cache_B);
        const bool left_val = cB.is_lval_cell();
        Value_P LO_A = cA.to_value(LOC);     // left argument of LO
        Value_P LO_B = cB.to_value(LOC);     // right argument of LO;
        if (left_val)
           {
             Cell * dest = cB.get_lval_value();
             if (dest->is_pointer_cell())
                {
                  Value_P sub = dest->get_pointer_value();
                  LO_B = sub->get_cellrefs(LOC);
                }
           }

        Token result = LO->eval_AB(*LO_A, *LO_B);

        // if LO was a primitive function, then result may be a value.
        // if LO was a user defined function then result may be TOK_SI_PUSHED.
        // in both cases result could be TOK_ERROR.
        //
        if (result.get_Class() == TC_VALUE)
           {
             Value_P vZ = result.get_apl_val();

             if (!Z)
                ;   // LO without result: nothing to store
             else if (vZ->is_simple_scalar() || (left_val && vZ->is_scalar()))
                {
                  Cell cache;
                  Z->next_ravel_Cell(vZ->get_cfirst(cache));
                }
             else
                Z->next_ravel_Pointer(vZ.get());

             continue;   // next z
           }

        if (result.get_tag() == TOK_VOID)   continue;   // next z: LO without result

        if (result.get_tag() == TOK_ERROR)   return result;

        Q1(result);   FIXME;
      }

   if (!Z)   return Token(TOK_VOID);   // LO without result

   Z->set_default(B, LOC);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//────────────────────────────────────────────────────────────────────────────
Token
Bif_OPER1_EACH::do_eval_LB(Token & _LO, cValue_R B)
{
   // monadic EACH: call _LO for every item of B

cFunction_P LO = _LO.get_function();
   Assert1(LO);

   if (LO->is_operator() &&
       !_LO.is_SLASH_or_BACKSLASH())     SYNTAX_ERROR;
   if (!(LO->get_signature() & SIG_B))   VALENCE_ERROR;

   if (B.is_empty())
      {
        if (!LO->has_result())   return Token(TOK_VOID);    // no-op

        Value_P first_B = Bif_F12_TAKE::first(B);

        // evaluate the fill function (lrm p. 245)
        //
        Value_P Z1;
        if (LO != &Quad_EC::fun && (LO->is_defined() || LO->may_push_SI()))
           {
             // the fill function of a defined function (or of a derived
             // function whose operand is defined, e.g. F⍤1 for a
             // ⎕FX-defined F) is the identity function, i.e. its right
             // argument -- see the dyadic branch above for the full
             // explanation (Bugs28 #35). ⎕EC itself is excluded: it also
             // may_push_SI(), but has its own dedicated fill value
             // fabricated below rather than being its own identity.
             //
             Z1 = first_B;
           }
        else if (LO->is_scalar_function() || (LO == &Bif_F12_DOMINO::fun))
           {
             Token tok_Z1 = LO->eval_fill_B(*first_B);
             if (tok_Z1.get_Class() != TC_VALUE)   return tok_Z1;
             Z1 = tok_Z1.get_apl_val();
           }
        else if (LO == &Quad_EC::fun)
           {
             // fake Z1←⎕EC ''  ←→  3 (0 0) (0 0⍴0)
             //
             if (!B.is_character_cell(0))
                {
                  MORE_ERROR() << "⎕EC¨B: B's prototype is not a character"
                                  " (needed to fabricate ⎕EC's fill value)";
                  DOMAIN_ERROR;
                }
             Z1 = Value_P(3, LOC);
             Z1->next_ravel_Number(3);
             {
               Value_P sub1(2, LOC);
               sub1->next_ravel_0();
               sub1->next_ravel_0();
               sub1->check_value(LOC);
               Z1->next_ravel_Pointer(sub1.get());
             }
             Value_P sub2 = Idx0_0(LOC);
             Z1->next_ravel_Pointer(sub2.get());
           }
        else
           {
             // the fill function is the function itself
             //
             Token tok_Z1 = LO->eval_B(*first_B);
             if (tok_Z1.get_Class() != TC_VALUE)   return tok_Z1;
             Z1 = tok_Z1.get_apl_val();
           }

        Value_P Z(B.get_shape(), LOC);

        // Z1 is the prototype of the empty Z
        //
        if (Z1->is_simple_scalar())     // then ⊂Z1 is Z1
           {
             Cell cache;
             Z->set_ravel_Cell(0, Z1->get_cscalar(cache));
           }
        else                            // need to encose Z1
           Z->set_ravel_Pointer(0, Z1.get());

        Z->check_value(LOC);
        Z->to_type(true);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (LO->may_push_SI())   // user defined LO
      {
        if (!LO->has_result())
           return Macro::get_macro(Macro::MAC_LO_EACH_B)->eval_LB(_LO, B);

        if (LO == &Bif_F1_EXECUTE::fun)
           return Macro::get_macro(Macro::MAC_Z__EXEC_EACH_B)->eval_B(B);

        return Macro::get_macro(Macro::MAC_Z__LO_EACH_B)->eval_LB(_LO, B);
      }

const ShapeItem len_Z = B.element_count();
Value_P Z;
   if (LO->has_result())   Z = Value_P(B.get_shape(), LOC);

   loop (z, len_Z)
      {
        if (LO->get_fun_valence() == 0)
           {
             // we allow niladic functions N so that one can simply loop
             // over them with N ¨ 1 2 3 4
             //
             Token result = LO->eval_();

             if (result.get_Class() == TC_VALUE)
                {
                  Value_P vZ = result.get_apl_val();
                  if (!Z)
                     ;   // LO without result: nothing to store
                  else if (vZ->is_simple_scalar())
                     {
                       Cell cache;
                       Z->next_ravel_Cell(vZ->get_cfirst(cache));
                     }
                  else
                     Z->next_ravel_Pointer(vZ.get());

                  continue;   // next z
           }

             if (result.get_tag() == TOK_VOID)   continue;   // next z

             if (result.get_tag() == TOK_ERROR)   return result;

             Q1(result);   FIXME;
           }
        else
           {
             Cell cache;
             const Cell & cB = B.get_cravel(z, cache);
             const bool is_left_val = cB.is_lval_cell();
             Value_P LO_B = cB.to_value(LOC);      // right argument of LO

             if (is_left_val)
                {
                  Cell * dest = cB.get_lval_value();
                  // target can be 0! (Value.cc:502's own comment) -- e.g.
                  // a selective assignment through an over-take of an
                  // empty vector, (,¨1↑⍬)←,1: dereferencing it
                  // unconditionally segfaults instead of a clean error.
                  if (dest == 0)   INDEX_ERROR;
                  if (dest->is_pointer_cell())
                     {
                       Value_P sub = dest->get_pointer_value();
                       LO_B = sub->get_cellrefs(LOC);
                     }
                }

             Token result = LO->eval_B(*LO_B);
             if (result.get_Class() == TC_VALUE)
                {
                  Value * vZ = result.get_apl_val().get();

                  if (!Z)
                     ;   // LO without result: nothing to store
                  else if (vZ->is_simple_scalar() ||
                           (is_left_val && vZ->is_scalar()))
                     {
                       Cell cache;
                       Z->next_ravel_Cell(vZ->get_cfirst(cache));
                     }
                  else
                     Z->next_ravel_Pointer(vZ);

                  continue;   // next z
                }

             if (result.get_tag() == TOK_VOID)   continue;   // next z

             if (result.get_tag() == TOK_ERROR)   return result;

             Q1(result);   Q1(*LO) FIXME;
           }
      }

   if (!Z)   return Token(TOK_VOID);   // LO without result

   Z->set_default(B, LOC);
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//════════════════════════════════════════════════════════════════════════════

