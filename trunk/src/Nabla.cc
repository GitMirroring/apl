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

#include <climits>

#include "Command.hh"
#include "InputFile.hh"
#include "LineInput.hh"
#include "Logging.hh"
#include "Nabla.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "Symbol.hh"
#include "UserFunction.hh"
#include "UserPreferences.hh"
#include "Workspace.hh"

//════════════════════════════════════════════════════════════════════════════
void
LineLabel::print(ostream & out) const
{
UCS_string ucs;
   ucs << UNI_L_BRACK << ln_major;
   if (ln_minor.size())   ucs << UNI_FULLSTOP << ln_minor;
   ucs << UNI_R_BRACK; 

   while (ucs.size() < 5)   ucs << UNI_SPACE;
   out << ucs;
}
//────────────────────────────────────────────────────────────────────────────
UCS_string
LineLabel::print_prompt(int min_size) const
{
UCS_string ret;
   ret << UNI_L_BRACK << ln_major;

   if (ln_minor.size())
      {
        ret << UNI_FULLSTOP;
        loop(s, ln_minor.size())   ret << Unicode(char(ln_minor[s]));
      }

   ret << UNI_R_BRACK << UNI_SPACE;
   while (ret.ssize() < min_size)   ret << UNI_SPACE;
   return ret;
}
//────────────────────────────────────────────────────────────────────────────
bool
LineLabel::operator ==(const LineLabel & other) const
{
   return (ln_major == other.ln_major) && (ln_minor == other.ln_minor);
}
//────────────────────────────────────────────────────────────────────────────
bool
LineLabel::operator <(const LineLabel & other) const
{
   if (ln_major != other.ln_major)   return ln_major < other.ln_major;
 return  ln_minor.compare(other.ln_minor) < 0;
}
//────────────────────────────────────────────────────────────────────────────
void
LineLabel::next()
{
   if (ln_minor.size() == 0)   // full number: add 1
      {
        ++ln_major;
        return;
      }

   // fract number: increment last fract digit
   //
const Unicode cc = ln_minor[ln_minor.size() - 1];
   if (cc != UNI_9)   ln_minor[ln_minor.size() - 1] = Unicode(cc + 1);
   else                     ln_minor << UNI_1;
}
//────────────────────────────────────────────────────────────────────────────
void
LineLabel::insert()
{
   ln_minor << UNI_1;
}
//════════════════════════════════════════════════════════════════════════════
ostream &
operator <<(ostream & out, const LineLabel & lab)
{
   lab.print(out);
   return out;
}
//════════════════════════════════════════════════════════════════════════════
void
Nabla::edit_function(const UCS_string & cmd)
{
Nabla nabla(cmd);
   nabla.edit();
}
//────────────────────────────────────────────────────────────────────────────
UCS_string
Nabla::get_label_and_text(int line, bool & is_current) const
{
const FunLine & fl = lines[line];
   is_current = fl.label == current_line;
   return fl.get_label_and_text();
}
//────────────────────────────────────────────────────────────────────────────
Nabla::Nabla(const UCS_string & cmd)
   : defn_line_no(InputFile::current_line_no()),
     fun_symbol(0),
     ecmd(ECMD_NOP),
     edit_from(-1),
     edit_to(-1),
     function_existed(false),
     modified(false),
     do_close(false),
     locked(false),
     out_of_order(false),
     current_line(1),
     first_command(cmd)
{
   Workspace::more_error().clear();
}
//────────────────────────────────────────────────────────────────────────────
void
Nabla::throw_edit_error(const char * why)
{
UCS_string command = first_command;
   if (trailing_nabla)   command << UNI_NABLA;

   COUT << "DEFN ERROR+" << endl
        << "      " << command << endl
        << "      " << UCS_string(command.size() - 1, UNI_SPACE)
        << "^" << endl;

   if (Workspace::more_error().size() == 0)   MORE_ERROR() << why;

   Error::throw_define_error(fun_header, first_command, why);
}
//────────────────────────────────────────────────────────────────────────────
void
Nabla::edit()
{
   if (const char * error = start())
      {
        Log(LOG_verbose_error)
           {
             if (Workspace::more_error().size() == 0)
                {
                  UERR << "Bad ∇-open '" << first_command
                       << "' : '" << error << "'" << endl;
                }
           }
        throw_edit_error(error);
      }

   // editor loop
   //
int control_D_count = 0;
   Log(LOG_nabla)   UERR << "Nabla(" << fun_header << ")..." << endl;
try_again:
   while (!do_close)
       {
         const UCS_string prompt = current_line.print_prompt(0);
         bool eof = false;
         UCS_string line;
         if (UserPreferences::uprefs.raw_cin)
            {
              LineHistory lh(10);
              InputMux::get_line(LIM_Nabla, prompt, line, eof, lh);
            }
         else
            {
              LineHistory lh(*this);
              InputMux::get_line(LIM_Nabla, prompt, line, eof, lh);
            }

         if (eof)   // end-of-input (^D) pressed
            {
              ++control_D_count;
              if (control_D_count < 5)
                 {
                    COUT << "^D" << endl;
                    continue;
                 }
               COUT << endl << "      *** end of input" << endl;
               Command::cmd_OFF(5);
            }

         if (const char * loc = parse_oper(line, false))
            {
              UERR << "??? " << loc << endl;
              continue;
            }

         try
            {
              if (const char * loc = execute_oper())
                 {
                   UERR << "∇-command failed: " << loc << endl;
                   Output::set_color_mode(Output::COLM_INPUT);
                 }
            }
         catch (Error & err)
            {
              // running_script() alone misses -T testcase files
              // (is_validating(): files_todo[0].is_script is false
              // there) -- without also checking it here, the new
              // throw_edit_error() escalation in edit_body_line() above
              // gets silently caught and swallowed right here instead
              // of escaping this while loop, and the loop just keeps
              // reading (and misinterpreting) whatever comes next.
              //
              if (InputFile::running_script() ||
                  InputFile::is_validating())   throw;
              if (!err.get_print_loc())   err.print_em(COUT, LOC);
              Output::set_color_mode(Output::COLM_INPUT);
            }
       }

   Log(LOG_nabla)
      {
        UERR << "done: '" << fun_header << "'" << endl;
        loop(l, lines.size())   UERR << lines[l].text << endl;
      }

UCS_string fun_text;
   loop(l, lines.size())
      {
        fun_text << lines[l].text << UNI_LF;
      }

   // maybe copy function into the history
   //
   if (!UserPreferences::uprefs.raw_cin &&
       !InputFile::running_script() &&
       ((UserPreferences::uprefs.nabla_to_history ==  /* always     */ 2) ||
        ((UserPreferences::uprefs.nabla_to_history == /* if changed */ 1) &&
       modified)))
      {
        // create a history entry that can be re-entered and replace
        // the last history line (which contained some ∇foo ...)
        //
        {
          UCS_string line_0(U"    ");
          line_0 << UNI_NABLA << lines[0].text;
          line_0.remove_trailing_whitespaces();
          LineInput::replace_history_line(line_0);
        }

        for (size_t l = 1; l < lines.size(); ++l)
            {
              UCS_string line_l;
              line_l << UNI_L_BRACK << int(l) << UNI_R_BRACK
                     << UNI_SPACE << UNI_SPACE;
              while (line_l.size() < 6)   line_l << UNI_SPACE;
              line_l << lines[l].text;
              line_l.remove_trailing_whitespaces();

              LineInput::add_history_line(line_l);
           }

        UCS_string line_N(U"   ");
        line_N << UNI_NABLA;
        LineInput::add_history_line(line_N);
      }

int error_line = 0;
char creator[APL_PATH_MAX+20];
   SPRINTF(creator, "%s:%d", InputFile::current_filename(), defn_line_no)
const UTF8_string creator_utf8(creator);

UserFunction * ufun;
   try
      {
        ufun = UserFunction::fix(fun_text, error_line,
                                 /* keep_existing */ false,
                                 LOC, creator_utf8);
      }
   catch (Error &)
      {
        ufun = 0;
      }
   catch (std::bad_alloc &)
      {
        ufun = 0;
        WS_FULL;
      }
   catch (...)
      { FIXME; }

   if (ufun == 0)
      {
        const UCS_string & MORE = Workspace::more_error();
        if (InputFile::running_script())
           {
             // the ∇-editor runs from a script, therefore warning the user
             // interactively and asking to fix the fault makes no sense. We
             // therefore exit with DEFN_ERROR, so that the script does not
             // hang in a endless try again loop.
             //
             UTF8_string more_utf8(MORE);
             throw_edit_error(more_utf8.c_str());
           }

        if (error_line == -1)   /// unknown error line
           {
             COUT <<
             MORE << 
"\nFatal error in defined function.\n"
"    You may want to change faulty line(s)\n"
"    or cancel editing entirely with:   [→]∇\n";
           }
        else
           {
             COUT <<
             MORE << "\nFatal error in defined function line [" << error_line
                  << "]. To fix this you may want to:\n"
                     "    change the faulty line with:    ["
                  << error_line << "] ..., or \n"
                     "    delete the faulty line with:    [∆"
                  << error_line << "], or\n"
                     "    cancel editing entirely with:   [→]∇.\n";
           }
        do_close = false;
        goto try_again;
      }

   if (locked)
      {
         const int exec_properties[4] = { 1, 1, 1, 1 };
         ufun->set_exec_properties(exec_properties);
      }

   // set stop and trace vectors
   //
std::vector<Function_Line> stop_vec;
std::vector<Function_Line> trace_vec;
   loop(l, lines.size())
       {
         if (lines[l].stop_flag)    stop_vec.push_back(Function_Line(l));
         if (lines[l].trace_flag)   trace_vec.push_back(Function_Line(l));
       }

   ufun->set_trace_stop(stop_vec,  true);
   ufun->set_trace_stop(trace_vec, false);

   // set_trace_stop() calls parse_body(), so we have to redo optimizations
   //
   ufun->optimize_labels();
   ufun->optimize_label_vectors();
   ufun->compute_if_else_targets();
}
//────────────────────────────────────────────────────────────────────────────
void
Nabla::FunLine::print(ostream & out) const
{
   label.print(out);

   // print a space unless text is a label or a comment
   //
   if (!text.is_comment_or_label())   out << " ";
   out << text << endl;
}
//────────────────────────────────────────────────────────────────────────────
UCS_string
Nabla::FunLine::get_label_and_text() const
{
UCS_string ret = label.print_prompt(6);
   return ret << text;
}
//────────────────────────────────────────────────────────────────────────────
const char *
Nabla::start()
{
   /* This function parses member 'first_command', which is the line with
      which the ∇-editor was started. There are 4 valid cases:

      1a. ... FUN[X]   where X is an axis (and therefore FUN is new)
      1b. ... FUN ...  where FUN is new
      1c. ... FUN ...  where FUN exists, but in script
      2.  ... FUN ...  where FUN exists

      Cases 1a. and 1b. create a new function.
      Case  1c. overwrites an existing function.
      Case  2.  starts editing of an existing function.

      start() returns 0 on success or a short error description on failure.
    */

   // skip (and remember) trailing ∇ (if any)
   //
   first_command.remove_trailing_whitespaces();
   trailing_nabla = false;
   if (first_command.back() == UNI_NABLA)
      {
        first_command.pop_back();
        trailing_nabla = true;
        do_close = true;
      }

UCS_string::iterator c(first_command);

   // skip any leading spaces
   //
   c.skip_white();

   // skip the leading nabla. This should always succeed.
   //
   if (c.has_more() && c.next() != UNI_NABLA)   return "Bad ∇-command (no ∇)";

   // skip leading spaces
   //
   c.skip_white();

   // function header... Copy anything, but stop at '[' (if present).
   //
   while (c.has_more() && c.lookup() != UNI_L_BRACK)
         fun_header << c.next();
   c.skip_white();

   /* at this point there could be an axis specification [X] or
      a ∇-operation like [⎕] or [N]...

      For example:

          ∇FOO[X] B   : the '[' starts an axis specification
          ∇FOO[⎕]     : the '[' starts a ∇-operation [⎕] (display)
          ∇FOO[2] ⍴B  : the '[' starts a ∇-operation [2] (change line 2)

      We first handle (and rule out) the axis case
    */
   if (c.has_more()              &&
       c.lookup() == UNI_L_BRACK &&
       is_axis(c.rest()))
      {
        /* new function header, i.e. not a ∇-command. Restore the symbol name
           name left of [ ]. The possible cases are

                 FOO [...]
               A FOO [...]
             (LO)FOO [...]
           A (LO)FOO [...]

           move backwards, skipping the whitespace between FOO and [...]
         */
        size_t end = c.get_pos();
        while (end && first_command[end - 1] == UNI_SPACE)   --end;
        size_t pos = end;
        while (pos && Avec::is_symbol_char(first_command[pos-1]))   --pos;
        const UCS_string function_name(first_command, pos, end - pos);
        fun_symbol = Workspace::lookup_symbol(function_name);

        /* Copy the rest to fun_header. After that, fun_header is
           the entire first_command, but with its leading and
           trailing ∇s (and whitespaces) removed.
         */
        while (c.has_more())   fun_header << c.next();
        if (InputFile::running_script())   // case 1a.
           {
             if (const char * loc = open_new_function())   return loc;
             return 0;
           }
      }

   /* at this point fun_header is either:

      (a) the entire header of a new function, or else
      (b) (only) the function name of a supposedly existing function. In
          this case c.rest() shall be empty or an ∇-command.

      The name of the function to be edited could be anywhere in 'fun_header'.
      We therefore parse 'fun_header' into 'hdr' and then fetch the name from
      'hdr'.
    */
bool hdr_has_vars;
   {
     const UserFunction_header hdr(fun_header, /* macro= */ false);
     if (hdr.get_error())
        {
           static char cc[200];
           SPRINTF(cc, "Bad function header: %s", hdr.get_error_info());
           return cc;
        }

     hdr_has_vars = hdr.has_vars();
     fun_symbol = Workspace::lookup_symbol(hdr.get_name());
     Assert(fun_symbol);
   }

   if (fun_header.size() == 0)   return "no function name";

   // optional ∇-operation. The case where '[' starts a header was already
   // handled above.
   //
   if (c.has_more() && c.lookup() == UNI_L_BRACK)
      {
        UCS_string oper;
        Unicode cc;
        do
          {
            if (!c.has_more())   return "no ] in ∇-command";
            oper << (cc = c.next());
          } while (cc != UNI_R_BRACK);

        if (const char * loc = parse_oper(oper, true))   return loc;   // error
      }

   switch(fun_symbol->get_NC())
      {
        case NC_UNUSED_USER_NAME:   // open a new function
             // this is the case where a new function or operator shall be
             // defined, and the new function or operator has no axis.
             //
             modified = true;
             {
               // a new function must not have a command
               //
               if (ecmd != ECMD_NOP)   return "∇-command in new function";

               // case 1b.
               if (const char * loc = open_new_function())   return loc;
             }
             break;

        case NC_FUNCTION:
        case NC_OPERATOR:           // open an existing function
             // Bugs28 #51: case 1c. (unconditionally overwrite with a
             // brand-new, empty function) was taken whenever running
             // in a script, even when parse_oper() just above had
             // already recognised a genuine ∇-command ([⎕], [N],
             // [N⎕], [∆N], [→]) on the EXISTING function -- so e.g.
             // the classic "display F" idiom, ∇F[⎕]∇, silently
             // replaced F with an empty niladic function in script
             // mode (while working correctly on stdin/interactively).
             // Case 1c. is only actually intended for a genuine new
             // function HEADER (ecmd==ECMD_NOP, i.e. no ∇-command was
             // parsed) -- an actual ∇-command means the user is editing
             // the EXISTING function, exactly as in interactive mode.
             //
             // Roy Tobin, 2026-09-26: the #51 fix above additionally
             // required hdr_has_vars (the header carries a result,
             // argument(s) or operand(s), e.g. "Z←F B") to reach case
             // 1c., on the theory that a bare function name always means
             // an existing-function edit. That is true only
             // interactively; a script/)COPY providing a COMPLETE new
             // "∇NAME ... ∇" definition looks identical at the header
             // line (bare name, ecmd==ECMD_NOP) whether NAME is niladic
             // or not, so requiring hdr_has_vars wrongly routed every
             // niladic/no-result function's fresh redefinition into
             // open_existing_function(), which preloads and then keeps
             // the OLD body -- each )COPY of the same file appended
             // another copy of the function's lines instead of replacing
             // them. ecmd==ECMD_NOP already excludes the bracket-command
             // case #51 cared about, so hdr_has_vars added nothing there
             // and is dropped.
             //
             if (InputFile::running_script() &&
                 ecmd == ECMD_NOP)   // script, new header
                {
                  // case 1c.
                  if (const char * loc = open_new_function())   return loc;
                  break;   // continue below
                }

             // interactive, or a script ∇-command (Bugs28 #51).
             //
             // case 2.
             if (const char * loc =
                       open_existing_function(fun_symbol->get_name()))
                return loc;

             if (hdr_has_vars)
                {
                  // an existing function was opened with a header that
                  // contains more than the function name.
                  //
                  return "attempt to ∇-open existing function with "
                              "a new function header";
                }

             break;

        default:
             return "attempt to ∇-open a non-function at " LOC;
      }

   // at this point the (new or existing) function was successfully opened.
   // That means that at least the header was present (lines.size() > 0)

   // immediate close (only show command is allowed here),
   // e.g. ∇fun[⎕]∇
   //
   if (ecmd == ECMD_EDIT && c.has_more())
      {
        /* For example:  ∇FOO[1]123∇

           We have parsed ∇FOO[1] and c.rest() is the line to be replaced,
           optionally followed by a closing ∇. We copy c.rest() into
           current_text and then 
         */
         while (c.has_more())   current_text << c.next();
         return execute_oper();
      }

   if (c.has_more())
      {
        if (ecmd == ECMD_NOP)    return 0;
        if (ecmd != ECMD_SHOW)   return "illegal command between ∇ ... ∇";
        return execute_oper();
      }

   return execute_oper();
}
//════════════════════════════════════════════════════════════════════════════
const char *
Nabla::parse_oper(UCS_string & oper, bool initial)
{
   Log(LOG_nabla)
      UERR << "parsing oper '" << oper << "'" << endl;

   // skip trailing spaces
   //
   oper.remove_trailing_whitespaces();
   if (oper.size() > 0 && oper.back() == UNI_NABLA)
      {
        do_close = true;
        oper.pop_back();
        while (oper.size() > 0 && oper.back() <= ' ')   oper.pop_back();
      }
   else if (oper.size() > 0 && oper.back() == UNI_DEL_TILDE)
      {
        do_close = true;
        locked = true;
        oper.pop_back();
        while (oper.size() > 0 && oper.back() <= ' ')   oper.pop_back();
      }

   current_text.clear();
   ecmd = ECMD_NOP;

   if (oper.size() == 0 && do_close)   return 0;

UCS_string::iterator c(oper);
Unicode cc = c.has_more() ? c.next() : Invalid_Unicode;
UCS_string text = oper;
   while (cc == ' ')   cc = c.next();   // skip leading whitespace

   // we expect one of the following:
   //
   // [⎕] [n⎕] [⎕m] [n⎕m] [⎕n-m]                    (show)
   //     [n∆] [∆m] [n∆m] [∆n-m] [∆n1 n2 ...]       (delete)
   // [→]                                           (escape)
   // [n]                                           (goto)
   // text                                          (override text)

   if (cc != UNI_L_BRACK)   // override text
      {
        ecmd = ECMD_EDIT;
        edit_from = current_line;
        current_text = text;
//      for (; c.has_more(); cc = c.next())   current_text <<cc;
        return 0;
      }

   // a loop over multiple commands, like
   // [2⎕4] [∆5]
   //
   // only the last command is executed; the previous commands are discarded/
   //
command_loop:

   // at this point, [ was seen and skipped

   ecmd = ECMD_NOP;
   edit_from.clear();   // set to missing
   edit_to.clear();     // set to missing
   got_minus = false;   // set to missing

   // set optional edit_from (if present)
   //
   if (c.has_more() && (Avec::is_digit(c.lookup()) ||    // N.M
                    c.lookup() == UNI_FULLSTOP))     //  .M
      {
        edit_from = parse_lineno(c);
      }

   // operation, which is one of:
   //
   // [⎕   show
   // []   edit
   // [∆   delete
   // [→   abandon
   //
   if (!c.has_more())   return "Bad ∇-command";
   switch (c.lookup())
      {
        case UNI_Quad_Quad:
        case UNI_Quad_Quad1:    ecmd = ECMD_SHOW;     c.next();   break;
        case UNI_R_BRACK:       ecmd = ECMD_EDIT;                 break;
        case UNI_DELTA:         ecmd = ECMD_DELETE;   c.next();   break;
        case UNI_RIGHT_ARROW:   ecmd = ECMD_ESCAPE;   c.next();   break;

        default: UERR << "Bad edit op '" << c.lookup() << "'"
                      << " in line " << current_line << endl;
                 return "Bad ∇-command";
      }

   // don't allow escape in the first command
   if (initial)
      {
        if (ecmd == ECMD_ESCAPE)   return "Bad initial ∇-command →";
      }

again:
   // set optional edit_to (if present)
   //
   if (c.has_more() && Avec::is_digit(c.lookup()))   edit_to = parse_lineno(c);

   if (c.has_more() && c.lookup() == UNI_MINUS)   // range
      {
        if (got_minus)   return "error: second -  in ∇-range";
        got_minus = true;
        edit_from = edit_to;   // shift
        c.next();   // consume the -
        goto again;
      }

   if (c.has_more() && c.next() != UNI_R_BRACK)   return "missing ] in ∇-range";

   // at this point we have parsed an editor command, like:
   //
   // [from ⎕ to]
   // [from ∆ to]
   // [from]

   c.skip_white();

   if (c.has_more() &&
       c.lookup() == UNI_L_BRACK)   // another command: ignore the previous one
      {
         c.next();   // eat the [
         goto command_loop;
      }

   // copy the rest to current_text. Set do_close if ∇ or ⍫ is seen
   // unless inside strings.
   //
   while (c.has_more())
      {
        switch(cc = c.next())
           {
             case UNI_NABLA:           // ∇
                  do_close = true;
                  return 0;

             case UNI_DEL_TILDE:       // ⍫
                  locked = true;
                  do_close = true;
                  return 0;

             case UNI_DOUBLE_QUOTE:  // "
                  current_text << cc;
                  for (;;)
                      {
                        if (!c.has_more())   // premature end of input
                           {
                             current_text << UNI_DOUBLE_QUOTE;
                             return 0;
                           }
                        cc = c.next();

                        current_text << cc;
                        if (cc == UNI_DOUBLE_QUOTE)   break; // string end
                        if (cc == UNI_BACKSLASH)      // \x
                           {
                             if (!c.has_more())   // premature end of input
                                {
                                  current_text << UNI_BACKSLASH
                                               << UNI_DOUBLE_QUOTE;
                                  return 0;
                                }
                             cc = c.next();
                             current_text << cc;
                           }
                      }
                  break;

             case UNI_SINGLE_QUOTE:    // '
                  current_text << cc;
                  for (;;)
                      {
                        // no need to care for ''. Since we only copy, we can
                        // handle ' ... '' ... ' like two adjacent strings
                        // instead of a string containing a (doubled) quote.
                        //
                        if (!c.has_more())   // premature end of input
                           {
                             current_text << UNI_SINGLE_QUOTE;
                             return 0;
                           }
                        cc = c.next();
                        current_text << cc;
                        if (cc == UNI_SINGLE_QUOTE)      break;   // string end
                      }
                  break;

             default:
                current_text << cc;
           }
      }

   return 0;   // OK
}
//════════════════════════════════════════════════════════════════════════════
const char *
Nabla::execute_oper()
{
   if (ecmd == ECMD_NOP)
      {
        Log(LOG_nabla)
           UERR << "Nabla::execute_oper(NOP)" << endl;
        return 0;
      }

const bool have_from = edit_from.ln_major != -1;
const bool have_to = edit_to.ln_major != -1;

   if (lines.size())
      {
        if (!have_from && ecmd != ECMD_DELETE)   edit_from = lines[0].label;
        if (!have_to)     edit_to = lines[lines.size() - 1].label;
      }

   if (ecmd == ECMD_SHOW)     return execute_show();
   if (ecmd == ECMD_DELETE)   return execute_delete();
   if (ecmd == ECMD_EDIT)     return execute_edit();
   if (ecmd == ECMD_ESCAPE)   return execute_escape();

   UERR << "edit command " << ecmd
        << " from " << edit_from << " to " << edit_to << endl;
   FIXME;

   return LOC;
}
//────────────────────────────────────────────────────────────────────────────
const char *
Nabla::execute_show()
{
   Log(LOG_nabla)
      UERR << "Nabla::execute_oper(SHOW) from " << edit_from
           << " to " << edit_to << " line-count " << lines.size() << endl;

int idx_from = find_line(edit_from);
int idx_to   = find_line(edit_to);

const LineLabel user_edit_to = edit_to;

   if (idx_from == -1)   edit_from = lines[idx_from = 0].label;
   if (idx_to == -1)     edit_to   = lines[idx_to = lines.size() - 1].label;

   Log(LOG_nabla)
      UERR << "Nabla::execute_oper(SHOW) from "
           << edit_from << " to " << edit_to << endl;

   if (idx_from == 0)                     // then print header line
      COUT << "    ∇" << endl;
   for (int e = idx_from; e <= idx_to; ++e)   lines[e].print(COUT);
   if (idx_to == int(lines.size() - 1))   // then print last line
      {
        // LRM pp.346,349: the closing ∇ of a full [⎕]-style display is
        // followed by the function's creation time stamp
        // (LanguageVariances.md #47) -- only when this is an already-
        // established function (function_existed, matching how
        // 2⎕AT/⎕NC already report it elsewhere), not one still being
        // defined for the first time in this very editing session.
        //
        cFunction_P fun = function_existed ? fun_symbol->get_function() : 0;
        if (fun && fun->get_creation_time())
           {
             COUT << "    ∇  ";
             Workspace::get_v_Quad_TZ().print_timestamp(COUT,
                                          fun->get_creation_time()) << endl;
           }
        else
           {
             COUT << "    ∇" << endl;
           }
      }

   if (user_edit_to.valid())   // eg. [⎕42] or [2⎕42]
      {
        current_line = user_edit_to;
        if (line_exists(current_line))   current_line.next();
        if (line_exists(current_line))
           {
             // user_edit_to and its next line exist. Increase minor length
             // That is, if both [8] and [9] exist then use [8.1]
             //
             current_line = user_edit_to;
             current_line.insert();
           }
      }
   else                      // eg. [42⎕] or [⎕]
      {
        current_line = lines.back().label;
        current_line.next();
      }

   Log(LOG_nabla)
      UERR << "Nabla::execute_oper(SHOW) done with current_line '"
           << current_line << "'" << endl;

   return 0;
}
//────────────────────────────────────────────────────────────────────────────
const char *
Nabla::execute_delete()
{
   // for delete we want exact numbers.
   //
const int idx_to = find_line(LineLabel(edit_to));
   if (idx_to == -1)   return "Bad line number N in [M∆N] ";

   modified = true;

   if (edit_from == -1)   // [∆N] : delete single line
      {
        // lines[0] is the header line; nothing upstream stops N from
        // resolving to it, and deleting it would desync every later
        // line index from its label. Reject rather than corrupt.
        if (idx_to == 0)   return "cannot delete the header line [0]";
        lines.erase(lines.begin() + idx_to);
        return 0;
      }

   // [N∆M] : delete multiple lines
   //
const int idx_from = find_line(LineLabel(edit_from));
   if (idx_from == -1)       return "Bad line number M in [M∆N] ";
   if (idx_from >= idx_to)   return "M ≥ N in [M∆N] ";
   if (idx_from == 0)        return "cannot delete the header line [0]";

   loop(j, 1 + idx_to - idx_from)   lines.erase(lines.begin() + idx_from);
   // guard defensively: were lines ever emptied above, .back() is UB.
   if (lines.size())   current_line = lines.back().label;
   return 0;
}
//────────────────────────────────────────────────────────────────────────────
const char *
Nabla::execute_edit()
{
   // if the user has not specified a line then edit at current_line
   //
   if (edit_from.ln_major == -1)   edit_from = current_line;

   current_line = edit_from;

   // if the user has specified a line label without a text then we are done
   //
   if (current_text.size() == 0)   // empty line (we MAY need it)
      {
        if (!UserPreferences::uprefs.new_multi_line_strings)
           return 0;   // we do not
        if (out_of_order)
           return 0;   // we do not;
      }

   // check that current_text is valid
   //
   if (current_line.is_header_line_number())   return edit_header_line();
   else                                        return edit_body_line();
}
//────────────────────────────────────────────────────────────────────────────
const char *
Nabla::edit_header_line()
{
   // parse the header and check that it is valid
   //
UserFunction_header header(current_text, false);
   if (header.get_error() != E_NO_ERROR)
      {
        CERR << "BAD FUNCTION HEADER";
        COUT << endl;
        return 0;
      }

   // check if the function name has changed
   //
   Assert(fun_symbol);
const UCS_string & old_name = fun_symbol->get_name();
const UCS_string & new_name = header.get_name();
   if (old_name != new_name)
      {
        // the name has changed. This is OK if the new name can be edited.
        //
        // the old symbol shall not be ⎕FXed when closing the editor, so we
        // can simply forget it and continue with the new function
        //
        // We need to check that it is not a variable or an existing function.
        //
        Symbol * sym = Workspace::lookup_symbol(new_name);   // create if needed
        Assert(sym);
        if (sym->get_NC() != NC_UNUSED_USER_NAME)
           {
             CERR << "BAD FUNCTION HEADER";
             COUT << endl;
             return 0;
           }
      }

   modified = true;

   lines[0].text = current_text;
   current_line.next();
   return 0;
}
//────────────────────────────────────────────────────────────────────────────
const char *
Nabla::edit_body_line()
{
UCS_string parse_text = current_text;   // a copy that can be modified.
   if (UserPreferences::uprefs.new_multi_line_strings)
      {
        // figure the multi-line status from all lines before current_line
        Multiline_status multi = MLS_APL_text;
        loop(i, lines.size())
            {
              if (current_line == lines[i].label)   break;   // line replace
              if (current_line <  lines[i].label)   break;   // after current
              if (-1 != lines[i].text.multi_pos())
                 {
                   if (multi <= MLS_APL_text)   multi = MLS_Inside_multi;
                   else                         multi = MLS_APL_text;
                 }
            }

        if (multi >= MLS_Start_of_multi)   // line belongs to a multi-line
           {
             parse_text.clear();
           }
       else          // APL code outside multi-line strings (or start of one)
           {
             const int pos = parse_text.multi_pos();
             if (pos != -1)   // start of multi-line string (""" or «««)
                {
                  // for the sole purpose of parsing: replace the start of the
                  // multi-line string (""" or »»») with the empty string ''.
                  //
                  parse_text.resize(pos);
                  parse_text << UNI_SINGLE_QUOTE << UNI_SINGLE_QUOTE;
                }
           }
      }

   if (parse_text.size())
      {
        const Parser parser(PM_FUNCTION, LOC, false);
        Token_string in;

        ErrorCode ec = E_NO_ERROR;
        try   { ec = parser.parse(parse_text, in, true); }
        catch (const Error & err)   { ec = err.get_error_code(); }

        if ((ec == E_NO_STRING_END) &&
            UserPreferences::uprefs.old_multi_line_strings)
           {
             ec = E_NO_ERROR;
             Workspace::more_error().clear();
           }

        if (ec)
           {
             // Staying in the editor loop so an interactive user can
             // retry the same line is correct -- but from a script (or
             // a -T testcase file: is_validating(), not running_script()
             // -- confirmed live that -T's files_todo entries have
             // is_script false) there is no user to fix it, and the
             // caller's while loop (Nabla::edit()) would otherwise keep
             // reading and discarding whatever comes next from the
             // input stream as further retry attempts for this same
             // broken line, forever (or until a lone ∇ happens to
             // appear). Confirmed live: a bracket-mismatch SYNTAX ERROR
             // on a ∇-edited body line, from a -T testcase, silently
             // swallowed the opening line and first body line of the
             // NEXT function definition in the same test run into this
             // stuck retry loop instead of executing them
             // (devel_doc/Carets.txt gap #1's Nabla.cc note). Nabla::
             // edit() itself already has this exact InputFile::
             // running_script() escalation for a sibling failure (the
             // whole function failing to compile once fully typed, see
             // below) -- apply the same one here too (extended to also
             // cover -T), before the per-line retry gets a chance to
             // consume anything else.
             //
             if (InputFile::running_script() || InputFile::is_validating())
                throw_edit_error("SYNTAX ERROR in function line");

             CERR << "SYNTAX ERROR";
             if (Workspace::more_error().size())
                {
                  CERR << "+" << endl << Workspace::more_error();
                }
             COUT << endl;
             return 0;
           }
      }

   // some users prefer the removal of leading and trailing whitespace
   //
   if (UserPreferences::uprefs.discard_indentation)
      current_text.remove_leading_and_trailing_whitespaces();

   modified = true;

const int idx_from = find_line(edit_from);

   Assert(lines.size() > 0);
   if (idx_from == -1)   // new line
      {
        // find the largest label before edit_from (if any)
        //
        int before_idx = -1;
        loop(i, lines.size())
            {
              if (lines[i].label < edit_from)   before_idx = i;
              else                              break;
            }

        FunLine fl(edit_from, current_text);
        lines.insert(lines.begin() + before_idx + 1, fl);
      }
   else
      {
        // replace line
        //
        lines[idx_from].text = current_text;
      }

   current_line.next();

   return 0;
}
//────────────────────────────────────────────────────────────────────────────
const char *
Nabla::execute_escape()
{
   // the user has entered [→].
   //
   // Note that fun_symbol and fun_symbol->get_function() may both be valid
   // even though the function is "fresh". We use function_existed instead.
   //
   lines.clear();


   if (function_existed)   // existing function
      {
        Assert(fun_symbol);
        const Function * fun = fun_symbol->get_function();
        Assert(fun);
        const UserFunction * ufun = fun->get_func_ufun();
        Assert(ufun);
        loop(l, ufun->get_text_size())
            {
              const UCS_string & fun_line = ufun->get_text(l);
              lines.push_back(FunLine(l, fun_line));
            }
      }
   else       // new function: only restore the header
      {
        lines.push_back(FunLine(0, fun_header));
        current_line = LineLabel(1);
      }

   return 0;
}
//────────────────────────────────────────────────────────────────────────────
const char *
Nabla::open_new_function()
{
   Log(LOG_nabla)
      UERR << "creating new function with header '"
           << fun_header << "'" << endl;

   // check for lambda
   //
   if (fun_header.contains(UNI_LAMBDA))
       {
         CERR << "\n*** WARNING: trying to edit a lambda? Don't!***" << endl;
       }

   function_existed = false;
   lines.push_back(FunLine(0, fun_header));
   return 0;
}
//────────────────────────────────────────────────────────────────────────────
const char *
Nabla::open_existing_function(const UCS_string & name)
{
   Log(LOG_nabla)
      UERR << "opening existing function '" << name << "'" << endl;

   function_existed = true;
   if (const char * why = fun_symbol->cant_be_defined())   return why;

cFunction_P function = fun_symbol->get_function();
   Assert(function);

   if (function->get_exec_properties()[0])
      return "function is locked";

   if (function->is_native())
      return "function is native";

   if (function->is_lambda())
      return "function is a lambda";

   if (Workspace::is_called(fun_symbol->get_name()))
      return "function is used, pendent or suspended";

const UserFunction * ufun = function->get_func_ufun();
   if (ufun == 0)
      return "function is not editable at " LOC;

const UCS_string ftxt = function->canonical(false);
   Log(LOG_nabla)   UERR << "existing function is:\n" << ftxt << endl;

UCS_string_vector tlines;
   ftxt.to_vector(tlines);

   Assert(tlines.size());
   fun_header = tlines[0];
   loop(t, tlines.size())
       {
         FunLine fl(t, tlines[t]);

         // set stop and trace flags
         //
         loop(st, ufun->get_stop_lines().size())
             {
               if (t == ufun->get_stop_lines()[st])   // stop set
                  {
                    fl.stop_flag = true;
                    break;
                  }
             }

         loop(tr, ufun->get_trace_lines().size())
             {
               if (t == ufun->get_trace_lines()[tr])   // trace set
                  {
                    fl.trace_flag = true;
                    break;
                  }
             }

         lines.push_back(fl);
       }

   current_line = LineLabel(tlines.size());

   return 0;
}
//────────────────────────────────────────────────────────────────────────────
bool
Nabla::is_axis(const UCS_string & ucs)
{
   // ucs was checked to start with with '['. Return true iff the '['
   // is the start of an axis specification such as [ Name ]
   //
UCS_string::iterator c(ucs);
   Assert(c.has_more() && c.lookup() == UNI_L_BRACK);   // [
   c.next();   // skip '['
   c.skip_white();
   if (!c.has_more())                           return false;    // not an axis
   if (!Avec::is_first_symbol_char(c.next()))   return false;    // not an axis
   while (c.has_more() && Avec::is_symbol_char(c.lookup()))   c.next();
   c.skip_white();
   if (!c.has_more() || c.lookup() != UNI_R_BRACK)   return false;
   c.next();   // skip ']'

   // Bugs28 #100(y): a bracket-axis is only ever valid on a header that
   // also has a right argument B -- UserFunction_header::init_signature()
   // only recognises [X] after first stripping a trailing B off the
   // token list, so a niladic "F[X]" with no B is not, and never was, a
   // representable axis header. Without this check, a purely structural
   // [Name] match here mistook a single-line ∇-editor delete command
   // like [∆3] (∆ is a legal identifier-start character, and a
   // digit-only suffix is a legal identifier continuation, so "∆3"
   // parses as one ordinary name) for a new function's axis parameter,
   // pre-empting parse_oper() -- which already handles [∆N] correctly
   // -- and producing a DEFN ERROR instead of deleting line N. If
   // nothing meaningful follows ']', this cannot be a real axis header,
   // so it must be left for parse_oper() to interpret as a command.
   //
   c.skip_white();
   return c.has_more();
}
//════════════════════════════════════════════════════════════════════════════
LineLabel
Nabla::parse_lineno(UCS_string::iterator & c)
{
LineLabel ret(0);

   while (c.has_more() && Avec::is_digit(c.lookup()))
      {
        const int digit = c.next() - UNI_0;
        // detect overflow *before* it happens (signed overflow is UB):
        // an arbitrarily long digit run (e.g. [99999999999999999999])
        // would otherwise overflow ln_major -- mirrors the marker-index
        // parse in Tokenizer.cc.
        if (ret.ln_major > (INT_MAX - digit) / 10)
           ret.ln_major = INT_MAX;
        else
           ret.ln_major = 10 * ret.ln_major + digit;
      }

   if (c.has_more() && c.lookup() == UNI_FULLSTOP)
      {
        c.next();   // eat the .
        while (c.has_more() && Avec::is_digit(c.lookup()))
              ret.ln_minor << c.next();
      }

   return ret;
}
//────────────────────────────────────────────────────────────────────────────
int
Nabla::find_line(const LineLabel & lab) const
{
   if (lab.ln_major == -1)   return -1;

   loop(l, lines.size())   if (lab == lines[l].label)   return l;

   return -1;   // not found.
}
//════════════════════════════════════════════════════════════════════════════

