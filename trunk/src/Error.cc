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

#include "Common.hh"
#include "Error.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "Symbol.hh"
#include "StateIndicator.hh"
#include "UserFunction.hh"
#include "Workspace.hh"

#include "Workspace.icc"

//════════════════════════════════════════════════════════════════════════════
Error::Error(ErrorCode ec, const char * loc)
   : error_code(ec),
     throw_loc(loc),
     parser_loc(0),
     show_locked(false),
     left_caret(-1),
     right_caret(-1),
     print_loc(0)
{
const bool have_more = Workspace::more_error().size();
   if (have_more && Workspace::more_error().back() != UNI_PLUS)
      {
        SPRINTF(error_message_1, "%s%c", error_name(error_code), UNI_PLUS);
      }
   else
      {
        SPRINTF(error_message_1, "%s", error_name(error_code));
      }

   if (have_more && Command::auto_MORE)
      {
        CERR << Workspace::more_error() << endl;
      }

   *symbol_name = 0;
   *error_message_2 = 0;
}
//════════════════════════════════════════════════════════════════════════════
bool
Error::is_known() const
{
   switch(error_code)
      {
   /// the cases
#define err_def(c, _txt, _maj, _min) case  E_ ## c:   return true;
#include "Error.def"
      }

   return false;
}
//────────────────────────────────────────────────────────────────────────────
bool
Error::is_syntax_or_value_error() const
{
   if (error_major(error_code) == 2)   return true;   // some SYNTAX ERROR
   if (error_major(error_code) == 3)   return true;   // VALUE ERROR
   return false;
}
//────────────────────────────────────────────────────────────────────────────
void
Error::add_MORE_indicator(bool have_more)
{
   if (have_more)   // )MORE info available
      {
        const size_t len = strlen(error_message_1);
        if (len && error_message_1[len - 1] != UNI_PLUS &&
            len < sizeof(error_message_1) - 1)
           {
             error_message_1[len]     = '+';
             error_message_1[len + 1] = 0;
           }
      }
}
//────────────────────────────────────────────────────────────────────────────
UCS_string
Error::get_error_line_3() const
{
   if (error_code == E_NO_ERROR)   return UCS_string();   // no error

   if (left_caret < 0)   return UCS_string();             // no ^ position

UCS_string ret;
   if (left_caret > 0)   ret << UCS_string(left_caret, UNI_SPACE);
   ret << UNI_CIRCUMFLEX;

const int diff = right_caret - left_caret;
   if (diff <= 0)   return ret;
   if (diff > 1)   ret << UCS_string(diff - 1, UNI_SPACE);
   ret << UNI_CIRCUMFLEX;

   return ret;
}
//────────────────────────────────────────────────────────────────────────────
void
Error::print_em(ostream & out, const char * loc)
{
   if (print_loc)
      {
        CERR << endl << "*** Error printed twice; first printed at "
             << print_loc << endl
             << "now printed from " << loc << endl;

        return;
      }

   print_loc = loc;

   // an empty line 1 means "no message" (LanguageVariances.md #28,
   // e.g. ⎕ES with an event code that has no defined ⎕ET text) --
   // skip it entirely rather than printing a blank line; ⎕EM itself
   // is unaffected (still a fixed 3-row matrix, see Quad_EM).
   //
const UCS_string line_1 = get_error_line_1();
   if (line_1.size())   out << line_1 << endl;

   out << get_error_line_2() << endl
       << get_error_line_3() << endl;
}
//────────────────────────────────────────────────────────────────────────────
void
Error::print(ostream & out, const char * loc) const
{
   if (print_loc)
      {
        CERR << endl << "*** Error printed twice; first printed at "
             << print_loc << endl
             << "now printed at " << loc << endl;

        return;
      }

   Log(LOG_verbose_error)
      {
        out << endl
            << "--------------------------" << endl;

        if (error_code == E_NO_ERROR)
           {
             out << error_name(E_NO_ERROR) << endl
                 << "--------------------------" << endl;

             return;
           }

        if (*error_message_1)
           {
             out << error_message_1 << endl;
           }
        else
           {
             out << error_name(error_code);
             if (Workspace::more_error().back() != UNI_PLUS)   out << UNI_PLUS;
             out << endl;
           }

        if (parser_loc)   out << "   Parser LOC: " << parser_loc  << endl;
        if (print_loc)    out << "   Print LOC:  " << print_loc   << endl;
        out                   << "   loc:        " << loc         << endl;
        loc = print_loc;

        if (*symbol_name)
           out                << "   Symbol:     " << symbol_name << endl;

        out <<                   "   Thrown at:  " << throw_loc   << endl
            <<                   "--------------------------"     << endl
                                                                  << endl;
      }
}
//────────────────────────────────────────────────────────────────────────────
void
Error::set_error_line_1(const char * msg_1)
{
   strncpy(error_message_1, msg_1, sizeof(error_message_1));
   error_message_1[sizeof(error_message_1) - 1] = 0;
}
//────────────────────────────────────────────────────────────────────────────
void
Error::set_error_line_2(const char * msg_2)
{
   strncpy(error_message_2, msg_2, sizeof(error_message_2));
   error_message_2[sizeof(error_message_2) - 1] = 0;
}
//────────────────────────────────────────────────────────────────────────────
void
Error::set_error_line_2(const UCS_string & ucs, int lcaret, int rcaret)
{
   // line 2 and the carets are meant to be collected exactly once, at
   // the single point (Tokenizer.cc lexical error, Token_string bracket
   // mismatch, ...) that actually knows the precise range -- never
   // blindly overwritten by an outer/later layer that only has a bare
   // ErrorCode to work with. Path A's update_error_info() does NOT use
   // this overload (it seeds line 2 via the 1-arg set_error_line_2() and
   // adjusts the carets incrementally via set_left_caret()/
   // set_right_caret()), so this Assert is not in its way.
   Assert(left_caret == -1);

UTF8_string utf(ucs);
   strncpy(error_message_2, utf.c_str(), sizeof(error_message_2));
   error_message_2[sizeof(error_message_2) - 1] = 0;
   left_caret = lcaret;
   right_caret = rcaret;
}
//────────────────────────────────────────────────────────────────────────────
void
Error::update_error_info(StateIndicator * si)
{
   /*
      construct lines 2 and 3 of the standard 3-line APL error message.
      Line 1 was already constructed in Error::Error(ErrorCode ec...).
      For eample: Let

            ∇FOO
      [1] 1 2 3 ◊ Q←Q++
      [2] ∇

      Then:

            FOO                   ⍝ APL code producing the error
      1 2 3                       ⍝ first (correct) statement
      VALUE ERROR                 ⍝ error line 1
      FOO[1]  Q←Q++               ⍝ error line 2: failed statement
                ^                 ⍝ error line 3: caret line (error range)

       There is no error_message_3 in class Error; the third error line
       is constructed from left_caret and right_caret when needed.

       lrm: "The left caret indicates how far execution of the expression
             progressed before the suspension occurred.
             The right caret indicates the likely point of the error. (On
             occasion, the two carets overlap so that only one is displayed.)"
    */

   // the "      " 6-space literal this used to hardcode here (and
   // below) happens to match Workspace::get_prompt()'s only live value
   // (Workspace.cc's constructor has an alternate, 6-*character* but
   // non-blank "  ∇   " prompt behind a permanently-false "#if 0 &&
   // MINGW_SRC", and a longer, fully commented-out "-----> " one) --
   // derive it from get_prompt() directly instead of duplicating
   // whatever its current value happens to be (user, 2026-08-19).
   //
   {
     const UTF8_string prompt_utf(Workspace::get_prompt());
     set_error_line_2(prompt_utf.c_str());
   }
   set_right_caret(-1);
   set_left_caret(Workspace::get_prompt().size());   // first char after the prompt

   // prepare the second error line (= display of the failed statement)
   //
   if (const UserFunction * ufun = si->get_executable()->get_exec_ufun())
      {
        // ufun->print_line_PCs(LOC);
        // property 1 (nonsuspendable) is inherited from every calling
        // )SI entry too (apl2lrm.txt p.360-361 "or-ing"), not just ufun's
        // own: a function called (directly or transitively) by a
        // nonsuspendable one must behave as nonsuspendable itself.
        //
        if (get_show_locked() || si->get_inherited_exec_property(1))
           {
             {
               const UTF8_string prompt_utf(Workspace::get_prompt());
               set_error_line_2(prompt_utf.c_str());
             }
             set_left_caret(Workspace::get_prompt().size());
             ufun->set_locked_error_info(*this);
             goto out;   // maybe print
           }

        UCS_string ucs(ufun->get_name_and_line(si->get_PC()));
        ucs << UNI_SPACE << UNI_SPACE;
        const UTF8_string utf(ucs);
        set_error_line_2(utf.c_str());
        set_left_caret(ucs.size());
      }

   {
     const Prefix & prefix = si->get_prefix();
     const Function_PC from = prefix.get_range_low();
     const Function_PC to   = prefix.get_range_high();
     const Function_PC2 error_range(from, to);

     si->get_executable()->set_error_info(*this, error_range);
   }

   // print error, unless we are in safe execution mode.
   //
out:
   StateIndicator::get_error(si) = *this;

   // )SI entries below a ⎕ES entry must not print anything but simply return
   for (const StateIndicator * si1 = si; si1; si1 = si1->get_parent())
       {
         if (si1->get_safe_execution_depth()) return;
       }

   print_em(UERR, LOC);
}
//────────────────────────────────────────────────────────────────────────────
const char *
Error::error_name(ErrorCode err)
{
   switch(err)
      {
   /// the cases
#define err_def(c, txt, _maj, _min) \
   case E_ ## c:   return txt;

#include "Error.def"
      }

   return "Unknown Error";
}
//════════════════════════════════════════════════════════════════════════════
/// map \b code to DOMAIN ERROR if property 3 (error conversion) is set on
/// \b si or inherited from any of its calling )SI entries (apl2lrm.txt
/// p.360-361 "or-ing", not just the current function's own -- a function
/// called (directly or transitively) by an error-converting one must
/// convert its own errors the same way). Shared by throw_apl_error() and
/// Error::throw_symbol_error(), which is a separate VALUE_ERROR throw
/// site that does not go through throw_apl_error() at all and therefore
/// needs the same check applied to its own base error code.
static ErrorCode
maybe_convert_to_domain_error(ErrorCode code, StateIndicator * si)
{
   if (si && si->get_inherited_exec_property(3))   return E_DOMAIN_ERROR;
   return code;
}
//════════════════════════════════════════════════════════════════════════════
void
throw_apl_error(ErrorCode code, const char * loc)
{
   ADD_EVENT(0, VHE_Error, code, loc);

StateIndicator * si = Workspace::SI_top();   // the current )SI entry

   Log(LOG_error_throw)
      {
        CERR << endl
             << "throwing " << Error::error_name(code)
             << " at " << loc << endl;
      }

   Log(LOG_verbose_error)
      {
        if (!(si && si->get_safe_execution_depth()))   BACKTRACE
      }

   code = maybe_convert_to_domain_error(code, si);

Error error(code, loc);
   if (si)   error.update_error_info(si);

const Error & eref = error;
   throw eref;
}
//════════════════════════════════════════════════════════════════════════════
void
Error::throw_define_error(const UCS_string & fun_name, const UCS_string & cmd,
                          const char * loc)
{
   // this error is thrown if the ∇-editor detects an invalid function header,
   // for example: "∇FOO 2"

   Log(LOG_error_throw)   
      {   
        CERR << "throwing DEFN ERROR at " << loc
             << " (function is " << fun_name << ")" << endl;
      }

   Log(LOG_verbose_error)     BACKTRACE

Error err(E_DEFN_ERROR, loc);
Error & eref = err;
UTF8_string fun_name_utf(fun_name);
   SPRINTF(err.symbol_name, "%s", fun_name_utf.c_str());

UTF8_string cmd_utf(cmd);   // cmd is something like ∇FUN[⎕]∇
   SPRINTF(err.error_message_2, "PROMPT%s", cmd_utf.c_str());
   eref.left_caret = 5 + cmd.size();   // last Unicode of error_message_2
   if (Workspace::SI_top())   *Workspace::get_error() = eref;
   throw eref;
}
//────────────────────────────────────────────────────────────────────────────
void
Error::throw_parse_error(ErrorCode code, const char * par_loc, const char *loc)
{
   Log(LOG_error_throw)
      {
        CERR << "\nthrowing " << Error::error_name(code)
             << " at " << loc << endl;
      }

   Log(LOG_verbose_error)   BACKTRACE

   // set )MORE error (unless the caller has done so)
   if (Workspace::more_error().size() == 0)   // no )MORE info yet
      {
        MORE_ERROR() << Error::error_name(code);
      }

Error error(code, loc);
   error.parser_loc = par_loc;

// StateIndicator * si = Workspace::SI_top();
// if (si)   error.update_error_info(si);

const Error & eref = error;
   throw eref;
}
//────────────────────────────────────────────────────────────────────────────
void
Error::throw_parse_error(ErrorCode code, const UCS_string & line,
                         int start, int end, const char * par_loc,
                         const char * loc)
{
   Log(LOG_error_throw)
      {
        CERR << "\nthrowing " << Error::error_name(code)
             << " at " << loc << endl;
      }

   Log(LOG_verbose_error)   BACKTRACE

   // set )MORE error (unless the caller has done so)
   if (Workspace::more_error().size() == 0)   // no )MORE info yet
      {
        MORE_ERROR() << Error::error_name(code);
      }

Error error(code, loc);
   error.parser_loc = par_loc;

   if (start < 0)                    start = 0;
   if (end < start)                  end = start;
   if (end > int(line.size()))       end = int(line.size());

UCS_string fragment;
   for (int p = start; p < end; ++p)   fragment << line[p];
   error.set_error_line_2(fragment, 0, fragment.size());

const Error & eref = error;
   throw eref;
}
//────────────────────────────────────────────────────────────────────────────
void
Error::throw_symbol_error(const UCS_string & sym_name, const char * loc)
{
   Log(LOG_error_throw)   
      {   
        CERR << "throwing VALUE ERROR at " << loc;
        if (sym_name.size())   CERR << " (symbol is: '" << sym_name << "')"; 
        CERR << endl;
      }

   // Workspace::SI_top() can return null (empty SI stack, e.g. no
   // statement/function currently suspended) -- the unguarded
   // dereference below crashed in that case; SI_top() is correctly
   // null-checked a few lines further down in this same function for
   // exactly this reason. An empty SI is trivially "not in ⎕ES", so
   // treat it the same as get_safe_execution_depth()==0.
   if (Workspace::more_error().size() == 0 &&   // no )MORE info provided, and
       (!Workspace::SI_top() ||
        Workspace::SI_top()->get_safe_execution_depth() == 0))   // and not in ⎕ES
      {
        char extra[20] = { 0 };
        if (sym_name.size() == 1 && sym_name[0] >= 0x80)
           SPRINTF(extra, " (U+%4.4X)", sym_name[0]);
        MORE_ERROR() << "Offending symbol: " << sym_name << extra;
      }

   Log(LOG_verbose_error)     BACKTRACE

StateIndicator * si = Workspace::SI_top();

Error err(maybe_convert_to_domain_error(E_VALUE_ERROR, si), loc);
UTF8_string sym_name_utf(sym_name);
   SPRINTF(err.symbol_name, "%s", sym_name_utf.c_str());

   if (si)   err.update_error_info(si);   // )SI not empty

const Error & eref = err;
   throw eref;
}
//════════════════════════════════════════════════════════════════════════════

