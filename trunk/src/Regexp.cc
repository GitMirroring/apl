/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright © 2017 Elias Mårtenson

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

#include "Workspace.hh"
#include "Regexp.hh"

//════════════════════════════════════════════════════════════════════════════
static const PCRE2_UCHAR32 *
ucs_to_codepoints(const UCS_string & string)
{
int size = string.size();
PCRE2_UCHAR32 * buf = new PCRE2_UCHAR32[size];
    loop(i, size)   buf[i] = string[i];
    return buf;
}
//════════════════════════════════════════════════════════════════════════════
RegexpMatch::RegexpMatch(pcre2_code * code, const UCS_string & B,
                         PCRE2_SIZE start, uint32_t extra_options)
   : matched_B(B)
{
   match_data = pcre2_match_data_create_from_pattern_32(code, NULL);
   if (match_data == 0)   // allocation failure
      {
        MORE_ERROR() << "pcre2_match_data_create_from_pattern_32() failed";
        DOMAIN_ERROR;
      }

   // Bugs28 #98: the pattern is compiled with PCRE2_UTF (Regexp.cc,
   // pcre2_compile_32()), so pcre2_match_32() re-validates the UTF-32
   // encoding of the *entire* subject (0..B.size(), not just from
   // start) on every single call unless told not to -- an O(B.size())
   // cost paid again for every match of a global (⎕RE['g']) search, on
   // top of the actual (cheap) matching work, making a global match
   // quadratic overall once the match count grows with B ('a' ⎕RE['g']
   // 32000⍴'a' took ~370ms; a single, non-global match of the same
   // subject took ~1ms). Every call after the first (start > 0) is
   // continuing a search over this SAME B that a previous call already
   // validated successfully to get this far, so PCRE2_NO_UTF_CHECK is
   // safe there; the first call (start == 0) still validates B in
   // full, so a B holding a genuinely invalid codepoint (Bugs28 #79/#80:
   // GNU APL's Unicode is not restricted to legal Unicode scalars) is
   // still caught as a real PCRE2 error.
   //
const uint32_t options = (start ? PCRE2_NO_UTF_CHECK : 0) | extra_options;

   // an empty B may have no buffer at all, but PCRE2 rejects a NULL
   // subject even with length 0 ("NULL argument passed", Bugs35 #57)
   //
static const PCRE2_UCHAR32 empty_subject[1] = { 0 };
const PCRE2_UCHAR32 * subject = B.size() ? B.raw<PCRE2_UCHAR32>()
                                         : empty_subject;
   match_result = pcre2_match_32(code, subject, B.size(),
                                 start, options, match_data, NULL);
   if (match_result == 0)
      {
        MORE_ERROR() << "Match buffer too small";
        ovector_count = 0;
        ovector = 0;
        pcre2_match_data_free(match_data);   // ~RegexpMatch() will not run
        DOMAIN_ERROR;
      }
    else if (match_result > 0)
      {
        ovector = pcre2_get_ovector_pointer_32(match_data);
        ovector_count = pcre2_get_ovector_count_32(match_data);
      }
    else if (match_result == PCRE2_ERROR_NOMATCH)
      {
        // PCRE2_ERROR_NOMATCH is an entirely ordinary "the pattern did
        // not match" outcome, not an error -- ⎕RE (without the E flag)
        // returns ⍬ for it without raising anything. Setting )MORE text
        // here left every subsequent (successful!) statement's )MORE
        // claiming a bogus "libpcre error: no match", clobbering any
        // real )MORE text from an actual, earlier error (Blake McBride,
        // Bugs20 #7).
        //
        ovector_count = 0;
        ovector = 0;
      }
    else   // match_result < 0, and not PCRE2_ERROR_NOMATCH: a genuine
           // PCRE runtime error (e.g. match limit exceeded), not "no
           // match" -- returning ⍬ for this the same way as an ordinary
           // non-match left the caller with no way to tell the two apart
           // (Blake McBride, Bugs28 #72). DOMAIN ERROR instead, matching
           // the allocation-failure and buffer-too-small cases above.
      {
        MORE_ERROR() << "libpcre error: "
                     << Regexp::pcre_error(match_result);
        ovector_count = 0;
        ovector = 0;
        pcre2_match_data_free(match_data);   // ~RegexpMatch() will not run
        DOMAIN_ERROR;
      }
}
//────────────────────────────────────────────────────────────────────────────
RegexpMatch::~RegexpMatch()
{
   pcre2_match_data_free(match_data);
}
//────────────────────────────────────────────────────────────────────────────
bool
RegexpMatch::is_match() const
{
   return match_result > 0;
}
//────────────────────────────────────────────────────────────────────────────
UCS_string
RegexpMatch::matched_string() const
{
const PCRE2_SIZE * ovector = get_ovector();
UCS_string result(matched_B, ovector[0], ovector[1] - ovector[0]);
   return result;
}
//────────────────────────────────────────────────────────────────────────────
int
RegexpMatch::num_matches() const
{
   if (match_result < 0)
      {
        MORE_ERROR() << "Attempt to call num_matches without matches";
        FIXME;
      }

    return match_result;
}
//════════════════════════════════════════════════════════════════════════════
Regexp::Regexp(const UCS_string & pattern, int flags)
{
const PCRE2_UCHAR32 * pattern_ucs = ucs_to_codepoints(pattern);

int error_code = 0;
PCRE2_SIZE error_offset = -1;

   code = pcre2_compile_32(pattern_ucs, pattern.size(),
                           PCRE2_UTF | PCRE2_UCP | flags, &error_code,
                           &error_offset, 0);
   delete[] pattern_ucs;

   if (code)   // OK
      {
        compute_group_parents(pattern);
        return;
      }

   MORE_ERROR() << 
"in (say) Z←A ⎕RE B: error when compiling the regular expression A.\n"
"    The cause of the error was: " << pcre_error(error_code) << ".\n"
"    The regular expression A was: '" << pattern << "'\n"
"    The error was detected at " << int(error_offset) << "↓A.\n"
"    the offending rest (" << (int)error_offset << "↓A) of A was: '"
   << UCS_string(pattern, error_offset) << "'";

   DOMAIN_ERROR;
}
//────────────────────────────────────────────────────────────────────────────
void
Regexp::compute_group_parents(const UCS_string & pattern)
{
   // the structure of the ⎕RE result is the structure of the capturing
   // groups in the pattern (apl.texi "Complex Matches"), not of the spans
   // of one match (Bugs38 #55). group_parents[g] is the innermost capturing
   // group that contains group g (0 = the whole match).
   //
std::vector<int> parents(1, -1);   // group 0: the whole match
std::vector<int> open;             // the groups of the open ( (or -1)
const ShapeItem len = pattern.size();
   for (ShapeItem p = 0; p < len; ++p)
       {
         const Unicode uni = pattern[p];
         if (uni == UNI_BACKSLASH)
            {
              if (p + 1 < len && pattern[p + 1] == UNI_Q)   // \Q ... \E
                 {
                   for (p += 2; p + 1 < len; ++p)
                       if (pattern[p] == UNI_BACKSLASH &&
                           pattern[p + 1] == UNI_E)   { ++p;   break; }
                 }
              else ++p;   // skip the escaped character
              continue;
            }

         if (uni == UNI_L_BRACK)   // character class [...]
            {
              ++p;
              if (p < len && pattern[p] == UNI_CIRCUMFLEX)   ++p;
              if (p < len && pattern[p] == UNI_R_BRACK)      ++p;   // []...]
              for (; p < len && pattern[p] != UNI_R_BRACK; ++p)
                  if (pattern[p] == UNI_BACKSLASH)   ++p;
              continue;
            }

         if (uni == UNI_L_PARENT)
            {
              const Unicode n1 = p + 1 < len ? pattern[p + 1] : Unicode_0;
              const Unicode n2 = p + 2 < len ? pattern[p + 2] : Unicode_0;
              const Unicode n3 = p + 3 < len ? pattern[p + 3] : Unicode_0;
              if (n1 == UNI_QUESTION && n2 == UNI_NUMBER_SIGN)   // (?# ...)
                 {
                   while (p < len && pattern[p] != UNI_R_PARENT)   ++p;
                   continue;
                 }

              bool capturing = n1 != UNI_QUESTION && n1 != UNI_ASTERISK;
              if (n1 == UNI_QUESTION)   // named groups capture
                 {
                   if (n2 == UNI_SINGLE_QUOTE)   capturing = true;
                   if (n2 == UNI_P && n3 == UNI_LESS)   capturing = true;
                   if (n2 == UNI_LESS && n3 != UNI_EQUAL &&
                       n3 != UNI_EXCLAM)   capturing = true;
                 }

              if (capturing)
                 {
                   int parent = 0;
                   for (ShapeItem o = open.size() - 1; o >= 0; --o)
                       if (open[o] > 0)   { parent = open[o];   break; }
                   open.push_back(parents.size());
                   parents.push_back(parent);
                 }
              else
                 {
                   open.push_back(-1);
                 }
              continue;
            }

         if (uni == UNI_R_PARENT && open.size())   open.pop_back();
       }

   // use the result only if it agrees with PCRE2 (e.g. not for (?|...),
   // where several groups share a number)
   //
   if (int(parents.size()) == expression_count() + 1)   group_parents = parents;
}
//────────────────────────────────────────────────────────────────────────────
Regexp::~Regexp()
{
   pcre2_code_free(code);
}
//────────────────────────────────────────────────────────────────────────────
int
Regexp::expression_count() const
{
uint32_t result;
    pcre2_pattern_info(code, PCRE2_INFO_CAPTURECOUNT, &result);

    return result;
}
//────────────────────────────────────────────────────────────────────────────
const RegexpMatch *
Regexp::match(const UCS_string &match, PCRE2_SIZE size) const
{
   return new RegexpMatch(code, match, size);
}
//────────────────────────────────────────────────────────────────────────────
UCS_string
Regexp::pcre_error(int ec)
{
union _u_pcre
{
  PCRE2_UCHAR32 pbuff[256];   // proposed max: 120 code units
  Unicode       ubuff[256];
} buffer;
const int len = pcre2_get_error_message_32(ec, buffer.pbuff, 256);
   if (len > 0)   return UCS_string(buffer.ubuff, len);

UCS_string ret(U"Unknown libpcre error ");
   return ret << ec;
}
//════════════════════════════════════════════════════════════════════════════
