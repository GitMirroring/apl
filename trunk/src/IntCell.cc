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
#include <math.h>

#include "Value.hh"
#include "ErrorCode.hh"
#include "PointerCell.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "Workspace.hh"
//════════════════════════════════════════════════════════════════════════════

const IntCell IntCell::boolean_FALSE(0);
const IntCell IntCell::boolean_TRUE(1);

/*
   general comment: for operations between an IntCell and a "higher" numeric
   Cell type (i.e. FloatCell or ComplexCell), we try to call the corresponding
   function (or its opposite) of the higher type.

   For example:
   INT ≠ FLOAT  --> FLOAT ≠ INT
   INT ≤ FLOAT  --> FLOAT ≥ INT   (≤ is the opposite of ≥)

 */
//────────────────────────────────────────────────────────────────────────────
// monadic built-in functions...
//────────────────────────────────────────────────────────────────────────────

//────────────────────────────────────────────────────────────────────────────
/// overloaded Cell::bif_near_int64_t()
ErrorCode
IntCell::bif_near_int64_t(Cell * Z) const
{
   return IntCell::bif_near_int64_t_i(Z, value.ival);
}

//────────────────────────────────────────────────────────────────────────────
/// overloaded Cell::bif_within_quad_CT()
ErrorCode
IntCell::bif_within_quad_CT(Cell * Z) const
{
   return IntCell::bif_within_quad_CT_i(Z, value.ival);
}
//────────────────────────────────────────────────────────────────────────────
bool
IntCell::equal(const Cell & other, double qct) const
{
   if (other.is_integer_cell())    return value.ival == other.get_int_value();
   if (other.is_numeric())         return other.equal(*this, qct);
   return false;
}
//────────────────────────────────────────────────────────────────────────────
// dyadic built-in functions...
//
// where possible a function with non-int A is delegated to the corresponding
// member function of A. For numeric cells that is the FloatCell or ComplexCell
// function and otherwise the default function (that returns E_DOMAIN_ERROR.
//
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_add(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      return IntCell::bif_add_ii(Z, A->get_int_value(), value.ival);
   if (A->is_real_cell())
      return FloatCell::bif_add_ff(Z, A->get_real_value(), APL_Float(value.ival));
   if (A->is_complex_cell())
      return ComplexCell::bif_add_cc(Z, A->get_complex_value(),
                                       APL_Complex(value.ival, 0));
   return E_DOMAIN_ERROR;
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_ceiling(Cell * Z) const
{
   return IntCell::bif_ceiling_i(Z, value.ival);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_conjugate(Cell * Z) const
{
   return IntCell::bif_conjugate_i(Z, value.ival);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_direction(Cell * Z) const
{
   return IntCell::bif_direction_i(Z, value.ival);
}
ErrorCode
IntCell::bif_divide(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;

   if (A->is_integer_cell())
      {
#ifdef cfg_RATIONAL_NUMBERS_WANTED
        APL_Integer a = A->get_int_value();
        APL_Integer b = value.ival;
        if (b == 0)
           {
             if (a == 0)   return IntCell::z1(Z);
             return E_DOMAIN_ERROR;
           }

        // a = -a and b = -b below would overflow (UB) for a or b ==
        // INT64_MIN, since -INT64_MIN is not representable as
        // APL_Integer (the same trap fixed elsewhere for bif_divide_ii/
        // bif_power_ii/bif_negative_i); reject up front instead of
        // risking UB or a wrong GCD from a still-negative operand.
        //
        if (a == APL_Integer(0x8000000000000000ULL))   return E_DOMAIN_ERROR;
        if (b == APL_Integer(0x8000000000000000ULL))   return E_DOMAIN_ERROR;

        if (b < 0)   { a = -a;   b = -b; }
        const APL_Integer g = FloatCell::gcd(b, a);
        if (b == g)   return IntCell::zI(Z, a/g);
        return FloatCell::zR(Z, a/g, b/g);
#else
        return IntCell::bif_divide_ii(Z, A->get_int_value(), value.ival);
#endif
      }
   if (A->is_real_cell())
      return FloatCell::bif_divide_ff(Z, A->get_real_value(),
                                          APL_Float(value.ival));
   if (A->is_complex_cell())
      return ComplexCell::bif_divide_cc(Z, A->get_complex_value(),
                                          APL_Complex(value.ival, 0));
   return E_DOMAIN_ERROR;
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_exponential(Cell * Z) const
{
   return IntCell::bif_exponential_i(Z, value.ival);
}
extern const APL_Integer int_factorials[] =
{
   /*  0! */  0x0000000000000001LL,
   /*  1! */  0x0000000000000001LL,
   /*  2! */  0x0000000000000002LL,
   /*  3! */  0x0000000000000006LL,
   /*  4! */  0x0000000000000018LL,
   /*  5! */  0x0000000000000078LL,
   /*  6! */  0x00000000000002d0LL,
   /*  7! */  0x00000000000013b0LL,
   /*  8! */  0x0000000000009d80LL,
   /*  9! */  0x0000000000058980LL,
   /* 10! */  0x0000000000375f00LL,
   /* 11! */  0x0000000002611500LL,
   /* 12! */  0x000000001c8cfc00LL,
   /* 13! */  0x000000017328cc00LL,
   /* 14! */  0x000000144c3b2800LL,
   /* 15! */  0x0000013077775800LL,
   /* 16! */  0x0000130777758000LL,
   /* 17! */  0x0001437eeecd8000LL,
   /* 18! */  0x0016beecca730000LL,
   /* 19! */  0x01b02b9306890000LL,
   /* 20! */  0x21c3677c82b40000LL
};

extern const APL_Float float_factorials[] =
{
   /*   0! */   1.000000000000000E0   ,
   /*   1! */   1.000000000000000E0   ,
   /*   2! */   2.000000000000000E0   ,
   /*   3! */   6.000000000000000E0   ,
   /*   4! */   2.400000000000000E1   ,
   /*   5! */   1.200000000000000E2   ,
   /*   6! */   7.200000000000000E2   ,
   /*   7! */   5.040000000000001E3   ,
   /*   8! */   4.032000000000000E4   ,
   /*   9! */   3.628800000000000E5   ,
   /*  10! */   3.628800000000000E6   ,
   /*  11! */   3.991680000000001E7   ,
   /*  12! */   4.790016000000000E8   ,
   /*  13! */   6.227020800000002E9   ,
   /*  14! */   8.717829120000003E10  ,
   /*  15! */   1.307674368000000E12  ,
   /*  16! */   2.092278988800000E13  ,
   /*  17! */   3.556874280960001E14  ,
   /*  18! */   6.402373705728002E15  ,
   /*  19! */   1.216451004088320E17  ,
   /*  20! */   2.432902008176640E18  ,
   /*  21! */   5.109094217170945E19  ,
   /*  22! */   1.124000727777608E21  ,
   /*  23! */   2.585201673888498E22  ,
   /*  24! */   6.204484017332394E23  ,
   /*  25! */   1.551121004333099E25  ,
   /*  26! */   4.032914611266057E26  ,
   /*  27! */   1.088886945041835E28  ,
   /*  28! */   3.048883446117139E29  ,
   /*  29! */   8.841761993739700E30  ,
   /*  30! */   2.652528598121910E32  ,
   /*  31! */   8.222838654177924E33  ,
   /*  32! */   2.631308369336936E35  ,
   /*  33! */   8.683317618811886E36  ,
   /*  34! */   2.952327990396041E38  ,
   /*  35! */   1.033314796638614E40  ,
   /*  36! */   3.719933267899012E41  ,
   /*  37! */   1.376375309122635E43  ,
   /*  38! */   5.230226174666011E44  ,
   /*  39! */   2.039788208119745E46  ,
   /*  40! */   8.159152832478979E47  ,
   /*  41! */   3.345252661316380E49  ,
   /*  42! */   1.405006117752880E51  ,
   /*  43! */   6.041526306337384E52  ,
   /*  44! */   2.658271574788449E54  ,
   /*  45! */   1.196222208654802E56  ,
   /*  46! */   5.502622159812089E57  ,
   /*  47! */   2.586232415111683E59  ,
   /*  48! */   1.241391559253607E61  ,
   /*  49! */   6.082818640342676E62  ,
   /*  50! */   3.041409320171338E64  ,
   /*  51! */   1.551118753287382E66  ,
   /*  52! */   8.065817517094388E67  ,
   /*  53! */   4.274883284060025E69  ,
   /*  54! */   2.308436973392414E71  ,
   /*  55! */   1.269640335365828E73  ,
   /*  56! */   7.109985878048636E74  ,
   /*  57! */   4.052691950487722E76  ,
   /*  58! */   2.350561331282879E78  ,
   /*  59! */   1.386831185456899E80  ,
   /*  60! */   8.320987112741390E81  ,
   /*  61! */   5.075802138772250E83  ,
   /*  62! */   3.146997326038794E85  ,
   /*  63! */   1.982608315404440E87  ,
   /*  64! */   1.268869321858842E89  ,
   /*  65! */   8.247650592082470E90  ,
   /*  66! */   5.443449390774431E92  ,
   /*  67! */   3.647111091818869E94  ,
   /*  68! */   2.480035542436830E96  ,
   /*  69! */   1.711224524281413E98  ,
   /*  70! */   1.197857166996989E100 ,
   /*  71! */   8.504785885678620E101 ,
   /*  72! */   6.123445837688608E103 ,
   /*  73! */   4.470115461512683E105 ,
   /*  74! */   3.307885441519385E107 ,
   /*  75! */   2.480914081139540E109 ,
   /*  76! */   1.885494701666050E111 ,
   /*  77! */   1.451830920282858E113 ,
   /*  78! */   1.132428117820629E115 ,
   /*  79! */   8.946182130782972E116 ,
   /*  80! */   7.156945704626378E118 ,
   /*  81! */   5.797126020747363E120 ,
   /*  82! */   4.753643337012839E122 ,
   /*  83! */   3.945523969720656E124 ,
   /*  84! */   3.314240134565352E126 ,
   /*  85! */   2.817104114380549E128 ,
   /*  86! */   2.422709538367272E130 ,
   /*  87! */   2.107757298379527E132 ,
   /*  88! */   1.854826422573984E134 ,
   /*  89! */   1.650795516090845E136 ,
   /*  90! */   1.485715964481761E138 ,
   /*  91! */   1.352001527678402E140 ,
   /*  92! */   1.243841405464130E142 ,
   /*  93! */   1.156772507081641E144 ,
   /*  94! */   1.087366156656742E146 ,
   /*  95! */   1.032997848823905E148 ,
   /*  96! */   9.916779348709490E149 ,
   /*  97! */   9.619275968248207E151 ,
   /*  98! */   9.426890448883240E153 ,
   /*  99! */   9.332621544394408E155 ,
   /* 100! */   9.332621544394408E157 ,
   /* 101! */   9.425947759838357E159 ,
   /* 102! */   9.614466715035121E161 ,
   /* 103! */   9.902900716486176E163 ,
   /* 104! */   1.029901674514562E166 ,
   /* 105! */   1.081396758240290E168 ,
   /* 106! */   1.146280563734708E170 ,
   /* 107! */   1.226520203196138E172 ,
   /* 108! */   1.324641819451828E174 ,
   /* 109! */   1.443859583202492E176 ,
   /* 110! */   1.588245541522742E178 ,
   /* 111! */   1.762952551090243E180 ,
   /* 112! */   1.974506857221073E182 ,
   /* 113! */   2.231192748659812E184 ,
   /* 114! */   2.543559733472186E186 ,
   /* 115! */   2.925093693493014E188 ,
   /* 116! */   3.393108684451897E190 ,
   /* 117! */   3.969937160808718E192 ,
   /* 118! */   4.684525849754287E194 ,
   /* 119! */   5.574585761207601E196 ,
   /* 120! */   6.689502913449124E198 ,
   /* 121! */   8.094298525273436E200 ,
   /* 122! */   9.875044200833596E202 ,
   /* 123! */   1.214630436702533E205 ,
   /* 124! */   1.506141741511140E207 ,
   /* 125! */   1.882677176888925E209 ,
   /* 126! */   2.372173242880046E211 ,
   /* 127! */   3.012660018457658E213 ,
   /* 128! */   3.856204823625802E215 ,
   /* 129! */   4.974504222477286E217 ,
   /* 130! */   6.466855489220473E219 ,
   /* 131! */   8.471580690878811E221 ,
   /* 132! */   1.118248651196003E224 ,
   /* 133! */   1.487270706090685E226 ,
   /* 134! */   1.992942746161518E228 ,
   /* 135! */   2.690472707318050E230 ,
   /* 136! */   3.659042881952546E232 ,
   /* 137! */   5.012888748274990E234 ,
   /* 138! */   6.917786472619488E236 ,
   /* 139! */   9.615723196941085E238 ,
   /* 140! */   1.346201247571752E241 ,
   /* 141! */   1.898143759076170E243 ,
   /* 142! */   2.695364137888161E245 ,
   /* 143! */   3.854370717180069E247 ,
   /* 144! */   5.550293832739300E249 ,
   /* 145! */   8.047926057471988E251 ,
   /* 146! */   1.174997204390910E254 ,
   /* 147! */   1.727245890454637E256 ,
   /* 148! */   2.556323917872863E258 ,
   /* 149! */   3.808922637630566E260 ,
   /* 150! */   5.713383956445850E262 ,
   /* 151! */   8.627209774233233E264 ,
   /* 152! */   1.311335885683452E267 ,
   /* 153! */   2.006343905095681E269 ,
   /* 154! */   3.089769613847348E271 ,
   /* 155! */   4.789142901463389E273 ,
   /* 156! */   7.471062926282889E275 ,
   /* 157! */   1.172956879426414E278 ,
   /* 158! */   1.853271869493734E280 ,
   /* 159! */   2.946702272495036E282 ,
   /* 160! */   4.714723635992057E284 ,
   /* 161! */   7.590705053947215E286 ,
   /* 162! */   1.229694218739448E289 ,
   /* 163! */   2.004401576545301E291 ,
   /* 164! */   3.287218585534293E293 ,
   /* 165! */   5.423910666131586E295 ,
   /* 166! */   9.003691705778428E297 ,
   /* 167! */   1.503616514864998E300 ,
   /* 168! */   2.526075744973197E302 ,
   /* 169! */   4.269068009004701E304 ,
   /* 170! */   7.257415615307993E306
};
enum
{
   int_fact_max   = sizeof(int_factorials)   / sizeof(APL_Integer),
   float_fact_max = sizeof(float_factorials) / sizeof(APL_Float)
};

ErrorCode
IntCell::bif_factorial(Cell * Z) const
{
   return IntCell::bif_factorial_i(Z, value.ival);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_floor(Cell * Z) const
{
   return IntCell::bif_floor_i(Z, value.ival);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_magnitude(Cell * Z) const
{
   return IntCell::bif_magnitude_i(Z, value.ival);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_multiply(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      return IntCell::bif_multiply_ii(Z, A->get_int_value(), value.ival);
   if (A->is_real_cell())
      return FloatCell::bif_multiply_ff(Z, A->get_real_value(),
                                            APL_Float(value.ival));
   if (A->is_complex_cell())
      return ComplexCell::bif_multiply_cc(Z, A->get_complex_value(),
                                            APL_Complex(value.ival, 0));
   return E_DOMAIN_ERROR;
}
//────────────────────────────────────────────────────────────────────────────
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_power(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;
   if (A->is_integer_cell())
      return IntCell::bif_power_ii(Z, A->get_int_value(), value.ival);
   if (A->is_real_cell())
      return FloatCell::bif_power_fi(Z, A->get_real_value(), value.ival);
   return ComplexCell::bif_power_ci(Z, A->get_complex_value(), value.ival);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_nat_log(Cell * Z) const
{
   return IntCell::bif_nat_log_i(Z, value.ival);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_negative(Cell * Z) const
{
   return IntCell::bif_negative_i(Z, value.ival);
}
ErrorCode
IntCell::bif_pi_times(Cell * Z) const
{
   return IntCell::bif_pi_times_i(Z, value.ival);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_pi_times_inverse(Cell * Z) const
{
   return IntCell::bif_pi_times_inverse_i(Z, value.ival);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_reciprocal(Cell * Z) const
{
#ifdef cfg_RATIONAL_NUMBERS_WANTED
   switch(value.ival)
      {
        case  0: return E_DOMAIN_ERROR;
        case  1: return IntCell::z1(Z);
        case -1: return IntCell::z_1(Z);
      }
   if (value.ival < 0)   return FloatCell::zR(Z, -1, -value.ival);
   else                  return FloatCell::zR(Z,  1,  value.ival);
#endif
   return IntCell::bif_reciprocal_i(Z, value.ival);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_roll(Cell * Z) const
{
   return IntCell::bif_roll_i(Z, value.ival);
}
//════════════════════════════════════════════════════════════════════════════
//════════════════════════════════════════════════════════════════════════════
ErrorCode
IntCell::bif_subtract(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      return IntCell::bif_subtract_ii(Z, A->get_int_value(), value.ival);
   if (A->is_real_cell())
      return FloatCell::bif_subtract_ff(Z, A->get_real_value(),
                                            APL_Float(value.ival));
   if (A->is_complex_cell())
      return ComplexCell::bif_subtract_cc(Z, A->get_complex_value(),
                                            APL_Complex(value.ival, 0));
   return E_DOMAIN_ERROR;
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_maximum(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      return IntCell::bif_maximum_ii(Z, A->get_int_value(), value.ival);
   if (A->is_real_cell())
      return FloatCell::bif_maximum_ff(Z, A->get_real_value(),
                                           APL_Float(value.ival));
   if (A->is_complex_cell())
      return ComplexCell::bif_maximum_cc(Z, A->get_complex_value(),
                                           APL_Complex(value.ival, 0));
   return E_DOMAIN_ERROR;
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_minimum(Cell * Z, const Cell * A) const
{
   if (A->is_integer_cell())
      return IntCell::bif_minimum_ii(Z, A->get_int_value(), value.ival);
   if (A->is_real_cell())
      return FloatCell::bif_minimum_ff(Z, A->get_real_value(),
                                           APL_Float(value.ival));
   if (A->is_complex_cell())
      return ComplexCell::bif_minimum_cc(Z, A->get_complex_value(),
                                           APL_Complex(value.ival, 0));
   return E_DOMAIN_ERROR;
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_residue(Cell * Z, const Cell * A) const
{
   if (!A->is_numeric())   return E_DOMAIN_ERROR;

   if (A->get_imag_value() != 0.0)
      return ComplexCell::bif_residue_cc(Z, A->get_complex_value(),
                                           APL_Complex(value.ival, 0));

const APL_Float a = A->get_real_value();

   // an exact IntCell A always takes the exact int64 '%' path below,
   // however large -- checked before the near-INT64-limit float
   // fallback rather than after it, since converting an A that is
   // already an exact IntCell to double is precisely where that
   // fallback loses the most precision (a double's 53-bit mantissa
   // cannot even distinguish adjacent int64 values up here), turning
   // what should be the best-case input into the worst case (Blake
   // McBride, Bugs22 #1).
   if (A->is_integer_cell())
      return IntCell::bif_residue_ii(Z, A->get_int_value(), value.ival);

   // for a genuinely non-integer A near INT64 limit, use float arithmetic
   if (a > (BIG_INT64_F - 1E10) || a < (1E10 - BIG_INT64_F))
      return FloatCell::bif_residue_ff(Z, a, APL_Float(value.ival));

   // non-integer float A: use CT tolerance check and near-zero guard
   // (mirrors the original IntCell::bif_residue for float A)
   if (a == 0.0)   return IntCell::zI(Z, value.ival);
   if (value.ival == 0)   return IntCell::z0(Z);

const APL_Float quot = value.ival / a;
const APL_Float qf = floor(quot);
const double qct = Workspace::get_CT();
   if (qct != 0)
      {
        // a nearby integer of 0 is never a match here either -- same
        // near-zero-quotient bug and fix as Cell::integral_within(),
        // whose two branches this duplicates by hand (Blake McBride,
        // Bugs22 #1).
        const APL_Float qc = ceil(quot);
        if (qc != 0.0 && quot > qc - qct)   return IntCell::z0(Z);
        if (qf != 0.0 && quot < qf + qct)   return IntCell::z0(Z);
      }

APL_Float z = value.ival - a * qf;
   if (Cell::is_near_zero(z))   return IntCell::z0(Z);
   if (a < 0.0 && z > 0.0)   z = z + a;
   else if (a > 0.0 && z < 0.0)   z = z + a;
   return FloatCell::zF(Z, z);
}
//════════════════════════════════════════════════════════════════════════════
ErrorCode
IntCell::bif_add_inverse(Cell * Z, const Cell * A) const
{
   return A->bif_subtract(Z, this);
}
ErrorCode
IntCell::bif_multiply_inverse(Cell * Z, const Cell * A) const
{
   return A->bif_divide(Z, this);
}
//────────────────────────────────────────────────────────────────────────────
CellType
IntCell::get_cell_subtype() const
{
   if (value.ival < 0)   // negative integer (only fits in signed containers)
      {
        if (-value.ival <= 0x80)
           return CellType(CT_INT | CTS_S8 | CTS_S16 | CTS_S32 | CTS_S64);

        if (-value.ival <= 0x8000)
           return CellType(CT_INT | CTS_S16 | CTS_S32 | CTS_S64);

        if (-value.ival <= 0x80000000)
           return CellType(CT_INT | CTS_S32 | CTS_S64);

        return CellType(CT_INT | CTS_S64);
      }

   // positive integer
   //
   if (value.ival == 0)   // 0: bit (fits in all containers)
      return CellType(CT_INT | CTS_BIT | CTS_X8  | CTS_S8  | CTS_U8  |
                                         CTS_X16 | CTS_S16 | CTS_U16 |
                                         CTS_X32 | CTS_S32 | CTS_U32 |
                                         CTS_X64 | CTS_S64 | CTS_U64);

   if (value.ival == 1)   // 1: bit (fits in all containers)
      return CellType(CT_INT | CTS_BIT | CTS_X8  | CTS_S8  | CTS_U8  |
                                         CTS_X16 | CTS_S16 | CTS_U16 |
                                         CTS_X32 | CTS_S32 | CTS_U32 |
                                         CTS_X64 | CTS_S64 | CTS_U64);

   if (value.ival <= 0x7F)
      return CellType(CT_INT | CTS_X8  | CTS_S8  | CTS_U8  |
                               CTS_X16 | CTS_S16 | CTS_U16 |
                               CTS_X32 | CTS_S32 | CTS_U32 |
                               CTS_X64 | CTS_S64 | CTS_U64);

   if (value.ival <= 0xFF)
      return CellType(CT_INT |                     CTS_U8  |
                               CTS_X16 | CTS_S16 | CTS_U16 |
                               CTS_X32 | CTS_S32 | CTS_U32 |
                               CTS_X64 | CTS_S64 | CTS_U64);

   if (value.ival <= 0x7FFF)
      return CellType(CT_INT | CTS_X16 | CTS_S16 | CTS_U16 |
                               CTS_X32 | CTS_S32 | CTS_U32 |
                               CTS_X64 | CTS_S64 | CTS_U64);

   if (value.ival <= 0xFFFF)
      return CellType(CT_INT |                     CTS_U16 |
                               CTS_X32 | CTS_S32 | CTS_U32 |
                               CTS_X64 | CTS_S64 | CTS_U64);

   if (value.ival <= 0x7FFFFFFF)
      return CellType(CT_INT | CTS_X32 | CTS_S32 | CTS_U32 |
                               CTS_X64 | CTS_S64 | CTS_U64);

   if (value.ival <= 0xFFFFFFFF)
      return CellType(CT_INT |                     CTS_U32 |
                               CTS_X64 | CTS_S64 | CTS_U64);

   if (value.ival <= 0x7FFFFFFFFFFFFFFFLL)   // note: this is always the case
      return CellType(CT_INT | CTS_X64 | CTS_S64 | CTS_U64);

   return CellType(CT_INT | CTS_U64);
}
//────────────────────────────────────────────────────────────────────────────
// throw/nothrow boundary. Functions above MUST NOT (directly or indirectly)
// throw while funcions below MAY throw.
//────────────────────────────────────────────────────────────────────────────

#include "Error.hh"

//────────────────────────────────────────────────────────────────────────────
bool
IntCell::greater(const Cell & other) const
{
const APL_Integer this_val  = get_int_value();

   switch(other.get_cell_type())
      {
        case CT_INT:
             {
               const APL_Integer other_val = other.get_int_value();
               if (this_val == other_val)   return this > &other;
               return this_val > other_val;
             }

        case CT_FLOAT:
             {
               const APL_Float other_val = other.get_real_value();
               if (this_val == other_val)   return this > &other;
               return this_val > other_val;
             }

        case CT_COMPLEX: break;
        case CT_CHAR:    return true;    // int is always greater than char
        case CT_POINTER: return false;   // int is always smaller than nested
        case CT_CELLREF: DOMAIN_ERROR;
        default:         Assert(0 && "Bad celltype");
      }

const Comp_result comp = compare(other);
   if (comp == COMP_EQ)   return this > &other;
   return (comp == COMP_GT);
}
//────────────────────────────────────────────────────────────────────────────
Comp_result
IntCell::compare(const Cell & other) const
{
   if (other.is_integer_cell())
      {
       if (value.ival == other.get_int_value())   return COMP_EQ;
       return (value.ival < other.get_int_value()) ? COMP_LT : COMP_GT;
      }

   if (other.is_numeric())   // float or complex
      {
        return Comp_result(-other.compare(*this));
      }

   if (other.is_character_cell())   return COMP_GT;   // numeric > char
   if (other.is_pointer_cell())     return COMP_LT;   // numeric < nested
   DOMAIN_ERROR;
}
//────────────────────────────────────────────────────────────────────────────
PrintBuffer
IntCell::character_representation(const PrintContext & pctx) const
{
   if (pctx.get_scaled())
      {
        FloatCell fc(get_int_value());
        return fc.character_representation(pctx);
      }

char cc[40];
   SPRINTF(cc, "%lld", long_long(value.ival));

UCS_string ucs;

   loop(c, sizeof(cc))
       {
         const char q = cc[c];
         if (q == 0)   break;
         if (q == '-')   ucs << UNI_OVERBAR;
         else            ucs << Unicode(q);
       }

ColInfo info;
   info.flags |= CT_INT;
   info.real_len = ucs.size();
   info.int_len = ucs.size();

   return PrintBuffer(ucs, info);
}
//────────────────────────────────────────────────────────────────────────────
int
IntCell::get_byte_value() const
{
   if (value.ival < -128)
      {
        MORE_ERROR() << "integer " << value.ival
                     << " is too small (< -128) for a byte value";
        DOMAIN_ERROR;
      }

   if (value.ival > 255)
      {
        MORE_ERROR() << "integer " << value.ival
                     << " is too large (> 255) for a byte value";
        DOMAIN_ERROR;
      }

   return value.ival;
}
//────────────────────────────────────────────────────────────────────────────
int
IntCell::CDR_size() const
{
   // use 4 byte for small integers and 8 bytes for others (converted to float).
   //
   // was: only val==0 or val==0xFFFFFFFF (4294967295 as a signed 64-bit
   // APL_Integer, not -1) got 4 bytes; every other integer -- including
   // ordinary small ones like 1, 2, or -5 -- fell through to 8, which
   // contradicts the comment's own stated intent and the sibling
   // range check already used for the same "fits in CDR's 32-bit slot"
   // decision elsewhere (cValue.cc's get_CDR_type()). Fixed to test the
   // actual signed 32-bit range.
   //
const APL_Integer val = get_int_value();

   if (val >= -0x80000000LL && val <= 0x7FFFFFFFLL)   return 4;
   return 8;
}
//════════════════════════════════════════════════════════════════════════════
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_add_ii(Cell * Z, APL_Integer a, APL_Integer b)
{
   // a + b directly would be signed overflow (UB) exactly when
   // sum_overflow() below is about to say so (Blake McBride,
   // Bugs12.md #11a). Compute via uint64_t, whose overflow is
   // well-defined wraparound, then reinterpret -- same bit pattern
   // two's-complement a + b would have produced anyway.
   //
const APL_Integer z = APL_Integer(uint64_t(a) + uint64_t(b));
   if (Cell::sum_overflow(z, a, b))
      return FloatCell::zF(Z, APL_Float(a) + APL_Float(b));
   return IntCell::zI(Z, z);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_subtract_ii(Cell * Z, APL_Integer a, APL_Integer b)
{
   // see bif_add_ii() above.
   //
const APL_Integer z = APL_Integer(uint64_t(a) - uint64_t(b));
   if (Cell::diff_overflow(z, a, b))
      return FloatCell::zF(Z, APL_Float(a) - APL_Float(b));
   return IntCell::zI(Z, z);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_multiply_ii(Cell * Z, APL_Integer a, APL_Integer b)
{
const APL_Float prod = APL_Float(a) * APL_Float(b);
   if (prod > LARGE_INT || prod < SMALL_INT)   return FloatCell::zF(Z, prod);
   return IntCell::zI(Z, a * b);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_divide_ii(Cell * Z, APL_Integer a, APL_Integer b)
{
   if (b == 0)
      {
        if (a == 0)   return IntCell::z1(Z);
        return E_DOMAIN_ERROR;
      }

   // a / b overflows (hardware #DE / SIGFPE) for the single combination
   // b == -1 && a == INT64_MIN, since -INT64_MIN is not representable
   // as APL_Integer. Confirmed directly: ¯9223372036854775808 ÷ ¯1
   // crashes the interpreter. Handle b == -1 up front (negation, with
   // the INT64_MIN sub-case promoted to float) instead of dividing.
   if (b == -1)
      {
        if (a == APL_Integer(0x8000000000000000ULL))   // INT64_MIN
           return FloatCell::zF(Z, -APL_Float(a));
        return IntCell::zI(Z, -a);
      }

const APL_Integer i_quot = a / b;
   if (a != i_quot * b)   return FloatCell::zF(Z, a / APL_Float(b));
   return IntCell::zI(Z, i_quot);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_power_ii(Cell * Z, APL_Integer a, APL_Integer b)
{
const bool invert_Z = b < 0;
   if (invert_Z)   b = -b;

   if (b == 0)   return IntCell::z1(Z);   // A⋆0 = 1

   if (b == 1)
      {
        if (invert_Z)
           {
             switch(a)
                {
                  case  0: return E_DOMAIN_ERROR;
                  case  1: return IntCell::z1(Z);
                  case -1: return IntCell::z_1(Z);
                  default: return FloatCell::zF(Z, 1.0 / a);
                }
           }
        return IntCell::zI(Z, a);
      }

   // b >= 2
   //
   if (a == 0)
      {
        if (invert_Z)   return E_DOMAIN_ERROR;
        return IntCell::z0(Z);
      }
   if (a == 1)    return IntCell::z1(Z);
   if (a == -1)   return (b & 1) ? IntCell::z_1(Z) : IntCell::z1(Z);
   if (a == APL_Integer(0x8000000000000000ULL))   // INT64_MIN
      {
        // -a below would overflow right back to INT64_MIN (UB, and stays
        // negative), which later makes a_2_n wrap to 0 and SIGFPEs on the
        // division at line ~822. pow() handles a negative base directly
        // (correct sign for odd/even b), so go straight to the float path.
        APL_Float z = pow(APL_Float(a), APL_Float(b));
        if (invert_Z)   z = 1.0 / z;
        return FloatCell::zF(Z, z);
      }

const bool negate_Z = (a < 0) && (b & 1);
   if (a < 0)   a = -a;

bool overflow = false;
APL_Integer a_2_n = a;
APL_Integer zi = 1;
   for (APL_Integer b1 = b; b1; b1 >>= 1)
      {
        if (b1 & 1)
           {
             if (uint64_t(zi) >= 0x7FFFFFFFFFFFFFFFULL / a_2_n)
                {
                  overflow = true;
                  break;
                }
             zi *= a_2_n;
             if (b1 == 1)   break;
           }

        if (a_2_n >= 100000000LL)
           {
             overflow = true;
             break;
           }
        a_2_n *= a_2_n;
      }

   if (!overflow)
      {
        if (negate_Z)    zi = -zi;
        if (!invert_Z)   return IntCell::zI(Z, zi);
        if (zi != 0)     return FloatCell::zF(Z, 1.0 / zi);
        return E_DOMAIN_ERROR;
      }

   // overflow: use float
   //
APL_Float z = pow(APL_Float(a), APL_Float(b));
   if (negate_Z)   z = -z;
   if (invert_Z)   z = 1.0 / z;
   if (!isfinite(z))   return E_DOMAIN_ERROR;   // 2⋆4096 gave ∞
   return FloatCell::zF(Z, z);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_maximum_ii(Cell * Z, APL_Integer a, APL_Integer b)
{
   return IntCell::zI(Z, a >= b ? a : b);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_minimum_ii(Cell * Z, APL_Integer a, APL_Integer b)
{
   return IntCell::zI(Z, a <= b ? a : b);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_residue_ii(Cell * Z, APL_Integer a, APL_Integer b)
{
   if (a == 0)   return IntCell::zI(Z, b);
   if (b == 0)   return IntCell::z0(Z);

   // b % a overflows (hardware #DE / SIGFPE) for a == -1 && b ==
   // INT64_MIN, the same INT64_MIN/-1 overflow as bif_divide_ii above.
   // Confirmed directly: ¯1 | ¯9223372036854775808 crashes the
   // interpreter. Residue by +-1 is always 0, so this is a cheap
   // early-out rather than a special divide.
   if (a == 1 || a == -1)   return IntCell::z0(Z);

APL_Integer rest = b % a;
   if (a < 0)
      {
        if (rest > 0)   rest += a;
      }
   else
      {
        if (rest < 0)   rest += a;
      }
   return IntCell::zI(Z, rest);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_ceiling_i(Cell * Z, APL_Integer b)
{
   return IntCell::zI(Z, b);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_floor_i(Cell * Z, APL_Integer b)
{
   return IntCell::zI(Z, b);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_conjugate_i(Cell * Z, APL_Integer b)
{
   return IntCell::zI(Z, b);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_direction_i(Cell * Z, APL_Integer b)
{
APL_Integer result = 0;
   if      (b > 0)   result =  1;
   else if (b < 0)   result = -1;
   return IntCell::zI(Z, result);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_exponential_i(Cell * Z, APL_Integer b)
{
const APL_Float z = exp(APL_Float(b));
   if (!isfinite(z))   return E_DOMAIN_ERROR;   // ⋆1000 gave ∞
   return FloatCell::zF(Z, z);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_factorial_i(Cell * Z, APL_Integer b)
{
   if (b < 0)                    return E_DOMAIN_ERROR;
   if (b < int_fact_max)       return IntCell::zI(Z, int_factorials[b]);
   if (b < float_fact_max)     return FloatCell::zF(Z, float_factorials[b]);
   return E_DOMAIN_ERROR;
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_magnitude_i(Cell * Z, APL_Integer b)
{
   if (b >= 0)   return IntCell::zI(Z, b);
   if (uint64_t(b) == 0x8000000000000000ULL)
      return FloatCell::zF(Z, -APL_Float(b));
   return IntCell::zI(Z, -b);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_nat_log_i(Cell * Z, APL_Integer b)
{
   if (b == 0)   return E_DOMAIN_ERROR;
   if (b > 0)    return FloatCell::zF(Z, log(APL_Float(b)));
   return ComplexCell::zC(Z, log(APL_Complex(b, 0)));
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_negative_i(Cell * Z, APL_Integer b)
{
   if (uint64_t(b) == 0x8000000000000000ULL)
      return FloatCell::zF(Z, -APL_Float(b));
   return IntCell::zI(Z, -b);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_pi_times_i(Cell * Z, APL_Integer b)
{
   return FloatCell::zF(Z, M_PI * b);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_pi_times_inverse_i(Cell * Z, APL_Integer b)
{
   return FloatCell::zF(Z, b / M_PI);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_reciprocal_i(Cell * Z, APL_Integer b)
{
   switch(b)
      {
        case  0: return E_DOMAIN_ERROR;
        case  1: return IntCell::z1(Z);
        case -1: return IntCell::z_1(Z);
      }
   return FloatCell::zF(Z, 1.0 / b);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_roll_i(Cell * Z, APL_Integer b)
{
   if (b <= 0)   return E_DOMAIN_ERROR;
const uint64_t rnd = Workspace::get_RL(b);
   return IntCell::zI(Z, Workspace::get_IO() + APL_Integer(rnd % b));
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_near_int64_t_i(Cell * Z, APL_Integer b)
{
   return IntCell::zI(Z, b);
}
//────────────────────────────────────────────────────────────────────────────
ErrorCode
IntCell::bif_within_quad_CT_i(Cell * Z, APL_Integer b)
{
   return IntCell::zI(Z, b);
}
