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

#include <errno.h>
#include <fenv.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "Common.hh"
#include "ComplexCell.hh"
#include "ErrorCode.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "Output.hh"
#include "Workspace.hh"

#include "Cell.icc"
#include "NumericCell.icc"

//════════════════════════════════════════════════════════════════════════════
ComplexCell::ComplexCell(APL_Complex c)
{
   value.cval[0]  = c.real();
   value.cval[1] = c.imag();
}
//────────────────────────────────────────────────────────────────────────────
ComplexCell::ComplexCell(APL_Float r, APL_Float i)
{
   value.cval[0] = r;
   value.cval[1] = i;
}
//────────────────────────────────────────────────────────────────────────────
bool
ComplexCell::equal(const Cell & A, double qct) const
{
   if (!A.is_numeric())   return false;
   return tolerantly_equal(A.get_complex_value(), get_complex_value(), qct);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_ceiling(Cell * Z) const
{
   return NumericCell::bif_ceiling_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_conjugate(Cell * Z) const
{
   return NumericCell::bif_conjugate_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_direction(Cell * Z) const
{
   return NumericCell::bif_direction_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_exponential(Cell * Z) const
{
   return NumericCell::bif_exponential_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
// monadic build-in functions...
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_factorial(Cell * Z) const
{
   return NumericCell::bif_factorial_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_floor(Cell * Z) const
{
   return NumericCell::bif_floor_c(Z, cval());
}
//════════════════════════════════════════════════════════════════════════════
ErrorCode
ComplexCell::bif_magnitude(Cell * Z) const
{
   return NumericCell::bif_magnitude_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_nat_log(Cell * Z) const
{
   return NumericCell::bif_nat_log_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_negative(Cell * Z) const
{
   return NumericCell::bif_negative_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_pi_times(Cell * Z) const
{
   return NumericCell::bif_pi_times_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_pi_times_inverse(Cell * Z) const
{
   return NumericCell::bif_pi_times_inverse_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_reciprocal(Cell * Z) const
{
   return NumericCell::bif_reciprocal_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_roll(Cell * Z) const
{
   return NumericCell::bif_roll_c(Z, cval());
}
//════════════════════════════════════════════════════════════════════════════
// dyadic build-in functions...
//════════════════════════════════════════════════════════════════════════════
ErrorCode
ComplexCell::bif_add(Cell * Z, const Cell * A) const
{
   return NumericCell::bif_add_cc(Z, A->get_complex_value(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_subtract(Cell * Z, const Cell * A) const
{
   return NumericCell::bif_subtract_cc(Z, A->get_complex_value(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_divide(Cell * Z, const Cell * A) const
{
   return NumericCell::bif_divide_cc(Z, A->get_complex_value(), cval());
}
//════════════════════════════════════════════════════════════════════════════
ErrorCode
ComplexCell::bif_logarithm(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;
   return NumericCell::bif_logarithm_cc(Z, A->get_complex_value(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_multiply(Cell * Z, const Cell * A) const
{
   return NumericCell::bif_multiply_cc(Z, A->get_complex_value(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_power(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;
   return NumericCell::bif_power_cc(Z, A->get_complex_value(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_maximum(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;
   return NumericCell::bif_maximum_cc(Z, A->get_complex_value(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_minimum(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;
   return NumericCell::bif_minimum_cc(Z, A->get_complex_value(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_residue(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;
   return NumericCell::bif_residue_cc(Z, A->get_complex_value(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_circle_fun(Cell * Z, const Cell * A) const
{
   if (!A->is_near_int())   return E_DOMAIN_ERROR;
   return NumericCell::bif_circle_fun_c(Z, A->get_checked_near_int(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_circle_fun_inverse(Cell * Z, const Cell * A) const
{
   if (!A->is_near_int())   return E_DOMAIN_ERROR;
   return NumericCell::bif_circle_fun_inverse_c(Z, A->get_checked_near_int(),
                                                   cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_add_inverse(Cell * Z, const Cell * A) const
{
   return A->bif_subtract(Z, this);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_multiply_inverse(Cell * Z, const Cell * A) const
{
   return A->bif_divide(Z, this);
}
//────────────────────────────────────────────────────────────────────────────
APL_Complex
ComplexCell::gamma(APL_Float x, const APL_Float & y)
{
const APL_Complex pi(M_PI, 0);
   if (x < 0.5)
      return pi / (sin(pi * APL_Complex(x, y)) * gamma(1.0 - x, -y));

   // coefficients for lanczos approximation of the gamma function.
   //
#define p0 APL_Complex(  1.000000000190015   )
#define p1 APL_Complex( 76.18009172947146    )
#define p2 APL_Complex(-86.50532032941677    )
#define p3 APL_Complex( 24.01409824083091    )
#define p4 APL_Complex( -1.231739572450155   )
#define p5 APL_Complex(  1.208650973866179E-3)
#define p6 APL_Complex( -5.395239384953E-6   )

   errno = 0;
   feclearexcept(FE_ALL_EXCEPT);

const APL_Complex z(x, y);
const APL_Complex z1(x + 5.5, y);
const APL_Complex z2(x + 0.5, y);

const APL_Complex ret( (complex_sqrt(APL_Complex(2*M_PI)) / z)
                      * (p0                           +
                         p1 / APL_Complex(x + 1.0, y) +
                         p2 / APL_Complex(x + 2.0, y) +
                         p3 / APL_Complex(x + 3.0, y) +
                         p4 / APL_Complex(x + 4.0, y) +
                         p5 / APL_Complex(x + 5.0, y) +
                         p6 / APL_Complex(x + 6.0, y))
                       * complex_power(z1, z2)
                       * complex_exponent(-z1)
                     );

   if (!isfinite(ret.real()))   DOMAIN_ERROR;
   if (!isfinite(ret.imag()))   DOMAIN_ERROR;
   return ret;

#undef p0
#undef p1
#undef p2
#undef p3
#undef p4
#undef p5
#undef p6
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::do_bif_circle_fun(Cell * Z, int fun, APL_Complex b)
{
const APL_Complex one(1.0, 0.0);
   switch(fun)
      {
        case -12: {
                    ComplexCell cb(-b.imag(), b.real());
                    cb.bif_exponential(Z);
                  }
                  return E_NO_ERROR;

        case -11: return ComplexCell::zC(Z, -b.imag(), b.real());

        case -10: return ComplexCell::zC(Z, b.real(), -b.imag());

        case  -9: return ComplexCell::zC(Z, b);
        case  -8: // ¯8○Z ←→ -8○Z
                  do_bif_circle_fun(Z, 8, b);
                  return ComplexCell::zC(Z, -Z->get_complex_value());

        case  -7: // arctanh(z) = 0.5 (ln(1.0 + z) - ln(1.0 - z))
                  {
                    const APL_Complex b1      = ONE() + b;
                    const APL_Complex b_1     = ONE() - b;
                    const APL_Complex log_b1  = log(b1);
                    const APL_Complex log_b_1 = log(b_1);
                    const APL_Complex diff    = log_b1 - log_b_1;
                    const APL_Complex half(diff.real()*0.5, diff.imag()*0.5);
                    return ComplexCell::zC(Z, half);
                  }

        case  -6: // arccosh(z) = ln(z + sqrt(z + 1) sqrt(z - 1))
                  {
                    const APL_Complex b1     = b + ONE();
                    const APL_Complex b_1    = b - ONE();
                    const APL_Complex root1  = complex_sqrt(b1);
                    const APL_Complex root_1 = complex_sqrt(b_1);
                    const APL_Complex prod   = root1 * root_1;
                    const APL_Complex sum    = b + prod;
                    const APL_Complex loga   = log(sum);
                    return ComplexCell::zC(Z, loga);
                  }

        case  -5: // arcsinh(z) = ln(z + sqrt(z^2 + 1))
                  {
                    const APL_Complex b2 = b*b;
                    const APL_Complex b2_1 = b2 + ONE();
                    const APL_Complex root = complex_sqrt(b2_1);
                    const APL_Complex sum =  b + root;
                    const APL_Complex loga = log(sum);
                    return ComplexCell::zC(Z, loga);
                  }

        case  -4: if (b.real() >= 0.0 ||
                      (b.real() > -1.0 && Cell::is_near_zero(b.imag()))
                     )   return ComplexCell::zC(Z,  complex_sqrt(b*b - one));
                  else   return ComplexCell::zC(Z, -complex_sqrt(b*b - one));

        case  -3: // arctan(z) = i/2 (ln(1 - iz) - ln(1 + iz))
                  {
                    const APL_Complex iz = APL_Complex(- b.imag(), b.real());
                    const APL_Complex piz = ONE() + iz;
                    const APL_Complex niz = ONE() - iz;
                    const APL_Complex log_piz = log(piz);
                    const APL_Complex log_niz = log(niz);
                    const APL_Complex diff = log_niz - log_piz;
                    const APL_Complex prod = APL_Complex(0, 0.5) * diff;
                    return ComplexCell::zC(Z, prod);
                  }

        case  -2: // arccos(z) = -i (ln( z + sqrt(z^2 - 1)))
                  {
                    const APL_Complex b2 = b*b;
                    const APL_Complex diff = b2 - ONE();
                    const APL_Complex root = complex_sqrt(diff);
                    const APL_Complex sum = b + root;
                    const APL_Complex loga = log(sum);
                    const APL_Complex prod = MINUS_i() * loga;
                    return ComplexCell::zC(Z, prod);
                  }

        case  -1: // arcsin(z) = -i (ln(iz + sqrt(1 - z^2)))
                  {
                    const APL_Complex b2 = b*b;
                    const APL_Complex diff = ONE() - b2;
                    const APL_Complex root = complex_sqrt(diff);
                    const APL_Complex sum  = APL_Complex(-b.imag(), b.real())
                                           + root;
                    const APL_Complex loga = log(sum);
                    const APL_Complex prod = MINUS_i() * loga;
                    return ComplexCell::zC(Z, prod);
                  }

        case   0: return ComplexCell::zC(Z, complex_sqrt(one - b*b));

        case   1: return ComplexCell::zC(Z, sin(b));

        case   2: return ComplexCell::zC(Z, cos(b));

        case   3: return ComplexCell::zC(Z, tan(b));

        case   4: return ComplexCell::zC(Z, complex_sqrt(one + b*b));

        case   5: return ComplexCell::zC(Z, sinh(b));

        case   6: return ComplexCell::zC(Z, cosh(b));

        case   7: return ComplexCell::zC(Z, tanh(b));

        case   8: { const APL_Complex b2 = b*b;
                    const APL_Complex square =                    // (¯1 - R⋆2)
                                      APL_Complex(-1, 0) - b2;
                    const APL_Complex root = complex_sqrt(square);
                    if (b.real()  > 0.0)
                       {
                         if (b.imag() > 0.0)   return ComplexCell::zC(Z,  root);
                         else                  return ComplexCell::zC(Z, -root);
                       }
                   else if (b.real() == 0.0)
                       {
                         if (b.imag() > 1.0)   return ComplexCell::zC(Z,  root);
                         else                  return ComplexCell::zC(Z, -root);
                       }
                   else   // b.real() < 0,0;
                       {
                         if (b.imag() >= 0.0)  return ComplexCell::zC(Z,  root);
                         else                  return ComplexCell::zC(Z, -root);
                       }
                  }

        case   9: return ComplexCell::zC(Z, b.real());

        case  10: return ComplexCell::zC(Z, sqrt(mag2(b)));

        case  11: return ComplexCell::zC(Z, b.imag());

        case  12: return ComplexCell::zC(Z, atan2(b.imag(), b.real()));
      }

   // invalid fun
   //
   return E_DOMAIN_ERROR;
}
//────────────────────────────────────────────────────────────────────────────
APL_Float
ComplexCell::get_real_value() const
{
   return value.cval[0];
}
//────────────────────────────────────────────────────────────────────────────
APL_Float
ComplexCell::get_imag_value() const
{
   return value.cval[1];
}
//────────────────────────────────────────────────────────────────────────────
bool
ComplexCell::is_near_int() const
{
   return Cell::is_near_int(value.cval[0]) &&
          Cell::is_near_int(value.cval[1]);
}
//────────────────────────────────────────────────────────────────────────────
bool
ComplexCell::is_near_int64_t() const
{
   return Cell::is_near_int64_t(value.cval[0]) &&
          Cell::is_near_int64_t(value.cval[1]);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_near_int64_t(Cell * Z) const
{
   return NumericCell::bif_near_int64_t_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_within_quad_CT(Cell * Z) const
{
   return NumericCell::bif_within_quad_CT_c(Z, cval());
}
//════════════════════════════════════════════════════════════════════════════
bool
ComplexCell::is_near_zero() const
{
   if (value.cval[0]  >=  INTEGER_TOLERANCE)   return false;
   if (value.cval[0]  <= -INTEGER_TOLERANCE)   return false;
   if (value.cval[1] >=  INTEGER_TOLERANCE)    return false;
   if (value.cval[1] <= -INTEGER_TOLERANCE)    return false;
   return true;
}
//────────────────────────────────────────────────────────────────────────────
bool
ComplexCell::is_near_one() const
{
   if (value.cval[0] >  (1.0 + INTEGER_TOLERANCE))   return false;
   if (value.cval[0] <  (1.0 - INTEGER_TOLERANCE))   return false;
   if (value.cval[1] >  INTEGER_TOLERANCE)           return false;
   if (value.cval[1] < -INTEGER_TOLERANCE)           return false;
   return true;
}
//────────────────────────────────────────────────────────────────────────────
bool
ComplexCell::is_near_real() const
{
const APL_Float B2 = REAL_TOLERANCE*REAL_TOLERANCE;
const APL_Float I2 = value.cval[1] * value.cval[1];

   if (I2 < B2)     return true;   // I is absolutely small

const APL_Float R2 = value.cval[0] * value.cval[0];
   return (I2 < R2*B2);   // I is relatively small
}
//────────────────────────────────────────────────────────────────────────────
// throw/nothrow boundary. Functions above MUST NOT (directly or indirectly)
// throw while funcions below MAY throw.
//────────────────────────────────────────────────────────────────────────────

#include "Error.hh"   // throws

//────────────────────────────────────────────────────────────────────────────
bool
ComplexCell::greater(const Cell & other) const
{
   switch(other.get_cell_type())
      {
        case CT_CHAR:    return true;
        case CT_INT:
        case CT_FLOAT:
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
Comp_result
ComplexCell::compare(const Cell & other) const
{
   if (other.is_character_cell())   return COMP_GT;   // numeric > char
   if (other.is_pointer_cell())     return COMP_LT;   // numeric < nested
   if (!other.is_numeric())   DOMAIN_ERROR;

const double qct = Workspace::get_CT();
   if (equal(other, qct))   return COMP_EQ;

APL_Float areal = other.get_real_value();
APL_Float breal = get_real_value();

   // we compare complex numbers by their real part unless the real parsts
   // are equal. In that case we compare the imag parts. The reason for
   // comparing this way is compatibility with the comparison of real numbers
   //
   if (tolerantly_equal(areal, breal, qct))
      {
        areal = other.get_imag_value();
        breal = get_imag_value();
      }

   return (breal < areal) ? COMP_LT : COMP_GT;
}
//════════════════════════════════════════════════════════════════════════════
PrintBuffer
ComplexCell::character_representation(const PrintContext & pctx) const
{
   if (pctx.get_PP() < MAX_Quad_PP)
      {
         // 10⋆get_PP()
         //
         APL_Float ten_to_PP = 1.0;
         loop(p, pctx.get_PP())   ten_to_PP = ten_to_PP * 10.0;

         // lrm p. 13: In J notation, the real or imaginary part is not
         // displayed if it is less than the other by more than ⎕PP orders
         // of magnitude (unless ⎕PP is at its maximum).
         //
         const APL_Float pos_real = value.cval[0] < 0.0
                                  ? -value.cval[0] : value.cval[0];
         const APL_Float pos_imag = value.cval[1] < 0.0
                                  ? -value.cval[1] : value.cval[1];

         if (pos_real >= pos_imag)   // pos_real dominates pos_imag
            {
              if (pos_real > pos_imag*ten_to_PP)
                 {
                   const FloatCell real_cell(value.cval[0]);
                   return real_cell.character_representation(pctx);
                 }
            }
         else                        // pos_imag dominates pos_real
            {
              if (pos_imag > pos_real*ten_to_PP)
                 {
                   const FloatCell imag_cell(value.cval[1]);
                   PrintBuffer ret = imag_cell.character_representation(pctx);
                   ret.pad_l(UNI_J, 1);
                   ret.pad_l(UNI_0, 1);

                   ret.get_info().flags |= CT_COMPLEX;
                   ret.get_info().imag_len = 1 + ret.get_info().real_len;
                   ret.get_info().int_len = 1;
                   ret.get_info().fract_len = 0;
                   ret.get_info().real_len = 1;
                   return ret;
                 }
            }
      }

bool scaled_real = pctx.get_scaled();   // may be changed by print function
UCS_string ucs(value.cval[0], scaled_real, pctx);

ColInfo info;
   info.flags |= CT_COMPLEX;
   if (scaled_real)   info.flags |= real_has_E;
int int_fract = ucs.size();
   info.real_len = ucs.size();
   info.int_len = ucs.size();
   loop(u, ucs.size())
      {
       if (ucs[u] == UNI_FULLSTOP)
           {
             info.int_len = u;
             if (!scaled_real)   break;
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

   if (!is_near_real())
      {
        ucs << UNI_J;
        bool scaled_imag = pctx.get_scaled();  // may be changed by UCS_string()
        const UCS_string ucs_i(value.cval[1], scaled_imag, pctx);

        ucs << ucs_i;

        info.imag_len = ucs.size() - info.real_len;
        if (scaled_imag)   info.flags |= imag_has_E;
      }

   return PrintBuffer(ucs, info);
}
//────────────────────────────────────────────────────────────────────────────
bool
ComplexCell::need_scaling(const PrintContext &pctx) const
{
   // a complex number needs scaling if the real part needs it, ot
   // the complex part is significant and needs it.
   return FloatCell::need_scaling(value.cval[0], pctx.get_PP()) ||
          (!is_near_real() && 
          FloatCell::need_scaling(value.cval[1], pctx.get_PP()));
}
//────────────────────────────────────────────────────────────────────────────
bool
ComplexCell::get_near_bool()  const   
{
   if (value.cval[1] >  INTEGER_TOLERANCE)   DOMAIN_ERROR;
   if (value.cval[1] < -INTEGER_TOLERANCE)   DOMAIN_ERROR;

   if (value.cval[0] > INTEGER_TOLERANCE)   // 1 or invalid
      {
        if (value.cval[0] < (1.0 - INTEGER_TOLERANCE))   DOMAIN_ERROR;
        if (value.cval[0] > (1.0 + INTEGER_TOLERANCE))   DOMAIN_ERROR;
        return true;
      }
   else
      {
        if (value.cval[0] < -INTEGER_TOLERANCE)   DOMAIN_ERROR;
        return false;
      }
}
//────────────────────────────────────────────────────────────────────────────
APL_Integer
ComplexCell::get_near_int() const
{
// if (value.cval[1] >  qct)   DOMAIN_ERROR;
// if (value.cval[1] < -qct)   DOMAIN_ERROR;

const APL_Float val = value.cval[0];
const APL_Float result = round(val);
const APL_Float diff = val - result;
   if (diff > INTEGER_TOLERANCE)    DOMAIN_ERROR;
   if (diff < -INTEGER_TOLERANCE)   DOMAIN_ERROR;

   if (result > 0.0)   return   APL_Integer(result + 0.3);
   else                return - APL_Integer(0.3 - result);
}
//════════════════════════════════════════════════════════════════════════════
