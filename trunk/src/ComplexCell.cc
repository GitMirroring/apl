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
   return ComplexCell::bif_ceiling_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_conjugate(Cell * Z) const
{
   return ComplexCell::bif_conjugate_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_direction(Cell * Z) const
{
   return ComplexCell::bif_direction_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_exponential(Cell * Z) const
{
   return ComplexCell::bif_exponential_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
// monadic build-in functions...
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_factorial(Cell * Z) const
{
   return ComplexCell::bif_factorial_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_floor(Cell * Z) const
{
   return ComplexCell::bif_floor_c(Z, cval());
}
//════════════════════════════════════════════════════════════════════════════
ErrorCode
ComplexCell::bif_magnitude(Cell * Z) const
{
   return ComplexCell::bif_magnitude_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_nat_log(Cell * Z) const
{
   return ComplexCell::bif_nat_log_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_negative(Cell * Z) const
{
   return ComplexCell::bif_negative_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_pi_times(Cell * Z) const
{
   return ComplexCell::bif_pi_times_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_pi_times_inverse(Cell * Z) const
{
   return ComplexCell::bif_pi_times_inverse_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_reciprocal(Cell * Z) const
{
   return ComplexCell::bif_reciprocal_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_roll(Cell * Z) const
{
   return ComplexCell::bif_roll_c(Z, cval());
}
//════════════════════════════════════════════════════════════════════════════
// dyadic build-in functions...
//════════════════════════════════════════════════════════════════════════════
ErrorCode
ComplexCell::bif_add(Cell * Z, const Cell * A) const
{
   return ComplexCell::bif_add_cc(Z, A->get_complex_value(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_subtract(Cell * Z, const Cell * A) const
{
   return ComplexCell::bif_subtract_cc(Z, A->get_complex_value(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_divide(Cell * Z, const Cell * A) const
{
   return ComplexCell::bif_divide_cc(Z, A->get_complex_value(), cval());
}
//════════════════════════════════════════════════════════════════════════════
ErrorCode
ComplexCell::bif_logarithm(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;
   return ComplexCell::bif_logarithm_cc(Z, A->get_complex_value(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_multiply(Cell * Z, const Cell * A) const
{
   return ComplexCell::bif_multiply_cc(Z, A->get_complex_value(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_power(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;
   return ComplexCell::bif_power_cc(Z, A->get_complex_value(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_maximum(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;
   return ComplexCell::bif_maximum_cc(Z, A->get_complex_value(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_minimum(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;
   return ComplexCell::bif_minimum_cc(Z, A->get_complex_value(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_residue(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;
   return ComplexCell::bif_residue_cc(Z, A->get_complex_value(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_circle_fun(Cell * Z, const Cell * A) const
{
   if (!A->is_near_int())   return E_DOMAIN_ERROR;
   return ComplexCell::bif_circle_fun_c(Z, A->get_checked_near_int(), cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_circle_fun_inverse(Cell * Z, const Cell * A) const
{
   if (!A->is_near_int())   return E_DOMAIN_ERROR;
   return ComplexCell::bif_circle_fun_inverse_c(Z, A->get_checked_near_int(),
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
   return ComplexCell::bif_near_int64_t_c(Z, cval());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_within_quad_CT(Cell * Z) const
{
   return ComplexCell::bif_within_quad_CT_c(Z, cval());
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
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_add_cc(Cell * Z, APL_Complex a, APL_Complex b)
{
const APL_Complex z = a + b;
   if (!isfinite(z.real()))   return E_DOMAIN_ERROR;
   if (!isfinite(z.imag()))   return E_DOMAIN_ERROR;
   return ComplexCell::zC(Z, z);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_subtract_cc(Cell * Z, APL_Complex a, APL_Complex b)
{
const APL_Complex z = a - b;
   if (!isfinite(z.real()))   return E_DOMAIN_ERROR;
   if (!isfinite(z.imag()))   return E_DOMAIN_ERROR;
   return ComplexCell::zC(Z, z);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_multiply_cc(Cell * Z, APL_Complex a, APL_Complex b)
{
const APL_Complex z = a * b;
   if (!isfinite(z.real()))   return E_DOMAIN_ERROR;
   if (!isfinite(z.imag()))   return E_DOMAIN_ERROR;
   return ComplexCell::zC(Z, z);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_divide_cc(Cell * Z, APL_Complex a, APL_Complex b)
{
   if (b.real() == 0.0 && b.imag() == 0.0)
      {
        if (a.real() != 0.0)   return E_DOMAIN_ERROR;
        if (a.imag() != 0.0)   return E_DOMAIN_ERROR;
        return IntCell::z1(Z);
      }

   // Smith's algorithm (avoids squaring b's components, unlike the naive
   // formula and unlike libstdc++'s std::complex<>::operator/() for some
   // versions/platforms) - confirmed directly: with the naive/library
   // division, (1E10J1E10) ÷ (1E¯300J0) silently underflowed to 0J0
   // instead of overflowing (the true quotient, ~1E310, does not fit in
   // a double either way, but the wrong answer was a finite 0, not Inf).
   //
APL_Complex z;
   if (fabs(b.real()) >= fabs(b.imag()))
      {
        const APL_Float r   = b.imag() / b.real();
        const APL_Float den = b.real() + r*b.imag();
        z = APL_Complex((a.real() + r*a.imag()) / den,
                         (a.imag() - r*a.real()) / den);
      }
   else
      {
        const APL_Float r   = b.real() / b.imag();
        const APL_Float den = b.imag() + r*b.real();
        z = APL_Complex((a.real()*r + a.imag()) / den,
                         (a.imag()*r - a.real()) / den);
      }

   if (!isfinite(z.real()))   return E_DOMAIN_ERROR;
   if (!isfinite(z.imag()))   return E_DOMAIN_ERROR;
   return ComplexCell::zC(Z, z);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_power_ci(Cell * Z, APL_Complex a, APL_Integer b)
{
   // complex base to integer power (no parity trick, use complex_power)
   //
const bool invert_Z = b < 0;
   if (invert_Z)   b = -b;

   if (b == 0)   return IntCell::z1(Z);

   if (b == 1)
      {
        if (!invert_Z)   return ComplexCell::zC(Z, a);
        const APL_Float denom = a.real()*a.real() + a.imag()*a.imag();
        if (denom == 0.0)   return E_DOMAIN_ERROR;
        return ComplexCell::zC(Z, a.real()/denom, -a.imag()/denom);
      }

const APL_Complex z = complex_power(a, APL_Complex(APL_Float(b), 0.0));
   if (!isfinite(z.real()))   return E_DOMAIN_ERROR;
   if (!isfinite(z.imag()))   return E_DOMAIN_ERROR;
   if (!invert_Z)   return ComplexCell::zC(Z, z);

const APL_Float denom = z.real()*z.real() + z.imag()*z.imag();
   if (denom == 0.0)   return E_DOMAIN_ERROR;
   return ComplexCell::zC(Z, z.real()/denom, -z.imag()/denom);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_power_cc(Cell * Z, APL_Complex a, APL_Complex b)
{
   if (a.real() == 0.0 && a.imag() == 0.0)
      {
        if (b.real() == 0.0)   return IntCell::z1(Z);
        if (b.real()  > 0.0)   return IntCell::z0(Z);
        return E_DOMAIN_ERROR;
      }
const APL_Complex z = complex_power(a, b);
   if (!isfinite(z.real()))   return E_DOMAIN_ERROR;
   if (!isfinite(z.imag()))   return E_DOMAIN_ERROR;
   return ComplexCell::zC(Z, z);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_logarithm_cc(Cell * Z, APL_Complex a, APL_Complex b)
{
   // a = base (A), b = argument (B = this)
   //
   if (b == a)   return IntCell::z1(Z);
   if (b.real() == 0.0 && b.imag() == 0.0)   return E_DOMAIN_ERROR;
   if (fabs(a.real() - 1.0) <= INTEGER_TOLERANCE &&
       fabs(a.imag())        <= INTEGER_TOLERANCE)   return E_DOMAIN_ERROR;

   if (a.imag() == 0.0)
      {
        const APL_Complex z = log(b) / log(a.real());
        if (!isfinite(z.real()))   return E_DOMAIN_ERROR;
        if (!isfinite(z.imag()))   return E_DOMAIN_ERROR;
        return ComplexCell::zC(Z, z);
      }

const APL_Complex z = log(b) / log(a);
   if (!isfinite(z.real()))   return E_DOMAIN_ERROR;
   if (!isfinite(z.imag()))   return E_DOMAIN_ERROR;
   return ComplexCell::zC(Z, z);
}
// complex near-real test: |imag| is small relative to |real|
static inline bool
nc_near_real(APL_Complex c)
{
const APL_Float B2 = REAL_TOLERANCE * REAL_TOLERANCE;
const APL_Float I2 = c.imag() * c.imag();
   if (I2 < B2)   return true;   // absolutely small
const APL_Float R2 = c.real() * c.real();
   return I2 < R2 * B2;          // relatively small
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_maximum_cc(Cell * Z, APL_Complex a, APL_Complex b)
{
   if (!nc_near_real(a))   return E_DOMAIN_ERROR;
   if (!nc_near_real(b))   return E_DOMAIN_ERROR;
   return ComplexCell::zV(Z, a.real() >= b.real() ? a.real() : b.real());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_minimum_cc(Cell * Z, APL_Complex a, APL_Complex b)
{
   if (!nc_near_real(a))   return E_DOMAIN_ERROR;
   if (!nc_near_real(b))   return E_DOMAIN_ERROR;
   return ComplexCell::zV(Z, a.real() <= b.real() ? a.real() : b.real());
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_residue_cc(Cell * Z, APL_Complex a, APL_Complex b)
{
   if (a.real() == 0.0 && a.imag() == 0.0)   return ComplexCell::zC(Z, b);
   if (b.real() == 0.0 && b.imag() == 0.0)   return IntCell::z0(Z);

const APL_Complex quot = b / a;

   // complex floor of quot
   //
APL_Float fr = floor(quot.real());
APL_Float Dr = quot.real() - fr;
APL_Float fi = floor(quot.imag());
APL_Float Di = quot.imag() - fi;
const double qct = Workspace::get_CT();
const double limit = 1.0 - qct;
   if (Dr > limit)   { fr += 1.0;   Dr = 0.0; }
   if (Di > limit)   { fi += 1.0;   Di = 0.0; }

APL_Complex floor_quot;
   if ((Dr + Di) < limit)     floor_quot = APL_Complex(fr,        fi);
   else if (Dr < (Di - qct))  floor_quot = APL_Complex(fr,        fi + 1.0);
   else                       floor_quot = APL_Complex(fr + 1.0,  fi);

   return ComplexCell::zC(Z, b - a * floor_quot);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_circle_fun_c(Cell * Z, APL_Integer fun, APL_Complex b)
{
   IntCell::z0(Z);
const ErrorCode ret = ComplexCell::do_bif_circle_fun(Z, fun, b);
   if (!Z->is_finite())   return E_DOMAIN_ERROR;
   return ret;
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_circle_fun_inverse_c(Cell * Z, APL_Integer fun, APL_Complex b)
{
   IntCell::z0(Z);

ErrorCode ret = E_DOMAIN_ERROR;
   switch(fun)
      {
        case  1: case -1:
        case  2: case -2:
        case  3: case -3:
        case  4: case -4:
        case  5: case -5:
        case  6: case -6:
        case  7: case -7:
                 ret = ComplexCell::do_bif_circle_fun(Z, -fun, b);
                 if (!Z->is_finite())   return E_DOMAIN_ERROR;
                 return ret;

        case -10:
                 ret = ComplexCell::do_bif_circle_fun(Z, fun, b);
                 if (!Z->is_finite())   return E_DOMAIN_ERROR;
                 return ret;

        default: return E_DOMAIN_ERROR;
      }

   return E_DOMAIN_ERROR;
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_ceiling_c(Cell * Z, APL_Complex b)
{
const APL_Float cr = ceil(b.real());
const APL_Float Dr = cr - b.real();
const APL_Float ci = ceil(b.imag());
const APL_Float Di = ci - b.imag();
const APL_Float D  = Dr + Di;
   if (D < 1.0)     return ComplexCell::zV(Z, cr, ci);
   if (Di > Dr)     return ComplexCell::zV(Z, cr, ci - 1.0);
   else             return ComplexCell::zV(Z, cr - 1.0, ci);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_floor_c(Cell * Z, APL_Complex b)
{
APL_Float fr = floor(b.real());
APL_Float Dr = b.real() - fr;
APL_Float fi = floor(b.imag());
APL_Float Di = b.imag() - fi;
const double qct = Workspace::get_CT();
const double limit = 1.0 - qct;
   if (Dr > limit)   { fr += 1.0;   Dr = 0.0; }
   if (Di > limit)   { fi += 1.0;   Di = 0.0; }
   if ((Dr + Di) < limit)    return ComplexCell::zV(Z, fr, fi);
   if (Dr < (Di - qct))      return ComplexCell::zV(Z, fr, fi + 1.0);
   return ComplexCell::zV(Z, fr + 1.0, fi);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_conjugate_c(Cell * Z, APL_Complex b)
{
   return ComplexCell::zC(Z, conj(b));
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_direction_c(Cell * Z, APL_Complex b)
{
const APL_Float mag = abs(b);
   if (mag == 0.0)   return IntCell::z0(Z);
   return ComplexCell::zC(Z, b.real()/mag, b.imag()/mag);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_exponential_c(Cell * Z, APL_Complex b)
{
   return ComplexCell::zC(Z, complex_exponent(b));
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_factorial_c(Cell * Z, APL_Complex b)
{
   if (nc_near_real(b))   return FloatCell::bif_factorial_f(Z, b.real());

ErrorCode ret = ComplexCell::zC(Z, ComplexCell::gamma(b.real() + 1.0,
                                                       b.imag()));
   if (errno)   return E_DOMAIN_ERROR;
   return ret;
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_magnitude_c(Cell * Z, APL_Complex b)
{
   return FloatCell::zF(Z, abs(b));
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_nat_log_c(Cell * Z, APL_Complex b)
{
   return ComplexCell::zC(Z, log(b));
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_negative_c(Cell * Z, APL_Complex b)
{
   return ComplexCell::zC(Z, -b);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_pi_times_c(Cell * Z, APL_Complex b)
{
const APL_Float pi(M_PI);
   return ComplexCell::zC(Z, b.real() * pi, b.imag() * pi);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_pi_times_inverse_c(Cell * Z, APL_Complex b)
{
const APL_Float pi(M_PI);
   return ComplexCell::zC(Z, b.real() / pi, b.imag() / pi);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_reciprocal_c(Cell * Z, APL_Complex b)
{
   // near-zero check
   //
   if (b.real() <=  INTEGER_TOLERANCE && b.real() >= -INTEGER_TOLERANCE &&
       b.imag() <=  INTEGER_TOLERANCE && b.imag() >= -INTEGER_TOLERANCE)
      return E_DOMAIN_ERROR;

   // near-real: use simpler formula
   //
const APL_Float B2 = REAL_TOLERANCE * REAL_TOLERANCE;
const APL_Float I2 = b.imag() * b.imag();
const APL_Float R2 = b.real() * b.real();
   if (I2 < B2 || I2 < R2*B2)
      {
        const APL_Float z = 1.0 / b.real();
        if (!isfinite(z))   return E_DOMAIN_ERROR;
        return FloatCell::zF(Z, z);
      }

const APL_Float denom = R2 + I2;
const APL_Float r = b.real() / denom;
   if (!isfinite(r))   return E_DOMAIN_ERROR;
const APL_Float i = b.imag() / denom;
   if (!isfinite(i))   return E_DOMAIN_ERROR;
   return ComplexCell::zC(Z, r, -i);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_roll_c(Cell * Z, APL_Complex b)
{
   if (!Cell::is_near_int(b.real()))   return E_DOMAIN_ERROR;
   if (!Cell::is_near_int(b.imag()))   return E_DOMAIN_ERROR;
   if (nearbyint(b.imag()) != 0.0)     return E_DOMAIN_ERROR;
const APL_Integer set_size = APL_Integer(nearbyint(b.real()));
   if (set_size <= 0)   return E_DOMAIN_ERROR;
const uint64_t rnd = Workspace::get_RL(set_size);
   return IntCell::zI(Z, Workspace::get_IO() + APL_Integer(rnd % set_size));
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_near_int64_t_c(Cell * Z, APL_Complex b)
{
   if (!Cell::is_near_int64_t(b.real()))   return E_DOMAIN_ERROR;
   if (!Cell::is_near_int64_t(b.imag()))   return E_DOMAIN_ERROR;

   if (b.imag() <  INTEGER_TOLERANCE &&
       b.imag() > -INTEGER_TOLERANCE &&
       b.real() <  BIG_INT64_F       &&
       b.real() > -BIG_INT64_F)
      return FloatCell::zF(Z, round(b.real()));

   return ComplexCell::zC(Z, round(b.real()), round(b.imag()));
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::bif_within_quad_CT_c(Cell * Z, APL_Complex b)
{
const double val_r = b.real();
   if (val_r > LARGE_INT)   return E_DOMAIN_ERROR;
   if (val_r < SMALL_INT)   return E_DOMAIN_ERROR;
const double val_i = b.imag();
   if (val_i > LARGE_INT)   return E_DOMAIN_ERROR;
   if (val_i < SMALL_INT)   return E_DOMAIN_ERROR;

const double max_diff_r = Workspace::get_CT() * fabs(val_r);
const double max_diff_i = Workspace::get_CT() * fabs(val_i);

const APL_Float val_dn_r = floor(val_r);
const APL_Float val_up_r = ceil(val_r);
const APL_Float val_dn_i = floor(val_i);
const APL_Float val_up_i = ceil(val_i);

double z_r;
   if      (val_r < (val_dn_r + max_diff_r))   z_r = val_dn_r;
   else if (val_r > (val_up_r - max_diff_r))   z_r = val_up_r;
   else                                        return E_DOMAIN_ERROR;

double z_i;
   if      (val_i < (val_dn_i + max_diff_i))   z_i = val_dn_i;
   else if (val_i > (val_up_i - max_diff_i))   z_i = val_up_i;
   else                                        return E_DOMAIN_ERROR;

   return ComplexCell::zC(Z, z_r, z_i);
}
//════════════════════════════════════════════════════════════════════════════
//════════════════════════════════════════════════════════════════════════════
//════════════════════════════════════════════════════════════════════════════
ErrorCode
ComplexCell::zV(Cell * Z, APL_Float flt)
{
   if (!Cell::is_near_int64_t(flt))   return FloatCell::zF(Z, flt);

   return flt < 0 ? IntCell::zI(Z, -int64_t(0.5 - flt))
                  : IntCell::zI(Z,  int64_t(0.5 + flt));
}
//──────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::zV(Cell * Z, APL_Complex cpx)
{
   return Cell::is_near_zero(cpx.imag()) ? ComplexCell::zV(Z, cpx.real())
                                   : ComplexCell::zC(Z, cpx);
}
//──────────────────────────────────────────────────────────────────────────
ErrorCode
ComplexCell::zV(Cell * Z, APL_Float real, APL_Float imag)
{
   return Cell::is_near_zero(imag) ? ComplexCell::zV(Z, real)
                             : ComplexCell::zC(Z, real, imag);
}
//════════════════════════════════════════════════════════════════════════════



