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

#include "IndexExpr.hh"
#include "PrintBuffer.hh"
#include "PrintOperator.hh"
#include "Value.hh"
#include "Workspace.hh"

//════════════════════════════════════════════════════════════════════════════
IndexExpr::IndexExpr(Assign_state astate, const char * loc)
   // M28 (Blake's report, Bugs6 #5): switching this to
   // DynamicObject(loc, &all_index_exprs) -- so erase_stale()/)CHECK's
   // "no stale indices" diagnostic would actually do something, instead
   // of being a permanent no-op -- was tried TWICE and reverted TWICE,
   // both times after a real SIGSEGV in ~IndexExpr() during
   // Prefix::clean_up() at )CLEAR (Quad_SYL.tc).
   //
   // First revert: at the time, DynamicObject had no destructor at all,
   // so any IndexExpr deleted through a path other than
   // IndexExpr::erase_stale()'s own ring walk (e.g. Prefix::clean_up()'s
   // "delete &tok.get_index_val();", or any of the individual delete
   // call sites in Parser.cc) left a dangling entry in all_index_exprs
   // -- exactly the Value/all_values bug fixed as Bugs6 #1, just for
   // IndexExpr.
   //
   // Second attempt (2026-07-30, SVN 2053): added
   // ~DynamicObject(){ unlink(); } for #1, reasoned that this made the
   // ring-membership change safe by the same logic, verified extensively
   // (exact Quad_SYL.tc repro, repeated-error+)SIC stress tests, a
   // suspended-state )SAVE/)LOAD round trip, the full regression suite
   // multiple times) -- all passed locally. Shipped, then a real user
   // (Bill Heagy) hit a SEGSEGV in the exact same place running the full
   // testcase suite (`apl -T testcases/*.tc`), which exercises far more
   // cross-file / cross-testcase workspace state than any of the above
   // targeted tests did. Could not reproduce locally afterwards (targeted
   // 2-file repro of the alphabetically-preceding testcase +
   // Quad_SYL.tc, and a full 269-file suite run under AddressSanitizer,
   // both completed cleanly) or pin down the exact mechanism through
   // further static analysis of Prefix.cc's IndexExpr-handling reduce_*()
   // functions and Prefix::reset()/clean_up(). Reverted again out of
   // caution rather than ship continued uncertainty on a real crash.
   //
   // Left as the original, safe, always-no-op anchor-only form. Do not
   // re-attempt without first getting a locally-reproducible crash (Bill
   // Heagy's exact SVN revision/environment/full testcase-order may
   // matter) -- verifying against a hand-picked subset of testcases is
   // not sufficient, as this history now shows twice over.
   : DynamicObject(loc),
     quad_io(Workspace::get_IO()),
     assign_state(astate),
     rank(0),
     value_count(0)
{
}
//────────────────────────────────────────────────────────────────────────────
IndexExpr::~IndexExpr()
{
   if (value_count)
      {
        loop(r, rank)   if (+values[r])   values[r].reset();
      }
}
//────────────────────────────────────────────────────────────────────────────
void
IndexExpr::check_index_range(const Shape & shape) const
{
   loop(r, rank)
      {
        if (const cValue * ival = get_axis_value(r))   // unless elided index
           {
             const ShapeItem max_idx = shape.get_shape_item(r) + quad_io;
             loop(i, ival->element_count())
                {
                  const APL_Integer idx = ival->get_near_int(i);
                  if (idx < quad_io || idx >= max_idx)   // invalid index
                     {
                       UCS_string & ucs = MORE_ERROR();
                       ucs << "Offending index: " << idx
                           << " (with ⎕IO: " << quad_io << ")\n"
                           << "offending axis:  " << (r + quad_io)
                           << " of some";
                       loop(s, shape.get_rank())
                           ucs << " " << shape.get_shape_item(s);
                       ucs << "⍴...";
                       if (idx >= max_idx)
                          ucs << "\nthe max index for axis "
                              << (r + quad_io)<< " is: "
                              << (max_idx - quad_io);
                       INDEX_ERROR;
                     }
                }
           }
      }
}
//────────────────────────────────────────────────────────────────────────────
sRank
IndexExpr::get_axis(sRank max_axis) const
{
   if (rank != 1)   INDEX_ERROR;

const APL_Integer qio = Workspace::get_IO();

Value_P I = values[0];
   if (!I->is_scalar_or_len1_vector())     INDEX_ERROR;

   if (!I->is_near_int(0))   INDEX_ERROR;

const APL_Integer wide_axis = I->get_near_int(0) - qio;

   // axis and max_axis are both signed, so "axis >= max_axis" alone does
   // not catch a negative axis (same class of bug as cValue.cc's
   // get_single_axis, see H17). This function currently has no callers,
   // but fix it to match the validated pattern used elsewhere. Bounds
   // are checked on the wide (APL_Integer) value *before* narrowing to
   // sRank (int16_t), so a huge out-of-range index cannot wrap into a
   // small in-range sRank first.
   //
   if (wide_axis < 0 || wide_axis >= max_axis)   INDEX_ERROR;

const sRank axis = sRank(wide_axis);

   return axis;
}
//════════════════════════════════════════════════════════════════════════════
Value_P
IndexExpr::extract_axis()
{
   if (rank == 0 || !values[0])    return Value_P();   // no index or [ ]

   Assert1(rank == 1);   // must only be called for [ axis ]
   --value_count;
Value_P ret = values[0];
   values[0].reset();   // decrement owner count
   return ret;
}
//────────────────────────────────────────────────────────────────────────────
void
IndexExpr::erase_stale(const char * loc)
{
   Log(LOG_Value__erase_stale)
      CERR << endl << endl << "erase_stale() called from " << loc << endl;

   for (DynamicObject * dob = all_index_exprs.get_next();
        dob != &all_index_exprs; dob = dob->get_next())
       {
         IndexExpr * idx = dob->pIndexExpr();

         Log(LOG_Value__erase_stale)
            {
              CERR << "Erasing stale IndexExpr:" << endl
                   << "  Allocated by " << idx->alloc_loc << endl;
            }

         dob = dob->get_prev();
         idx->unlink();
         delete idx;
       }
}
//────────────────────────────────────────────────────────────────────────────
int
IndexExpr::print_stale(ostream & out)
{
int count = 0;

   for (const DynamicObject * dob = all_index_exprs.get_next();
        dob != &all_index_exprs; dob = dob->get_next())
       {
         const IndexExpr * idx = dob->pIndexExpr();

         out << dob->where_allocated();

         try           { out << *idx; }
         catch (Error &)      { out << " *** corrupt ***"; }
         catch (std::bad_alloc &) { out << " *** corrupt ***"; WS_FULL; }
         catch (...)          { FIXME; }

         out << endl;

         ++count;
       }

   return count;
}
//════════════════════════════════════════════════════════════════════════════
ostream &
operator <<(ostream & out, const IndexExpr & idx)
{
   out << "[";
   loop(i, idx.get_rank())
      {
        if (i)   out << ";";
        if (const cValue * ival = idx.get_axis_value(i))
           {
             // value::print() may print a trailing LF that we don't want here.
             // We therefore print the index values ourselves.
             const PrintContext pctx = Workspace::get_PrintContext(PR_APL_MIN);
             PrintBuffer pb(*ival, pctx, 0);

             UCS_string ucs(pb, ival->get_rank(), Workspace::get_PW());
             out << ucs;
           }
      }
   return out << "]";
}
//════════════════════════════════════════════════════════════════════════════
