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

#ifndef __MACRO_HH_DEFINED__
#define __MACRO_HH_DEFINED__

#include "UserFunction.hh"

//════════════════════════════════════════════════════════════════════════════
/// a system function or operator implemented as an internal defined function
class Macro : public UserFunction
{
public:
   /// the unique number of a macro
   enum Macro_num
      {
#define mac_def(name, _txt) MAC_ ## name,   ///< a Macro_num
#include "Macro.def"
        MAC_COUNT,
        MAC_NONE = -1
      };

   /// A reference to a μ-prefixed macro symbol, used once macro-internal
   /// names (the macro's own name aside, see Macro_num above) become
   /// positional rather than named. A single value doubles as both "which
   /// macro" and "which argument slot" depending on its sign: negative
   /// values MUE_M1 .. MUE_M29 identify one of the 29 macros (mirroring
   /// Macro_num's 29 MAC_xxx entries, one-to-one, in the same order), and
   /// positive values MUE_1 .. MUE_17 identify one of the (at most 17)
   /// positional argument/local-var slots within whichever macro is
   /// current -- 17 being the highest count over all macros (in
   /// Z__pA_LO_REDUCE_X4_B and Z__nA_LO_REDUCE_X4_B: Z A1 LO X4 B rho_B3
   /// B3 rho_Z rho_Z3 T H M L a N I I_max). 0 is deliberately unused (ne
   /// either a valid macro nor a valid slot) so it stays available as an
   /// "unset" sentinel distinct from every real Mue_num value.
   ///
   enum Mue_num
      {
        MUE_M29 = -29,   ///< macro 29 (Z__EXEC_EACH_B)
        MUE_M28 = -28,   ///< macro 28 (Z__vA_LO_INNER_RO_vB)
        MUE_M27 = -27,   ///< macro 27 (Z__A_Quad_EB_B)
        MUE_M26 = -26,   ///< macro 26 (Z__A_Quad_EA_B)
        MUE_M25 = -25,   ///< macro 25 (Z__Quad_INP_B)
        MUE_M24 = -24,   ///< macro 24 (Z__A_LO_RANK_X7_B)
        MUE_M23 = -23,   ///< macro 23 (Z__LO_RANK_X5_B)
        MUE_M22 = -22,   ///< macro 22 (Z__LO_SCAN_X4_B)
        MUE_M21 = -21,   ///< macro 21 (Z__nA_LO_REDUCE_X4_B)
        MUE_M20 = -20,   ///< macro 20 (Z__pA_LO_REDUCE_X4_B)
        MUE_M19 = -19,   ///< macro 19 (Z__LO_REDUCE_X4_B)
        MUE_M18 = -18,   ///< macro 18 (Z__A_LO_POWER_RO_B)
        MUE_M17 = -17,   ///< macro 17 (Z__LO_POWER_RO_B)
        MUE_M16 = -16,   ///< macro 16 (Z__A_LO_POWER_N_B)
        MUE_M15 = -15,   ///< macro 15 (Z__LO_POWER_N_B)
        MUE_M14 = -14,   ///< macro 14 (A_LO_POWER_N_B)
        MUE_M13 = -13,   ///< macro 13 (LO_POWER_N_B)
        MUE_M12 = -12,   ///< macro 12 (Z__A_LO_INNER_RO_B)
        MUE_M11 = -11,   ///< macro 11 (Z__A_LO_OUTER_B)
        MUE_M10 = -10,   ///< macro 10 (Z__vA_LO_EACH_vB)
        MUE_M9  =  -9,   ///< macro  9 (Z__vA_LO_EACH_sB)
        MUE_M8  =  -8,   ///< macro  8 (Z__sA_LO_EACH_vB)
        MUE_M7  =  -7,   ///< macro  7 (Z__sA_LO_EACH_sB)
        MUE_M6  =  -6,   ///< macro  6 (Z__LO_EACH_B)
        MUE_M5  =  -5,   ///< macro  5 (vA_LO_EACH_vB)
        MUE_M4  =  -4,   ///< macro  4 (vA_LO_EACH_sB)
        MUE_M3  =  -3,   ///< macro  3 (sA_LO_EACH_vB)
        MUE_M2  =  -2,   ///< macro  2 (sA_LO_EACH_sB)
        MUE_M1  =  -1,   ///< macro  1 (LO_EACH_B)

        // 0 intentionally unused (see comment above)

        MUE_1   =   1,   ///< argument/local-var slot 1
        MUE_2   =   2,   ///< argument/local-var slot 2
        MUE_3   =   3,   ///< argument/local-var slot 3
        MUE_4   =   4,   ///< argument/local-var slot 4
        MUE_5   =   5,   ///< argument/local-var slot 5
        MUE_6   =   6,   ///< argument/local-var slot 6
        MUE_7   =   7,   ///< argument/local-var slot 7
        MUE_8   =   8,   ///< argument/local-var slot 8
        MUE_9   =   9,   ///< argument/local-var slot 9
        MUE_10  =  10,   ///< argument/local-var slot 10
        MUE_11  =  11,   ///< argument/local-var slot 11
        MUE_12  =  12,   ///< argument/local-var slot 12
        MUE_13  =  13,   ///< argument/local-var slot 13
        MUE_14  =  14,   ///< argument/local-var slot 14
        MUE_15  =  15,   ///< argument/local-var slot 15
        MUE_16  =  16,   ///< argument/local-var slot 16
        MUE_17  =  17,   ///< argument/local-var slot 17
        MUE_18  =  18,   ///< argument/local-var slot 18

        // MMAC_COUNT mirrors Macro_num::MAC_COUNT's value (29, the number
        // of macros) under a different name: Macro_num and Mue_num, both
        // plain enums of this same class, share one enumerator namespace,
        // so re-using the name MAC_COUNT here would be a duplicate-
        // definition error.
        //
        MMAC_COUNT = 29,   ///< the number of macros
        MARG_COUNT = 18,   ///< the number of argument/local-var slots

        // aliases for the 6 argument slots whose position and meaning
        // are consistent across (most) macros, in this fixed order:
        // B (the fundamental right operand, present in every macro),
        // A (the left/count argument, present in the dyadic-flavoured
        // macros), X (the axis/extra parameter X4/X5/X7/N, present in
        // REDUCE/SCAN/RANK/POWER-with-count), Z (the result-communication
        // symbol -- a named slot like B/A, not really an "argument", but
        // treated the same way rather than folded anonymously into the
        // generic locals below), LO and RO (the operator's own operand
        // functions). Z's slot number was chosen to come after LO/RO
        // rather than reordering them (LO=4/RO=5 are already deployed
        // across all 29 macros) -- conceptually it sits between X and LO,
        // per Jürgen Sauermann; numerically it's slot 6. Everything from
        // slot 7 on is just LV_1 .. LV_12 (12 slots, MARG_COUNT - 6).
        //
        MUE_B   = MUE_1,   ///< the right operand
        MUE_A   = MUE_2,   ///< the left/count operand
        MUE_X   = MUE_3,   ///< the axis/extra parameter
        MUE_LO  = MUE_4,   ///< the left operand function
        MUE_RO  = MUE_5,   ///< the right operand function
        MUE_Z   = MUE_6,   ///< the result-communication symbol

        LV_1    = MUE_7,    ///< local var 1
        LV_2    = MUE_8,    ///< local var 2
        LV_3    = MUE_9,    ///< local var 3
        LV_4    = MUE_10,   ///< local var 4
        LV_5    = MUE_11,   ///< local var 5
        LV_6    = MUE_12,   ///< local var 6
        LV_7    = MUE_13,   ///< local var 7
        LV_8    = MUE_14,   ///< local var 8
        LV_9    = MUE_15,   ///< local var 9
        LV_10   = MUE_16,   ///< local var 10
        LV_11   = MUE_17,   ///< local var 11
        LV_12   = MUE_18,   ///< local var 12
      };

   /// true once mue_symbols is fully populated (set at the end of
   /// init()). SymbolTable::lookup_symbol() consults this before taking
   /// its μ¯N/μN fast path: init() itself looks up those very same names
   /// (via Workspace::lookup_symbol(), i.e. through that same function)
   /// while filling mue_symbols in, and must not have its own lookups
   /// short-circuited back to the not-yet-assigned array slot it is in
   /// the middle of filling -- false (the default until init() finishes)
   /// keeps those particular lookups going through the ordinary
   /// (real-symbol-table) path, exactly as before this feature existed.
   static bool mue_ready;

   /// initialize mue_symbols. Must run BEFORE the macros themselves are
   /// ⎕FX-equivalently defined (static_Objects.cc's mac_def expansion,
   /// which constructs the 29 static Macro objects below): each Macro
   /// constructor parses its own APL source text right there and then,
   /// which already does its own (unrelated, μ-prefixed-name) symbol
   /// lookups, so mue_symbols needs to already be in place before that
   /// point if anything defined while ⎕FX-ing a macro is ever going to
   /// be able to reference it.
   static void init();

   /// Symbol for every Mue_num value. Indexed as: [0] = 0 (no symbol,
   /// Mue_num 0 is unused); [1 .. MMAC_COUNT] = the macro-name symbols
   /// μ¯1 .. μ¯29 (for MUE_M1 .. MUE_M29, i.e. index = -Mue_num);
   /// [MMAC_COUNT+1 .. MMAC_COUNT+MARG_COUNT] = the argument/local-var
   /// symbols μ1 .. μ17 (for MUE_1 .. MUE_17, i.e. index = Mue_num +
   /// MMAC_COUNT).
   static Symbol * mue_symbols[MMAC_COUNT + MARG_COUNT + 1];

   /// constructor
   /// @param num unique macro identifier
   /// @param text UTF-8 encoded APL source text of the macro body
   Macro(Macro_num num, const UTF8_string & text);

   /// overloaded UserFunction::get_macnum()
   virtual int get_macnum() const
      { return macro_number; }

   /// overloaded Function::is_macro()
   virtual bool is_macro() const   { return true; }

   /// return the macro with \b macro_number num
   /// @param num macro identifier to look up
   static Macro * get_macro(Macro_num num);

   /// unmark all values used in the bodies of all macros
   static void unmark_all_macros();

#define mac_def(name, _txt) static Macro name;   ///< a macro
#include "Macro.def"

   /// a vector of all macros
   static Macro * all_macros[MAC_COUNT];

protected:
   /// A (compile-time) unique number for this macro
   const Macro_num macro_number;

private:
   /// destructor (not supposed to be called)
   ~Macro();
};
//════════════════════════════════════════════════════════════════════════════

#endif // __MACRO_HH_DEFINED__

