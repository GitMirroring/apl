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
    Cmd_HOST:: implements host/file interaction: )HOST, ]NEXTFILE
    (+ have_capability), and ]PUSHFILE.
*/

#include "config.h"

#include <errno.h>
#include <string.h>

#include "Cmd_HOST.hh"
#include "Common.hh"
#include "InputFile.hh"
#include "IO_Files.hh"
#include "Sys.hh"
#include "UserPreferences.hh"

//════════════════════════════════════════════════════════════════════════════
void
Cmd_HOST::cmd_HOST(ostream & out, const UCS_string & arg)
{
   if (UserPreferences::uprefs.safe_mode)
      {
        out <<
"This interpreter was started in \"safe mode\" (command line option --safe,\n"
"see ⎕ARG). The APL command )HOST is not permitted in safe mode." << endl;
        return;
      }

UTF8_string host_cmd(arg);
PipeReader reader(host_cmd.c_str());
   if (!reader)   // popen() failed
      {
        out << ")HOST command failed: " << strerror(errno) << endl;
        return;
      }

   for (;;)
       {
         const int cc = reader.fgetc();
         if (cc == EOF)   break;
         out << char(cc);
       }

   errno = 0;
const int result = reader.close();
   Log(LOG_verbose_error)
      {
        if (result)   CERR << "NOTE: pclose(" << arg << ") says: errno="
                           << errno << " (" << strerror(errno) << ")" << endl;
      }

   out << endl << result << ' ' << endl;
}
//────────────────────────────────────────────────────────────────────────────
void
Cmd_HOST::cmd_NEXTFILE(ostream & out, const UCS_string_vector & args)
{
   loop(a, args.size())
       {
         const UCS_string & arg = args[a];

         // figure HAVE- or NO- prefix and capability
         //
         if (arg.starts_iwith("HAVE-"))
            {
              if (! have_capability(arg.drop(5)))   return;
            }
         else if (arg.starts_iwith("NO-"))
            {
              if (have_capability(arg.drop(3)))   return;
            }
         else
            {
              CERR << "]NEXTFILE: unknown capability '" << arg
                   << "' (ignored).\n"
                      "Capabilities start with HAVE- or with NO- "
                      "(try TAB expansion)."
                   << endl;
              continue;   // next arg
            }
       }

   IO_Files::next_file();
}
//────────────────────────────────────────────────────────────────────────────
bool
Cmd_HOST::have_capability(const UCS_string & capa)
{
const int len = capa.size();
   if (capa.front() == UNI_Quad_Quad)   // ⎕xx
      {
        const UCS_string capa1 = capa.drop(1);
        if (len == 4 && capa1.starts_iwith("FFT"))       return apl_FFT;
        if (len == 4 && capa1.starts_iwith("PNG"))       return apl_PNG;
        if (len == 3 && capa1.starts_iwith("RE"))        return apl_PCRE;
      }
   else
      {
        if (len == 3 && capa.starts_iwith("GTK"))        return apl_GTK3;
        if (len == 3 && capa.starts_iwith("GUI"))        return apl_GUI;
        if (len == 8 && capa.starts_iwith("POSTGRES"))   return apl_POSTGRES;
        if (len == 7 && capa.starts_iwith("SQLITE3"))    return apl_SQLITE3;
        if (len == 3 && capa.starts_iwith("X11"))        return apl_X11;
      }

   CERR << "]NEXTFILE: Unknown capability '" << capa << "'" << endl;
   return false;
}
//────────────────────────────────────────────────────────────────────────────
void
Cmd_HOST::cmd_PUSHFILE()
{
   CERR <<
"*** Pushing an immediate execution context (leave it with ]NEXTFILE)"
        << endl;

   if (InputFile::files_todo.size())
      InputFile::files_todo.front().set_pushed_pending(true);

InputFile fam("stdin", stdin, false, true, true, no_LX);
   fam.set_pushed_IE();
   InputFile::files_todo.insert(InputFile::files_todo.begin(), fam);
}
//════════════════════════════════════════════════════════════════════════════
