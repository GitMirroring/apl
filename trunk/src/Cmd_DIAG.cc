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
    Cmd_DIAG:: implements the diagnostic/debug commands
    )CHECK, )DOXY, )HISTORY, ]OPTIM, ]PSTAT.
*/

#include "config.h"

#include <errno.h>
#include <fstream>

#include "Cmd_DIAG.hh"
#include "Common.hh"
#include "Doxy.hh"
#include "DynamicObject.hh"
#include "Heapsort.hh"
#include "IndexExpr.hh"
#include "IO_Files.hh"
#include "LineInput.hh"
#include "Performance.hh"
#include "Value.hh"
#include "ValueHistory.hh"
#include "Workspace.hh"

//════════════════════════════════════════════════════════════════════════════
void
Cmd_DIAG::cmd_CHECK(ostream & out, const UCS_string & arg)
{
bool show_OK = true;   // assume more verbose output
   if (arg.size())   // check with argument (supposedly BRIEF).
      {
        UCS_string arg0(arg);
        arg0.remove_leading_and_trailing_whitespaces();
        const UCS_string brief(U"BRIEF");
        show_OK = arg0.compare(brief) != COMP_EQ;   // not BRIEF
        if (show_OK)   // still show_OK
           {
             out << arg << "? Expecting BRIEF." << endl;
             return;

           }
      }
   // 1. erase stale functions from failed ⎕EX
   //
   {
     bool erased = false;
     if (const int stale = Workspace::cleanup_expunged(CERR, erased))
        {
          out << "WARNING - " << stale << " stale functions ("
               << (erased ? "" : "not ") << "erased)" << endl;
        }
     else if (show_OK)
        {
          out << "OK      - no stale functions" << endl;
        }
   }

   // 2. print stale values (if any)
   //
   {
     if (const int stale = Value::print_stale(CERR))
        {
          out << "ERROR   - " << stale << " stale values" << endl;
          IO_Files::apl_error(LOC);
        }
     else if (show_OK)
        {
          out << "OK      - no stale values" << endl;
        }
   }

   // 3. print stale index expressions (if any)
   {
     if (const int stale = IndexExpr::print_stale(CERR))
        {
          out << "ERROR   - " << stale << " stale indices" << endl;
          IO_Files::apl_error(LOC);
        }
     else if (show_OK)
        {
          out << "OK      - no stale indices" << endl;
        }
   }

   // 4. discover duplicate parents. In the old clone() scheme every nested
   //    value has (at most) one parent. In the new clone() scheme, however,
   //    a nested value can be reused and have multiple parents.
   {
#ifndef NEW_CLONE   // old clone
     // 4a. create a { parent = 0, value } vector<val_val> of all values
     //
     std::vector<val_val> val_vals;
     ShapeItem duplicate_parents = 0;
     for (const DynamicObject * obj =
                DynamicObject::get_all_values()->get_next();
          obj != DynamicObject::get_all_values(); obj = obj->get_next())
         {
           const cValue * val = static_cast<const cValue *>(obj);

           val_val vv = { 0, val };   // no parent
           val_vals.push_back(vv);
         }

     // 4b. sort vector<val_val> val_vals by address so that we can search it.
     //
     Heapsort<val_val>::sort(&val_vals.front(), val_vals.size(), 0,
                             &val_val::greater);
     loop(v, (val_vals.size() - 1))
         Assert(&val_vals[v].child < &val_vals[v + 1].child);

      // 4c. set parents of pointer cells
      //
      loop(v, val_vals.size())   // for every .child (acting as parent here)
          {
            const cValue * val = val_vals[v].child;
            const ShapeItem ec = val->nz_element_count();
            loop(e, ec)   // for every ravel cell of the (parent-) value
                {
                  const Cell & cP = val->get_cravel(e);
                  if (!cP.is_pointer_cell())   continue;   // not a parent

                  const cValue * sub = cP.get_pointer_value().get();
                  Assert1(sub);

                  val_val * vvp = reinterpret_cast<val_val *>
                       Heapsort<val_val>::
                        search(sub, val_vals, val_val::compare, 0);
                  Assert(vvp);
                  if (vvp->parent == 0)   // child has no parent (OK)
                     {
                       vvp->parent = val;
                       continue;
                     }

                  // the child already has a parent (which is bad).
                  // print its history to help figuring why.
                  //
                  ++duplicate_parents;
                  out << "Value * vvp=" << voidP(vvp) << " already has parent "
                      << voidP(vvp->parent) << " when checking Value * val="
                      << voidP(vvp) << endl;

                  out << "History of the child:" << endl;
                  VH_entry::print_history(out, *vvp->child, LOC);
                  out << "History of the first parent:" << endl;
                  VH_entry::print_history(out, *vvp->parent, LOC);
                  out << "History of the second parent:" << endl;
                  VH_entry::print_history(out, *val, LOC);
                  out << endl;
               }
          }

     if (duplicate_parents)
          {
            out << "ERROR   - " << duplicate_parents
                << " duplicate parents" << endl;
            IO_Files::apl_error(LOC);
          }
#endif
          {
            if (show_OK)
               out << "OK      - no duplicate parents" << endl;
          }
   }

   // 5. discover strange Cells and counters.
   //
   Value::check_all_Cells(out);
}
//────────────────────────────────────────────────────────────────────────────
void
Cmd_DIAG::cmd_DOXY(ostream & out, const UCS_string_vector & args)
{
UTF8_string root("/tmp");
   if (args.size())   root = UTF8_string(args.front());

   try
     {
       Doxy doxy(out, root);
       doxy.gen();

       if (doxy.get_errors())
          out << "Command ]DOXY failed (" << doxy.get_errors() << " errors)"
              << endl;
      else
         out << "Command ]DOXY finished successfully." << endl
             << "    The generated documentation was stored in directory "
             << doxy.get_root_dir() << endl
             << "    You may now browse it from file://"
             << doxy.get_root_dir()
             << "/index.html" << endl;
     }
   catch (Error &)          {}
   catch (std::bad_alloc &) { WS_FULL; }
   catch (...)              { FIXME; }
}
//────────────────────────────────────────────────────────────────────────────
void
Cmd_DIAG::cmd_HISTORY(ostream & out, const UCS_string & arg)
{
   if (arg.size())   // )HISTORY  CLEAR (or else line filter)
      {
        if (arg.starts_iwith("CLEAR"))   LineInput::clear_history(out);
        else                             LineInput::print_history(out, arg);
      }
   else              // )HISTORY (with no argument/filter)
      {
        UCS_string no_filter;
        LineInput::print_history(out, no_filter);
      }
}
//────────────────────────────────────────────────────────────────────────────
void
Cmd_DIAG::cmd_OPTIM(ostream & out, const UCS_string & arg)
{
   if (arg.starts_iwith("CLEAR"))
      {
        out << "Optimization counters cleared" << endl;
        OptmizationStatistics::reset_all();
        return;
      }

int ulen;
#define optim(ena, opt, text) ulen = 40 + UTF8_string::bytes_chars(text);   \
   out << left << setw(ulen) << text << right << " : ";                     \
   if (ena) out << setw(6) << OptmizationStatistics::get(OPTI_ ## opt);     \
   else     out << "disabled";                                              \
   out << endl;
#include "Performance.def"
}
//────────────────────────────────────────────────────────────────────────────
void
Cmd_DIAG::cmd_PSTAT(ostream & out, const UCS_string & arg)
{
#ifndef cfg_PERFORMANCE_COUNTERS_WANTED
   out << "\n"
<< "Command ]PSTAT is not available, since performance counters were not\n"
"configured for this APL interpreter. To enable performance counters (which\n"
"will slightly decrease performance), recompile the interpreter as follows:"

<< "\n\n"
"   ./configure PERFORMANCE_COUNTERS_WANTED=yes (... "
<< "other configure options"
<< ")\n"
"   make\n"
"   make install (or try: src/apl to test without installing)\n"
"\n"

<< "above the src directory."
<< "\n";

   return;
#endif

   if (arg.starts_iwith("CLEAR"))
      {
        out << "Performance counters cleared" << endl;
        Performance::reset_all();
        return;
      }

   if (arg.starts_iwith("SAVE"))
      {
        const char * filename = "./PerformanceData.def";
        ofstream outf(filename, ofstream::out);
        if (!outf.is_open())
           {
             out << "opening " << filename
                 << " failed: " << strerror(errno) << endl;
             return;
           }

        out << "Writing performance data to file " << filename << endl;
        Performance::save_data(out, outf);
        return;
      }

Pfstat_ID iarg = PFS_ALL;
   if (arg.size() > 0)   iarg = Pfstat_ID(arg.atoi());

   Performance::print(iarg, out);
}
//════════════════════════════════════════════════════════════════════════════
