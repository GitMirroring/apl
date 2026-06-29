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

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <complex>

#include "Common.hh"
#include "ComplexCell.hh"
#include "Error.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "UTF8_string.hh"
#include "Workspace.hh"

#include "Cell.icc"
#include "Value.hh"

//════════════════════════════════════════════════════════════════════════════
bool
FloatCell::greater(const Cell & other) const
{
const APL_Float this_val  = get_real_value();

   switch(other.get_cell_type())
      {
        case CT_INT:
             {
               const APL_Float other_val(other.get_int_value());
               if (this_val == other_val)   return this > &other;
               return this_val > other_val;
             }

        case CT_FLOAT:
             {
               const APL_Float other_val = other.get_real_value();
               if (this_val == other_val)   return this > &other;
               return this_val > other_val;
             }

        case CT_CHAR:    return true;
        case CT_COMPLEX: break;
        case CT_POINTER: return false;
        case CT_CELLREF: DOMAIN_ERROR;
        default:         Assert(0 && "Bad celltype");
      }

const Comp_result comp = compare(other);
   if (comp == COMP_EQ)   return this > &other;
   return (comp == COMP_GT);
}
//────────────────────────────────────────────────────────────────────────────
bool
FloatCell::equal(const Cell & A, double qct) const
{
   if (!A.is_numeric())       return false;
   if (A.is_complex_cell())   return A.equal(*this, qct);
   return tolerantly_equal(A.get_real_value(), get_real_value(), qct);
}
//────────────────────────────────────────────────────────────────────────────
// dyadic built-in functions...
//
// where possible a function with non-real A is delegated to the corresponding
// member function of A. For numeric cells that is the ComplexCell function
// and otherwise the default function (that returns E_DOMAIN_ERROR.
//
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_add(Cell * Z, const Cell * A) const
{
   if (A->is_real_cell())
      {
#ifdef cfg_RATIONAL_NUMBERS_WANTED
   if (const APL_Integer denom_B = get_denominator())
   if (const APL_Integer denom_A = A->get_denominator())
      {
        // both A and B are rational
        //
        if (Cell::prod_overflow(denom_A, denom_B))   goto big;

        // compute common denominator...
        const APL_Integer gcd_AB   = gcd(denom_A, denom_B);
        const APL_Integer mult_A  = denom_A / gcd_AB;
        const APL_Integer mult_B  = denom_B / gcd_AB;
        const APL_Integer denom_AB = denom_A * mult_B;
        if (Cell::prod_overflow(denom_AB, mult_B))                goto big;

        // compute numerators...
        const APL_Integer numer_A = A->get_numerator();
        if (Cell::prod_overflow(numer_A, mult_B))                goto big;
        const APL_Integer numer_A1 = numer_A * mult_B;
        const APL_Integer numer_B = get_numerator();
        if (Cell::prod_overflow(numer_B, mult_A))                goto big;
        const APL_Integer numer_B1 = numer_B * mult_A;

        const APL_Integer sum_AB = numer_A1 + numer_B1;
        if (Cell::sum_overflow(sum_AB, numer_A1, numer_B1))      goto big;
        const APL_Integer sum_gcd = gcd(sum_AB, denom_AB);
        if (sum_gcd == denom_AB)   return IntCell::zI(Z, sum_AB / denom_AB);
        if (sum_gcd == 1)   return FloatCell::zR(Z, sum_AB, denom_AB);
        return FloatCell::zR(Z, sum_AB/sum_gcd, denom_AB/sum_gcd);
      }
      big:

#endif
        return NumericCell::bif_add_ff(Z, A->get_real_value(), dfval());
      }

   if (A->is_complex_cell())
      return NumericCell::bif_add_cc(Z, A->get_complex_value(),
                                       APL_Complex(dfval(), 0));
   return E_DOMAIN_ERROR;
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_ceiling(Cell * Z) const
{
#ifdef cfg_RATIONAL_NUMBERS_WANTED
   if (const APL_Integer denom = get_denominator())
      {
        const APL_Integer numer = get_numerator();
        APL_Integer quotient = numer / denom;
        if (numer > (quotient * denom))   ++quotient;
        return IntCell::zI(Z, quotient);
      }
#endif
   return NumericCell::bif_ceiling_f(Z, dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_conjugate(Cell * Z) const
{
#ifdef cfg_RATIONAL_NUMBERS_WANTED
   // convert quotients to double
#endif
   return NumericCell::bif_conjugate_f(Z, dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_direction(Cell * Z) const
{
#ifdef cfg_RATIONAL_NUMBERS_WANTED
   if (const APL_Integer denom = get_denominator())
      {
        if (get_numerator() > 0)   return IntCell::zI(Z,  1);
        if (get_numerator() < 0)   return IntCell::zI(Z, -1);
        return FloatCell::zF(Z, 0);
      }
#endif
   return NumericCell::bif_direction_f(Z, dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_divide(Cell * Z, const Cell * A) const
{
#ifdef cfg_RATIONAL_NUMBERS_WANTED
   if (const APL_Integer B_denom = get_denominator())  // B is rational
      {
        const APL_Integer B_numer = get_numerator();
        if (B_numer == 0)   // A ÷ 0
           {
             if (A->is_near_zero())   return IntCell::z1(Z);
             return E_DOMAIN_ERROR;
           }
        const FloatCell inv_B(B_denom, B_numer);
        return inv_B.bif_multiply(Z, A);
      }
#endif

   if (!A->is_numeric())   return E_DOMAIN_ERROR;

   if (A->is_complex_cell())
      return NumericCell::bif_divide_cc(Z, A->get_complex_value(),
                                          APL_Complex(dfval(), 0));
   return NumericCell::bif_divide_ff(Z, A->get_real_value(), dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_exponential(Cell * Z) const
{
   return NumericCell::bif_exponential_f(Z, dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_factorial(Cell * Z) const
{
   return NumericCell::bif_factorial_f(Z, dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_floor(Cell * Z) const
{
#ifdef cfg_RATIONAL_NUMBERS_WANTED
   if (const APL_Integer denom = get_denominator())
      {
        const APL_Integer numer = get_numerator();
        APL_Integer quotient = numer / denom;
        if (numer < (quotient * denom))   --quotient;
        return IntCell::zI(Z, quotient);
      }
#endif
   return NumericCell::bif_floor_f(Z, dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_magnitude(Cell * Z) const
{
#ifdef cfg_RATIONAL_NUMBERS_WANTED
   if (const APL_Integer denom = get_denominator())
      {
        const APL_Integer numer = get_numerator();
        return FloatCell::zR(Z, numer < 0 ? -numer : numer, denom);
      }
#endif
   return NumericCell::bif_magnitude_f(Z, dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_multiply(Cell * Z, const Cell * A) const
{
#ifdef cfg_RATIONAL_NUMBERS_WANTED
   if (APL_Integer denom_B = get_denominator())
   if (APL_Integer denom_A = A->get_denominator())
      {
        APL_Integer numer_A = A->get_numerator();
        APL_Integer numer_B = get_numerator();
        const APL_Integer gcd_A_B = gcd(numer_A, denom_B);
         if (gcd_A_B > 1)   { numer_A /= gcd_A_B;   denom_B /= gcd_A_B; }

        const APL_Integer gcd_B_A = gcd(numer_B, denom_A);
         if (gcd_B_A > 1)   { numer_B /= gcd_B_A;   denom_A /= gcd_B_A; }

        if (Cell::prod_overflow(numer_A, numer_B))   goto big;
        if (Cell::prod_overflow(denom_A, denom_B))   goto big;

        const APL_Integer numer = numer_A * numer_B;
        const APL_Integer denom = denom_A * denom_B;
        const APL_Integer prod_gcd = gcd(numer, denom);
        if (prod_gcd == denom)   return IntCell::zI(Z, numer / denom);
        if (prod_gcd == 1)       return FloatCell::zR(Z, numer, denom);
        return FloatCell::zR(Z, numer/prod_gcd, denom/prod_gcd);
      }
      big:

#endif

   if (!A->is_numeric())   return E_DOMAIN_ERROR;

   if (A->is_complex_cell())
      return NumericCell::bif_multiply_cc(Z, A->get_complex_value(),
                                            APL_Complex(dfval(), 0));
   return NumericCell::bif_multiply_ff(Z, A->get_real_value(), dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_power(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;
   if (A->is_complex_cell())
      return NumericCell::bif_power_cc(Z, A->get_complex_value(),
                                         APL_Complex(dfval(), 0));
   return NumericCell::bif_power_ff(Z, A->get_real_value(), dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_nat_log(Cell * Z) const
{
   return NumericCell::bif_nat_log_f(Z, dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_negative(Cell * Z) const
{
#ifdef cfg_RATIONAL_NUMBERS_WANTED
   if (const APL_Integer denom = get_denominator())
      return FloatCell::zR(Z, -get_numerator(), denom);
#endif
   return NumericCell::bif_negative_f(Z, dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_pi_times(Cell * Z) const
{
   return NumericCell::bif_pi_times_f(Z, dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_pi_times_inverse(Cell * Z) const
{
   return NumericCell::bif_pi_times_inverse_f(Z, dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_reciprocal(Cell * Z) const
{
#ifdef cfg_RATIONAL_NUMBERS_WANTED
   if (const APL_Integer denom = get_denominator())
      {
        if (uint64_t(denom) < 0x8000000000000000ULL)
           {
             const APL_Integer numer = get_numerator();
             if (numer == 1)    return IntCell::zI(Z,  denom);
             if (numer == -1)   return IntCell::zI(Z, -denom);
             if (numer < 0)     return FloatCell::zR(Z, -denom, -numer);
             else               return FloatCell::zR(Z, denom, numer);
           }
      }
#endif
   return NumericCell::bif_reciprocal_f(Z, dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_roll(Cell * Z) const
{
   return NumericCell::bif_roll_f(Z, dfval());
}
//════════════════════════════════════════════════════════════════════════════
Comp_result
FloatCell::compare(const Cell & other) const
{
   if (other.is_integer_cell())   // integer
      {
        const double qct = Workspace::get_CT();
        if (equal(other, qct))   return COMP_EQ;
        return (dfval() <= other.get_int_value())  ? COMP_LT : COMP_GT;
      }

   if (other.is_float_cell())
      {
        const double qct = Workspace::get_CT();
        if (equal(other, qct))   return COMP_EQ;
        return (dfval() <= other.get_real_value()) ? COMP_LT : COMP_GT;
      }

   if (other.is_complex_cell())   return Comp_result(-other.compare(*this));

   if (other.is_character_cell())   return COMP_GT;   // numeric > char
   if (other.is_pointer_cell())     return COMP_LT;   // numeric < nested
   DOMAIN_ERROR;
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_maximum(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;
   if (A->is_complex_cell())
      return NumericCell::bif_maximum_cc(Z, A->get_complex_value(),
                                            APL_Complex(dfval(), 0));
   return NumericCell::bif_maximum_ff(Z, A->get_real_value(), dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_minimum(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;
   if (A->is_complex_cell())
      return NumericCell::bif_minimum_cc(Z, A->get_complex_value(),
                                            APL_Complex(dfval(), 0));
   return NumericCell::bif_minimum_ff(Z, A->get_real_value(), dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_residue(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;
   if (A->get_imag_value() != 0.0)
      return NumericCell::bif_residue_cc(Z, A->get_complex_value(),
                                            APL_Complex(dfval(), 0));
   return NumericCell::bif_residue_ff(Z, A->get_real_value(), dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_subtract(Cell * Z, const Cell * A) const
{
   if (A->is_complex_cell())
      return NumericCell::bif_subtract_cc(Z, A->get_complex_value(),
                                             APL_Complex(dfval(), 0));
   if (!A->is_real_cell())   return E_DOMAIN_ERROR;
#ifdef cfg_RATIONAL_NUMBERS_WANTED
   if (const APL_Integer denom_B = get_denominator())
   if (const APL_Integer denom_A = A->get_denominator())
      {
        if (Cell::prod_overflow(denom_A, denom_B))   goto big;
        const APL_Integer gcd_AB   = gcd(denom_A, denom_B);
        const APL_Integer mult_A  = denom_A / gcd_AB;
        const APL_Integer mult_B  = denom_B / gcd_AB;
        const APL_Integer denom_AB = denom_A * mult_B;
        const APL_Integer numer_A = A->get_numerator();
        if (Cell::prod_overflow(numer_A, mult_B))                goto big;
        const APL_Integer numer_A1 = numer_A * mult_B;
        const APL_Integer numer_B = get_numerator();
        if (Cell::prod_overflow(numer_B, mult_A))                goto big;
        const APL_Integer numer_B1 = numer_B * mult_A;
        const APL_Integer diff_AB = numer_A1 - numer_B1;
        if (Cell::diff_overflow(diff_AB, numer_A1, numer_B1))    goto big;
        const APL_Integer diff_gcd = gcd(diff_AB, denom_AB);
        if (diff_gcd == denom_AB)   return IntCell::zI(Z, diff_AB / denom_AB);
        if (diff_gcd == 1)   return FloatCell::zR(Z, diff_AB, denom_AB);
        return FloatCell::zR(Z, diff_AB/diff_gcd, denom_AB/diff_gcd);
      }
      big:
#endif
   return NumericCell::bif_subtract_ff(Z, A->get_real_value(), dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_add_inverse(Cell * Z, const Cell * A) const
{
   return A->bif_subtract(Z, this);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_multiply_inverse(Cell * Z, const Cell * A) const
{
   return A->bif_divide(Z, this);
}
//────────────────────────────────────────────────────────────────────────────
bool
FloatCell::get_near_bool()  const
{
   if (dfval() > INTEGER_TOLERANCE)   // maybe 1
      {
        if (dfval() > (1.0 + INTEGER_TOLERANCE))   DOMAIN_ERROR;
        if (dfval() < (1.0 - INTEGER_TOLERANCE))   DOMAIN_ERROR;
        return true;
      }

   // maybe 0. We already know that dfval() ≤ qct
   //
   if (dfval() < -INTEGER_TOLERANCE)   DOMAIN_ERROR;
   return false;
}
//────────────────────────────────────────────────────────────────────────────
// monadic built-in functions...
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_near_int64_t(Cell * Z) const
{
   return NumericCell::bif_near_int64_t_f(Z, dfval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
FloatCell::bif_within_quad_CT(Cell * Z) const
{
   return NumericCell::bif_within_quad_CT_f(Z, dfval());
}
/* ╔═════════════════════════════════════════════════════════════════════════╗
   ║ throw/nothrow boundary. Functions above MUST NOT (directly or           ║
   ║ indirectly) throw while funcions below MAY throw.                       ║
   ╚═════════════════════════════════════════════════════════════════════════╝
 */
PrintBuffer
FloatCell::character_representation(const PrintContext & pctx) const
{
#ifdef cfg_RATIONAL_NUMBERS_WANTED
   if (const APL_Integer denom = get_denominator())
      {
        if (Workspace::get_v_Quad_PS().get_print_quotients())   // show A÷B
           {
             ColInfo info;
             info.flags |= CT_FLOAT;

             UCS_string ucs;
             APL_Integer numer = get_numerator();
             if (numer < 0)
                {
                  ucs << UNI_OVERBAR;
                  numer = -numer;
                }
             ucs << UCS_string::from_uint(numer);
             info.int_len = ucs.size();

             ucs << UNI_DIVIDE;

             ucs << UCS_string::from_uint(denom);
             info.denom_len = ucs.size() - info.int_len;
             info.real_len = ucs.size();
             return PrintBuffer(ucs, info);
           }
      }
#endif

bool scaled = pctx.get_scaled();   // may be changed by print function
UCS_string ucs = UCS_string(dfval(), scaled, pctx);

ColInfo info;
   info.flags |= CT_FLOAT;
   if (scaled)   info.flags |= real_has_E;

   // assume integer.
   //
int int_fract = ucs.size();
   info.real_len = ucs.size();
   info.int_len = ucs.size();
   loop(u, ucs.size())
      {
        if (ucs[u] == UNI_FULLSTOP)
           {
             info.int_len = u;
             if (!pctx.get_scaled())   break;
             continue;
           }

        if (ucs[u] == UNI_E)
           {
             if (info.int_len > u)   info.int_len = u;
             int_fract = u;
             break;
           }
      }

   info.fract_len = int_fract - info.int_len;

   return PrintBuffer(ucs, info);
}
//────────────────────────────────────────────────────────────────────────────
bool
FloatCell::is_big(APL_Float val, int quad_pp)
{
static const APL_Float big[MAX_Quad_PP + 1] =
{
                  1ULL, // not used since MIN_Quad_PP == 1
                 10ULL,
                100ULL,
               1000ULL,
              10000ULL,
             100000ULL,
            1000000ULL,
           10000000ULL,
          100000000ULL,
         1000000000ULL,
        10000000000ULL,
       100000000000ULL,
      1000000000000ULL,
     10000000000000ULL,
    100000000000000ULL,
   1000000000000000ULL,
  10000000000000000ULL,
 100000000000000000ULL,
};

   return val >= big[quad_pp] || val <= -big[quad_pp];
}
//────────────────────────────────────────────────────────────────────────────
bool
FloatCell::need_scaling(APL_Float val, int quad_pp)
{
   // A number is printed in scaled format if (see lrm pp. 11-13) either:
   //
   // (1) its integer part is longer that quad-PP, or

   // (2a) is non-zero, and
   // (2b) its integer part is 0, and
   // (2c) its fractional part starts with at least 5 zeroes.
   //
   if (val < 0.0)   val = - val;   // simplify comparisons

   if (is_big(val, quad_pp))        return true;    // case 1.

   if (val == 0.0)                  return false;   // not 2a.

   if (val < 0.000001)              return true;    // case 2

   return false;
}
//────────────────────────────────────────────────────────────────────────────
void
FloatCell::map_FC(UCS_string & ucs)
{
   loop(u, ucs.size())
      {
        switch(ucs[u])
           {
             case UNI_FULLSTOP: ucs[u] = Workspace::get_FC(0);   break;
             case UNI_COMMA:    ucs[u] = Workspace::get_FC(1);   break;
             case UNI_OVERBAR:        ucs[u] = Workspace::get_FC(5);   break;
             default:                 break;
           }
      }
}
//════════════════════════════════════════════════════════════════════════════
