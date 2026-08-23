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

#include <string.h>
#include <stdio.h>

#include "ArgCheck.hh"
#include "Avec.hh"
#include "Bif_F12_FORMAT.hh"
#include "CharCell.hh"
#include "PrimitiveFunction.hh"
#include "PrintOperator.hh"
#include "Value.hh"
#include "Workspace.hh"

Bif_F12_FORMAT   Bif_F12_FORMAT::fun;       // ⍕

//════════════════════════════════════════════════════════════════════════════
UCS_string
Format_sub::insert_fract_commas(const UCS_string & data) const
{
UCS_string ucs;
int d = 0;

   loop(f, format.ssize())
      {
        const Unicode format_char = format[f];
         if (format_char == UNI_COMMA)
            {
              ucs << Workspace::get_FC(1);
            }
         else if (Avec::is_digit(format_char))
            {
              if (d < data.ssize())   ucs << data[d++];
              else                    break;
            }
         else if (format_char == Workspace::get_FC(4))
            {
              // ⎕FC[5]: print-as-blank -- a fixed separator position that
              // never consumes a data digit (apl2lrm.txt p.141).
              ucs << UNI_SPACE;
            }
         else
            {
              CERR << "Offending format char [" << f << "] : '"
                   << format_char << "' at " << LOC << endl;
            }
      }

   if ((flt_mask & BIT_9) && ucs.all_zeroes())   ucs.clear();

   return ucs;
}
//────────────────────────────────────────────────────────────────────────────
UCS_string
Format_sub::insert_int_commas(const UCS_string & data, bool & overflow) const
{
size_t fill_pos = -1;
Unicode fill_char = UNI_SPACE;

   // BIT_0: pad with 0
   // BIT_8: fille with ⎕FC[3] (* by default)
   // BIT_9: pad with 0
   if (flt_mask & (BIT_0 | BIT_8 | BIT_9))   // format has a '0', '8', or '9'
      {
        loop(f, format.ssize())
           {
             const Unicode format_char = format[f];
              if (format_char == UNI_0 || format_char == UNI_9)
                 {
                   fill_pos = f;
                   fill_char = UNI_0;
                   break;
                 }

              if (format_char == UNI_8)
                 {
                   fill_pos = f;
                   fill_char = Workspace::get_FC(2);
                   break;
                 }
           }
      }

   // we move backwards from the end of the format field, filling it
   // with chars from data of from format.
   //
UCS_string ucs;
size_t d = data.size();

   // d  runs downwards from data.size()   to 0
   // f1 runs upwards  from 0              to  format.ssize()
   // f  runs downwards from format.ssize() to 0
   //
   loop(f1, format.ssize())
      {
        const size_t f = format.ssize() - f1 - 1;
        const Unicode format_char = format[f];
         if (format_char == UNI_COMMA)
            {
              // Workspace::get_FC(1) is ⎕FC[2] when ⎕IO is 1
              //
              if (d)                    ucs << Workspace::get_FC(1);
              else if (f >= fill_pos)   ucs << fill_char;
              else                      break;
            }
         else if (Avec::is_digit(format_char))
            {
              if (d)                    ucs << data[--d];
              else if (f >= fill_pos)   ucs << fill_char;
              else                      break;
            }
         else if (format_char == Workspace::get_FC(4))
            {
              // ⎕FC[5]: print-as-blank -- always a literal space,
              // never consumes a data digit, regardless of fill/
              // overflow state (apl2lrm.txt p.141).
              ucs << UNI_SPACE;
            }
         else
            {
              CERR << "Offending format char [" << f << "] : '"
                   << format_char << "' at " << LOC << endl;
              break;
            }
      }

   if (d)   // format too short
      {
         if (Workspace::get_FC(3) == UNI_0)   DOMAIN_ERROR;
         else                             overflow = true;
      }

   if ((flt_mask & BIT_9) && ucs.all_zeroes())   ucs.clear();

   ucs.reverse();
   return ucs;
}
//────────────────────────────────────────────────────────────────────────────
int
Format_sub::map_field(int type)
{
int flt_cnt = 0;

   loop(f, format.ssize())
      {
        switch(format[f])
           {
             case UNI_0:              flt_mask |= BIT_0;   break;
             case UNI_1: ++flt_cnt;   flt_mask |= BIT_1;   break;
             case UNI_2: ++flt_cnt;   flt_mask |= BIT_2;   break;
             case UNI_3: ++flt_cnt;   flt_mask |= BIT_3;   break;
             case UNI_4: ++flt_cnt;   flt_mask |= BIT_4;   break;
             case UNI_5:              flt_mask |= BIT_5;   break;
             case UNI_6:              flt_mask |= BIT_6;   break;
             case UNI_7:              flt_mask |= BIT_7;   break;
             case UNI_8:              flt_mask |= BIT_8;   break;
             case UNI_9:              flt_mask |= BIT_9;   break;
             default:                                            break;
           }
      }

   if (flt_mask & (BIT_0 | BIT_9))
      {
        min_len = format.ssize();
        loop(f, format.ssize())
            {
              const size_t pos = (type == 1) ? format.ssize() - f - 1 : f;
              const Unicode uni = format[pos];
              if (uni == UNI_0)   break;
              if (uni == UNI_9)   break;
              --min_len;
            }
      }

   return flt_cnt;
}
//────────────────────────────────────────────────────────────────────────────
ostream &
Format_sub::print(ostream & out) const
{
   out << "format: '" << format << "',"
          " min: " << min_len << ", out_len " << out_len << ", flags: ";
   loop(d, 32)   if (flt_mask & (1 << d))   out << char('0' + d);
   return out;
}
//════════════════════════════════════════════════════════════════════════════
Bif_F12_FORMAT::Format_LIFER::Format_LIFER(const UCS_string format)
   : exponent_char(UNI_E),
     expo_negative(false)
{
   // split one column format into sub-format chunks...
   //
int f = 0;
bool exponent_pending = false;
bool have_decimal_point = false;
bool exponent_requested = false;   // set at exponent_part: (see below)

// left_decorator:
   while (f < format.ssize())
      {
        if (is_picture_char(format[f]))   goto integral_part;
        else                              left_deco.format << format[f++];
      }
   goto fields_done;

integral_part:
   while (f < format.ssize())
      {
        const Unicode cc = format[f++];

        if (cc == UNI_FULLSTOP)
           {
             have_decimal_point = true;
             goto fractional_part;
           }
        if (cc == UNI_E)          goto exponent_part;
        if (is_picture_char(cc))
           {
             int_part.format << cc;
             if (cc == UNI_6)          goto right_decorator;
           }
        else
           {
             --f;
            goto right_decorator;
           }
      }
   goto fields_done;

fractional_part:
   while (f < format.ssize())
      {
        const Unicode cc = format[f++];
        if (cc == UNI_7)          exponent_pending = true;

        if (is_picture_char(cc))
           {
             fract_part.format << cc;
             if (cc == UNI_6)     goto right_decorator;
           }
        else
           {
             exponent_char = cc;
             goto exponent_decorator;
           }
      }
   goto right_decorator;

exponent_decorator:
   if (!exponent_pending)   { --f;   goto right_decorator; }

   while (f < format.ssize())
      {
        const Unicode cc = format[f++];

        if (!is_picture_char(cc))
           {
             expo_deco.format << cc;
           }
        else
           {
             --f;
             goto exponent_part;
           }
      }

exponent_part:
   // every route into this label (the direct 'E' trigger in integral_part,
   // and exponent_decorator's control-char lookahead) intends a genuine
   // exponent field with at least one digit position; remember that so
   // fields_done can catch the case where no digit position ever actually
   // materializes (e.g. a trailing/orphaned 'E' as in Blake McBride,
   // Bugs12.md #6: '9E', '99E', '9EE', '9E ', '9E'⍕1.5, '9E'⍕1E¯308) --
   // without this, exponent.size() stays 0 so the out_len computation
   // below skips expo_deco/exponent entirely, and format_by_example()'s
   // result width (from all_formats, the example string) silently
   // disagrees with format_example()'s output width (from the *_out_len
   // fields), which used to trip the row.size()==all_formats.size()
   // assertion (or, with asserts off, leave cells uninitialized).
   //
   exponent_requested = true;
   while (f < format.ssize())
      {
        const Unicode cc = format[f++];

        if (is_picture_char(cc))
           {
             exponent.format << cc;
             if (cc == UNI_6)          goto right_decorator;
           }
        else
           {
             --f;
             goto right_decorator;
           }
      }

right_decorator:   /// the right decorator

   if (have_decimal_point &&
       exponent.format.ssize() == 0 &&
       fract_part.format.ssize() == 0)   // quirk!
      {
        // this is ambiguous as to whether the decimal point belongs to
        // the number or to the right decorator. lrm says all non-digits
        // are decorators but that seems to be wrong anyhow. (e.g. ',').
        //
        // we put a trailing decimat point into the right decorator.
        //
        right_deco.format << UNI_FULLSTOP;
      }

   while (f < format.ssize())
      right_deco.format << format[f++];

fields_done:
   if (exponent_requested && exponent.format.ssize() == 0)
      {
        // an 'E' was seen (directly, or via a pending exponent flag) but
        // no digit position followed it before the format string ended or
        // a right decorator began -- there is no way to format a numeric
        // exponent with zero digit positions. See the exponent_part:
        // comment above (Blake McBride, Bugs12.md #6).
        //
        MORE_ERROR() << "A⍕B : Bad format A"
                        " (exponent 'E' without a digit position)";
        DOMAIN_ERROR;
      }

   left_deco.out_len = left_deco .format.ssize();

   int_part  .out_len = int_part  .format.ssize();

   if (fract_part.format.ssize())
      fract_part.out_len = fract_part.format.ssize() + 1;

   if (exponent.size())
      {
        expo_deco.out_len = expo_deco.size();
        exponent.out_len = exponent.format.ssize() + 1;
      }

   right_deco.out_len = right_deco.format.ssize();

   // compute floating flags.
   //
   {
     const int count = int_part  .map_field(0)
                     + fract_part.map_field(1);

     if (count == 1)   // flags apply to both sides.
        {
          int_part.  flt_mask |= fract_part.flt_mask & (BIT_1 | BIT_2 | BIT_3);
          fract_part.flt_mask |= int_part  .flt_mask & (BIT_1 | BIT_2 | BIT_3);
        }

     exponent.map_field(2);
   }
}
//────────────────────────────────────────────────────────────────────────────
UCS_string
Bif_F12_FORMAT::Format_LIFER::format_example(APL_Float value)
{
UCS_string data_int;
UCS_string data_fract;
UCS_string data_expo;
const APL_Float val_1 = (value < 0.0) ? APL_Float(-value) : value;

bool overflow = false;
   fill_data_fields(val_1, data_int, data_fract, data_expo, overflow);
   Log(LOG_Bif_F12_FORMAT)
      {
        Q1(overflow)
        Q1(data_int)
        Q1(data_fract)
        Q1(data_expo)
      }
   if (overflow)   return UCS_string(out_size(), Workspace::get_FC(3));

const UCS_string left = format_left_side(data_int, value < 0.0, overflow);
   if (overflow)   return UCS_string(out_size(), Workspace::get_FC(3));
   Assert(left.ssize() == (left_deco.out_len + int_part.out_len));

const UCS_string right = format_right_side(data_fract, value < 0.0, data_expo);
   Assert(right.ssize() == (fract_part.out_len + expo_deco.out_len +
                           exponent.out_len + right_deco.out_len));

   return left + right;
}
//────────────────────────────────────────────────────────────────────────────
void
Bif_F12_FORMAT::Format_LIFER::fill_data_fields(APL_Float value,
                UCS_string & data_int, UCS_string & data_fract,
                UCS_string & data_expo, bool & overflow)
{
char format[40];
const int data_buf_len = int_part.out_len + 1        // 123.
                       + fract_part.out_len          // 456
                       + 1 + exponent.out_len + 1;   // E-22

   if (data_buf_len > 100)   DOMAIN_ERROR;

char data_buf[101];
char * fract_end = 0;

   if (exponent.size())   // E in format string
      {
        // create a format like %.5E (if fract_part has 5 digits)
        //
        SPRINTF(format, "%%.%luE", ulong(fract_part.size()));
        SPRINTF(data_buf, format, value);

        char * ep = strchr(&data_buf[0], 'E');
        Assert(ep);
        fract_end = ep++;
        if      (*ep == '+')   ++ep;                               // skip +
        else if (*ep == '-')   { expo_negative = true;   ++ep; }   // skip -
        if (*ep == '0' && ep[1])   ++ep;          // skip leading 0 in exponent

        int elen = strlen(ep);

        // the exponent could be wider than the field the example string
        // reserved for it (e.g. '0.07E0' ⍕ 1E100 needs 3 exponent digits
        // but the example only reserves 1); unlike the integer-part
        // overflow check just below fill_data_fields()'s caller, this
        // case was not detected here, so it instead surfaced downstream
        // as a negative pad_count in format_right_side() -- a size_t
        // underflow that aborts the interpreter. Detect it here instead,
        // mirroring the integer-part check.
        if (elen > exponent.format.ssize())
           {
             if (Workspace::get_FC(3) == UNI_0)
                {
                  MORE_ERROR() << "A⍕B : Bad value " << value
                               << " (exponent exceeds A specification)";
                  DOMAIN_ERROR;
                }
             overflow = true;
             return;
           }

        // insert leading zeros until we have at least min_len digits.
        //
        for (; elen < exponent.min_len; ++elen)   data_expo << UNI_0;

        const UTF8_string ep_utf(ep);
        data_expo << UCS_string(ep_utf);
      }
   else   // no exponent in format string.
      {
        SPRINTF(format, "%%.%luf", ulong(fract_part.size()));

        const int dlen = snprintf(data_buf, data_buf_len, format, value);
        NULL_TERMINATE(data_buf)

        // the int part could be longer than allowed by the example string.
        //
        const char * dot = strchr(data_buf, '.');
        const int ilen = dot ? (dot - data_buf) : strlen(data_buf);
        if (ilen > int_part.out_len)
           {
             if (Workspace::get_FC(3) == UNI_0)
                {
                  MORE_ERROR() << "A⍕B : Bad value " << value
                               << " (integer part exceeds A specification)";
                  DOMAIN_ERROR;
                }
             overflow = true;
             return ;
           }

        Assert(dlen < data_buf_len);
        fract_end = &data_buf[dlen];
      }

char * int_end = strchr(data_buf, '.');
   if (fract_part.size() == 0)
      {
        Assert(int_end == 0);
        int_end = fract_end;
      }
   else
      {
        Assert(int_end);
        char * fract_digits = int_end + 1;

        int flen = fract_end - fract_digits;

        // remove trailing zeros, but leave at least min_len chars.
        //
        while (fract_digits[flen - 1] == '0' && flen > fract_part.min_len)
               fract_digits[--flen] = 0;

        loop(f, flen)   data_fract << Unicode(fract_digits[f]);
      }

const int ilen = int_end - &data_buf[0];

   // insert leading zeros so that we will have at least min_len digits
   // after appending the integer data.
   //
   loop(d, int_part.min_len - ilen)   data_int << UNI_0;

   loop(i, ilen)   data_int << Unicode(data_buf[i]);

   // convert 0.xxx to .xxx
   //
   if (  data_int.size() == 1
      && data_int[0] == UNI_0
      && int_part.min_len == 0
      && fract_part.size())   data_int.clear();

   Log(LOG_Bif_F12_FORMAT)
      {
        CERR << "At " LOC " in Format_LIFER::fill_data_fields()" << endl
             << "    format:     '" << format << "'"     << endl
             << "    value:      "  << value             << endl
             << "    data_int:   '" << data_int << "'"   << endl
             << "    data_fract: '" << data_fract << "'" << endl
             << "    data_expo:  '" << data_expo << "'"  << endl;
      }
}
//────────────────────────────────────────────────────────────────────────────
UCS_string
Bif_F12_FORMAT::Format_LIFER::format_left_side(const UCS_string data_int,
                                               bool negative, bool & overflow)
{
const UCS_string data = int_part.insert_int_commas(data_int, overflow);

   if (overflow)   return UCS_string();

   Assert(int_part.size() >= data.size());
const Unicode pad_char = int_part.pad_char(Workspace::get_FC(2));
const UCS_string pad(int_part.size() - data.size(), pad_char);

UCS_string ucs;
   if (int_part.no_float())                 // floating disabled.
      {
        ucs << left_deco.format << pad;
      }
   else if (int_part.do_float(negative))   // floating enabled, deco shown
      {
        ucs << pad << left_deco.format;
      }
   else                                     // floating enabled, deco hidden
      {
        ucs << pad << UCS_string(left_deco.size(), UNI_SPACE);
      }

   return ucs << data;
}
//════════════════════════════════════════════════════════════════════════════
UCS_string
Bif_F12_FORMAT::Format_LIFER::format_right_side(const UCS_string data_fract,
                                                bool negative,
                                                const UCS_string data_expo)
{
int pad_count = 0;
UCS_string ucs;

   if (fract_part.out_len)
      {
        const UCS_string data = fract_part.insert_fract_commas(data_fract);
        Assert(fract_part.size() >= data.size());
        pad_count += fract_part.size() - data.size();

        // print nothing instead of .
        if (data.size() == 0)   ++pad_count;
        else                    ucs << Workspace::get_FC(0);
        ucs << data;
      }

   if (exponent.size())
      {
        UCS_string data(1, exponent_char);   // the 'E'

        const Unicode pad_char = exponent.pad_char(Workspace::get_FC(2));
        if (exponent.no_float())              // floating disabled.
           {
             data << expo_deco.format << data_expo;
           }
        else if (exponent.do_float(expo_negative))   // floating, deco shown
           {
             data << expo_deco.format << data_expo;
           }
        else                                 // floating enabled and deco hidden
           {
             data << UCS_string(expo_deco.format.ssize(), pad_char)
                  << data_expo;
           }

        pad_count += expo_deco.out_len + exponent.out_len - data.size();
        ucs << data;
      }

   // now do the right side padding.
   {
     const Unicode pad_char = fract_part.pad_char(Workspace::get_FC(2));
     if (pad_count < 0)   pad_count = 0;   // defensive: see fill_data_fields()
     const UCS_string pad(pad_count, pad_char);

     if (fract_part.no_float())                // floating disabled.
        {
          ucs << right_deco.format << pad;
        }
     else if (fract_part.do_float(negative))  // floating, deco shown
        {
          ucs << pad << right_deco.format;
        }
     else                                      // floating, deco hidden
        {
          ucs << pad << UCS_string(right_deco.size(), pad_char);
        }
   }

   return ucs;
}
//────────────────────────────────────────────────────────────────────────────
Value_P
Bif_F12_FORMAT::format_by_specification(cValue_R A, cValue_R B)
{
   // A is a near-int scalar or vector.

const Shape shape_1(1);
const Shape & shape_B = B.get_rank() ? B.get_shape() : shape_1;

const ShapeItem rows_B = shape_B.get_rows();
const ShapeItem cols_B = shape_B.get_cols();

const ShapeItem len_A = A.element_count();

   if (len_A != 1 && len_A != 2 && len_A != 2*cols_B)
      {
        MORE_ERROR() << "A⍕B: expecting ⍴A to be 1, 2, or "
                     << (2*cols_B) << " (2×the number of columns of B); ⍴A"
                        " is " << len_A;
        LENGTH_ERROR;
      }

   if (shape_B.get_volume() == 0)   // empty B
      {
        ShapeItem W = 0;
        loop(c, cols_B)
           {
             // width (unlike the non-empty-B path's
             // format_one_col_by_spec(), which enforces width>=0) was
             // used completely unvalidated here -- a negative width
             // silently produced a Value with a negative shape item.
             // ¯5 2 ⍕ 0 3⍴0 gave ⍴Z == 0 ¯15 instead of DOMAIN_ERROR.
             const APL_Integer w = (len_A <= 2) ? A.get_near_int(0)
                                                : A.get_near_int(2*c);
             if (w < 0)
                {
                  MORE_ERROR() << "A⍕B: width " << w
                               << " (for column " << (c + Workspace::get_IO())
                               << ") is negative";
                  DOMAIN_ERROR;
                }
             W += w;
           }

        Shape shape_Z = shape_B.without_last_axis();
        shape_Z.add_shape_item(W);
        const ShapeItem ec_Z = shape_Z.get_volume();

        Value_P Z(shape_Z, LOC);
        loop(z, ec_Z)   Z->next_ravel_Char(UNI_SPACE);

        Z->set_proto_Spc();
        return Z;
      }

PrintBuffer pb;
   loop(col, cols_B)
       {
         APL_Integer col_width;
         APL_Integer precision;

         if (len_A == 1)
            {
              col_width = 0;
              precision = A.get_near_int(0);
            }
         else if (len_A == 2)
            {
              col_width = A.get_near_int(0);
              precision = A.get_near_int(1);
            }
         else
            {
              col_width = A.get_near_int(2*col);
              precision = A.get_near_int(2*col + 1);
            }

         // pb_col is the PrintBuffer for one numeric column.
         //
         PrintBuffer pb_col(format_one_col_by_spec(col_width, precision,
                                                   B, col,
                                                   cols_B, rows_B));

         // automatic col width normally reserves a leading separator
         // space, but that's meaningless for the very first column --
         // there is no preceding column to separate from (Blake
         // McBride, LanguageVariances.md #15: 0⍕A added a spurious
         // leading blank to column 0 specifically because col_width==0
         // for every column under an all-automatic-width spec like
         // `0⍕A`, and the content-based re-check just below only runs
         // for col>0).
         //
         bool insert_space_left = col && (col_width == 0);

         if (col && pb_col.get_column_count() > 0)   // subsequent column:
            {                                         // insert space if needed
              loop(y, pb_col.get_row_count())
                  {
                    if (pb_col.get_char(0, y) != UNI_SPACE)
                       {
                         insert_space_left = true;
                         break;
                       }
                  }
            }

         if (insert_space_left)   pb_col.pad_l(UNI_SPACE, 1);

         if (col)   pb.append_col(pb_col);
         else       pb = pb_col;
       }

const ShapeItem pb_w = pb.get_column_count();
const ShapeItem pb_h = pb.get_row_count();
Shape shape_Z(shape_B);
   shape_Z.set_last_shape_item(pb_w);

Value_P Z(shape_Z, LOC);

   loop(h, pb_h)
   loop(w, pb_w)   Z->next_ravel_Char(pb.get_char(w, h));

   return Z;
}
//────────────────────────────────────────────────────────────────────────────
bool
Bif_F12_FORMAT::is_control_char(Unicode uni)
{
   return Avec::is_digit(uni)      ||
          (uni == UNI_COMMA) ||
          (uni == UNI_FULLSTOP);
}
//────────────────────────────────────────────────────────────────────────────
bool
Bif_F12_FORMAT::is_picture_char(Unicode uni)
{
   // like is_control_char(), but for parsing a format-by-example picture
   // string specifically: also recognizes ⎕FC[5] (print-as-blank,
   // apl2lrm.txt p.141) as a digit-group control character. Deliberately
   // NOT folded into is_control_char() itself -- that function is also
   // used by Quad_FC::assign()/assign_indexed() (SystemVariable.cc) to
   // validate a *proposed* ⎕FC[5] value, where checking "does this equal
   // the *current* ⎕FC[5]" would be self-referential and wrong (e.g.
   // ⎕FC←'' silently turned its own '_' default into a space, since the
   // current ⎕FC[5] was already '_').
   //
   return is_control_char(uni) || (uni == Workspace::get_FC(4));
}
//────────────────────────────────────────────────────────────────────────────
Value_P
Bif_F12_FORMAT::monadic_format(cValue_R B)
{
   Assert(B.get_rank() <= 2);

const PrintStyle style(PrintStyle(PR_APL | PST_NO_FRACT_0));
const PrintContext pctx = Workspace::get_PrintContext(style);

const PrintBuffer pb(B, pctx, 0);

const ShapeItem width  = pb.get_column_count();
const ShapeItem height = pb.get_row_count();

   // monadic_format() returns the Value with rank 1 or 2 which may be changed
   // in Bif_F12_FORMAT::eval_B() later on to match the rather arbitrary rules
   // in lrm.
   //
Value_P Z;
   if (height == 1)   Z = Value_P(width, LOC);
   else               Z = Value_P(Shape(height, width), LOC);

   loop(h, height)
   loop(w, width)
      {
        const Unicode uni = pb.get_char(w, h);
        if (is_iPAD_char(uni))  Z->next_ravel_Char(UNI_SPACE);
        else                    Z->next_ravel_Char(uni);
      }

   Z->check_value(LOC);
   return Z;
}
//════════════════════════════════════════════════════════════════════════════
Token
Bif_F12_FORMAT::eval_B(cValue_R B) const
{
   // ISO and lrm: If B is a character array, then Z is B
   //
   if (!B.NOTCHAR())   return Token(TOK_APL_VALUE1, CLONE(&B, LOC));

   if (B.is_empty())
      {
        Value_P Z(B.get_shape(), LOC);
        Z->set_proto_Spc();
        Z->check_value(LOC);
        return Token(TOK_APL_VALUE1, Z);
      }

   if (!B.is_simple())
      {
        PrintBuffer pb(B, Workspace::get_PrintContext(PR_APL), 0);
        Assert(pb.is_rectangular());
        const ShapeItem cols = pb.get_column_count();
        const ShapeItem rows = pb.get_row_count();
        Shape shape_Z(rows, cols);
        Value_P Z(shape_Z, LOC);
        loop(y, rows)
            {
              UCS_string row = pb.get_line(y);
              row.map_pad();
              loop(x, cols)   Z->next_ravel_Char(row[x]);
            }

        Z->check_value(LOC);

        // turn 1-line matrices into vectors
        //
        if (Z->get_rank() == 2 && Z->get_shape().get_shape_item(0) == 1)
           {
             Shape sh(Z->get_shape().get_shape_item(1));
             Z->set_shape(sh);
           }

        return Token(TOK_APL_VALUE1, Z);
      }

Value_P Z;
   if (B.get_rank() > 2)
      {
        // temporarily reduce the N > 2 dimensions of B
        // to N = 2 dimensions of B1 (with the same ravel).
        //
        const Shape shape_B = B.get_shape();
        const Shape shape_B1(B.get_rows(), B.get_cols());

        try
           {
             static_cast<Value *>(const_cast<cValue *>(&B))->set_shape(shape_B1);
             Z = monadic_format(B);
             static_cast<Value *>(const_cast<cValue *>(&B))->set_shape(shape_B);
           }
        catch (Error &)
           {
             static_cast<Value *>(const_cast<cValue *>(&B))->set_shape(shape_B);
             throw;   // rethrow error
           }
        catch (std::bad_alloc &)
           {
             static_cast<Value *>(const_cast<cValue *>(&B))->set_shape(shape_B);
             throw;   // rethrow error
           }
        catch (...)
           { FIXME; }

        // ¯1↓⍴B ←→ ¯1↓⍴Z i.e. the leading axes of B have the same lengths as
        // the leading axes of Z. monadic ⍕ changes (increases) only the length
        // of the last axis. We reshape Z to ¯1↓⍴B , ¯1⍴⍴Z.
        //
        Shape shape_Z = B.get_shape().without_last_axis();   // ¯1↓⍴B
        shape_Z.add_shape_item(Z->get_last_shape_item());     // ¯1↓⍴B , ¯1↑⍴Z
        Z->set_shape(shape_Z);
      }
   else   // B.get_rank() is 0, 1, or 2
      {
        Z = monadic_format(B);
      }

   // adjust rank as needed:
   // ρρZ ←→ ,1⌈⍴ρB     if B is simple
   // ρρZ ←→ ,1 or ,2   if B is nested
   //
const APL_types::Depth depth = B.compute_depth();
Shape sZ;
   if (depth > 1)   // B is nested, therefore ⍴⍴R is 1 or 2
      {
        // the  examples in lrm contradict the text in lrm.
        //
        // Page 136 shows: R←2 3ρ'ONE' 1 1 'TWO' 2 22 which contains only
        // scalar and vector items (R itself being a matrix) and therefore
        // ⍕R should be a vector according to page 137:
        //
        // "Nested Arrays: When R is a nested array, Z is a vector if all"
        // items of R at any depth are scalars or vectors."
        //
        // lrm also contradicts the ISO standard regarding the rank of Z.
        //
        // We try our best...
        //
        if (B.is_one_dimensional())
           {
             sZ = Shape(Z->element_count());
           }
        else
           {
             sZ = Z->get_shape();   // leave Z as is (2-dimensional)
           }
      }
   else             // B is simple:                    result rank: ,1⌈⍴ρB
      {
        if (B.get_rank() < 2)
           {
             sZ = Shape(Z->get_cols());
           }
        else
           {
             sZ = B.get_shape();
             sZ.set_last_shape_item(Z->get_last_shape_item());
           }
      }

   if (sZ != Z->get_shape())
      {
        Assert(sZ.get_volume() == Z->element_count());
        Z->set_shape(sZ);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//════════════════════════════════════════════════════════════════════════════
Token
Bif_F12_FORMAT::eval_AB(cValue_R A, cValue_R B) const
{
Value_P Z;

   // any A should be a scalar or a vcector
   //
   ArgCheck::require_scalar_or_vector("A⍕B", "A", A);

   if      (A.is_char_array())   Z = format_by_example(A, B);
   else if (A.is_int_array())    Z = format_by_specification(A, B);
   else
      {
        MORE_ERROR() << "A⍕B: A must be characters (format by example) or"
                        " integers (format by specification)";
        DOMAIN_ERROR;
      }

   Z->set_proto_Spc();
   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//────────────────────────────────────────────────────────────────────────────
Value_P
Bif_F12_FORMAT::format_by_example(cValue_R A, cValue_R B)
{
   // convert the ravel of char vector A into UCS_string 'format'.
   //
UCS_string all_formats = A.get_UCS_ravel();
   if (all_formats.size() == 0)
      {
        MORE_ERROR() << "A⍕B: A (the format string) is empty";
        LENGTH_ERROR;
      }

const ShapeItem cols = B.get_cols();
const ShapeItem rows = B.get_rows();

   // split string all_formats into individual format fields, one per column.
   // If there is only one format field, then repeat it cols times.
   //
UCS_string_vector col_formats;
   split_example_into_columns(all_formats, col_formats);
   if (col_formats. size() == 1)
      {
        const UCS_string f = all_formats;
        const UCS_string col0 = col_formats[0];   // keep it out of loop!
        loop(c, cols - 1)
           {
             all_formats << f;
             col_formats.push_back(col0);
           }
      }

   if (cols != ShapeItem(col_formats.size()))
      {
        MORE_ERROR() << "A⍕B: A (the format string) has "
                     << col_formats.size() << " column format(s); expecting"
                        " 1 or " << cols << " (the number of columns of B)";
        LENGTH_ERROR;
      }

   // convert each column format string into a Format_LIFER
   //
vector<Format_LIFER> col_items;
   loop(c, col_formats.size())
       col_items.push_back(Format_LIFER(col_formats[c]));

   Log(LOG_Bif_F12_FORMAT)
      {
        Q1(all_formats)
        Q1(all_formats.size())
        Q1(col_formats.size())

        loop(c, col_items.size())
            {
              CERR << "At " LOC " in format_by_example()" << endl
                   << "    col_items[c].left_deco:     '"
                   << col_items[c].left_deco << "'"      << endl
                   << "    col_items[c].int_part:      '"
                   << col_items[c].int_part << "'"       << endl
                   << "    col_items[c].fract_part:    '"
                   << col_items[c].fract_part << "'"     << endl
                   << "    col_items[c].expo_deco:     '"
                   << col_items[c].expo_deco << "'"      << endl
                   << "    col_items[c].expo_negative: "
                   << col_items[c].expo_negative         << endl
                   << "    col_items[c].right_deco:    '"
                   << col_items[c].right_deco << "'"     << endl;
            }
      }

Shape shape_Z(B.get_shape());
   if (B.is_scalar())   shape_Z.add_shape_item(1);
   shape_Z.set_last_shape_item(all_formats.size());

Value_P Z(shape_Z, LOC);

   try
      {
        loop(r, rows)
           {
             UCS_string row;
             loop(c, cols)
                {
                  const ShapeItem idx_B = c + r*cols;
                  if (!B.is_real_cell(idx_B))   DOMAIN_ERROR;

                  const APL_Float value = B.get_real_value(idx_B);

                  const UCS_string item = col_items[c].format_example(value);
                  Log(LOG_Bif_F12_FORMAT)   Q1(item)
                  row << item;
                }

             Log(LOG_Bif_F12_FORMAT)   { Q1(row) Q1(all_formats) }

             Assert(row.size() == all_formats.size());
             loop(c, row.size())
                Z->set_ravel_Char(r*all_formats.size() + c, row[c]);
           }
      }
   catch (const Error & err)
      {
        throw err;   // rethrow
      }
   catch (std::bad_alloc &)
      {
        WS_FULL;
      }
   catch (...)
      { FIXME; }

   return Z;
}
//────────────────────────────────────────────────────────────────────────────
void
Bif_F12_FORMAT::split_example_into_columns(const UCS_string & all_formats,
                                   UCS_string_vector & col_formats)
{
   // split string 'all_formats' into fields, where:
   //
   // 1.   a field contains at least one digit and is delimited by either
   // 1a.  a space (which belongs to the next field),  or
   // 1b.  digit '6' (which belongs to the field).
   //
bool digit_seen = false;
UCS_string current_format;
   loop(f, all_formats.size())
       {
         const Unicode cc = all_formats[f];
         current_format << cc;

         if (Avec::is_digit(cc))   digit_seen = true;

         if ((cc == UNI_SPACE) && digit_seen)   // end of field, case 1a.
            {
              col_formats.push_back(current_format);
              current_format.clear();    // start a new field;
              digit_seen = false;
            }
         else if (cc == UNI_6)                  // end of field, case 1b.
            {
              ++f;   // next char is right decorator (and end of field)
              if (f < all_formats.ssize())
                 current_format << all_formats[f];

              col_formats.push_back(current_format);
              current_format.clear();    // start a new field;
              digit_seen = false;
              continue;   // next char
            }
       }

   if (col_formats.size() && !digit_seen)
      {
        col_formats.back() << current_format;
      }
   else
      {
        col_formats.push_back(current_format);
      }
}
//────────────────────────────────────────────────────────────────────────────
PrintBuffer
Bif_F12_FORMAT::format_one_col_by_spec(int width, int precision,
                                       cValue_R B, ShapeItem base,
                                       ShapeItem cols, ShapeItem rows)
{
   // precision comes straight from the left argument with no upper bound
   // otherwise: a large positive precision drives an unbounded
   // UCS_string(size_t, Unicode)-style allocation in format_float_by_spec()
   // (via from_double_to_fixed()), and an equally unbounded (-precision)
   // does the same via from_double_to_expo() below -- either exhausts
   // memory and aborts uncleanly (glibc sysmalloc assertion, not a clean
   // bad_alloc). No legitimate format specification needs anywhere near
   // this many digits.
   if (precision > 1000 || precision < -1000)
      {
        MORE_ERROR() << "A⍕B: precision " << precision
                     << " is too far from 0 (expecting ¯1000..1000)";
        DOMAIN_ERROR;
      }

   // width comes straight from the left argument too, with no
   // non-negativity check otherwise: on overflow (data.ssize() > width)
   // below, a negative width makes that comparison always true (any
   // non-negative ssize() is > any negative int) and then
   // UCS_string(width, ...) sign-extends the negative int to a
   // near-SIZE_MAX count, whose internal reserve() throws
   // std::length_error. (A positive width of 2^31 truncates to INT_MIN
   // at the int parameter and reaches the same path.)
   if (width < 0)
      {
        MORE_ERROR() << "A⍕B: width " << width << " is negative";
        DOMAIN_ERROR;
      }

PrintBuffer ret;

bool has_char    = false;
bool has_num     = false;
bool has_complex = false;

   // determine the data types (character/numbers/complex) in B
   loop(r, rows)
      {
        if (B.is_numeric(base + r*cols))
           {
             has_num = true;
             if (B.is_complex_cell(base + r*cols))   has_complex = true;
           }
        else
           {
             has_char = true;
           }
      }

   if (has_complex)
      {
         // split cB into vectors real and imag...
         Value_P real(rows, LOC);
         Value_P imag(rows, LOC);
         loop(r, rows)
            {
              Cell cache;
              const Cell & cell = B.get_cravel(base + r*cols, cache);
              if (cell.is_complex_cell())
                 {
                   real->next_ravel_Float(cell.get_real_value());
                   imag->next_ravel_Float(cell.get_imag_value());
                 }
              else
                 {
                   real->next_ravel_Cell(cell);
                   imag->next_ravel_Char(UNI_SPACE);
                 }
            }

         PrintBuffer pb_real = format_one_col_by_spec(width, precision,
                                                      *real, 0, 1, rows);
         PrintBuffer pb_imag = format_one_col_by_spec(width, precision,
                                                      *imag, 0, 1, rows);

         PrintBuffer pb_real_imag;
         loop(r,rows)
             {
               UCS_string real_imag = pb_real.get_line(r);
               UCS_string imag = pb_imag.get_line(r);
               // pb_real's own internal column alignment (e.g. lining up
               // the 'E' of scientific notation across rows of differing
               // exponent width) can leave trailing padding on a row's
               // real part; strip it before appending 'J' + imag, or that
               // padding becomes a stray blank between the real and
               // imaginary parts, e.g. "2.00E0 J3.00E0" instead of
               // "2.00E0J3.00E0" (a formatting bug, not a real gap).
               // Symmetric with imag's own leading-whitespace strip below.
               //
               real_imag.remove_trailing_whitespaces();
               imag.remove_leading_whitespaces();
               if (imag.size())
                  {
                    real_imag << UNI_J << imag;
                  }
               pb_real_imag.append_ucs(real_imag);
             }
         return pb_real_imag;
      }

   // real B. (if B was initially complex then we arrive here twice: once
   // for the real parts and once for the imag parts).
   //
   loop(r, rows)
      {
        const ShapeItem idx_B = base + r*cols;
        if (B.is_character_cell(idx_B))
           {
             UCS_string data = UCS_string(B.get_char_value(idx_B));

             add_row(ret, r, has_char, has_num, UNI_E, data);
             continue;
           }

        if (Value_P value = B.try_pointer_value(idx_B))
           {
             UCS_string data = value->get_UCS_ravel();

             if (width && data.ssize() > width)   // overflow
                {
                  if (Workspace::get_FC(3) == UNI_0)   DOMAIN_ERROR;

                  data = UCS_string(width, Workspace::get_FC(3));
                }

             add_row(ret, r, has_char, has_num, UNI_E, data);
             continue;
           }

        if (!B.is_real_cell(idx_B))   DOMAIN_ERROR;

        APL_Float value = B.get_real_value(idx_B);
        if (!isfinite(value))
           {
             MORE_ERROR() << "A⍕B : Bad value " << value
                          << " (expecting a finite number)";
             DOMAIN_ERROR;
           }

        if (precision >= 0)   // integer or floating format
          {
            // precision == 0 means integer;
            // precision  > 0 is the number of digits AFTER the decimal point

            // fix values close to 0 as 0 so that, for example, we never
            // see ¯.0000 in the formatted output. The threshold below
            // which a value actually ROUNDS to all-zero digits at
            // 'precision' decimals is half a unit in the last place,
            // i.e. 0.5 x 10^-precision -- the previous 0.01-based
            // starting point was 50x too small at every precision, so
            // negative values in that gap escaped the clamp, rounded to
            // all zero digits anyway, and kept their sign (Blake
            // McBride, Bugs23 #5 -- e.g. 1⍕¯0.04 gave ¯.0 instead of
            // .0). Clamping only ever changes the sign here, never a
            // printed digit, since these values round to zero either way.
            //
            double minval = 0.5;
            loop(p,  precision)   minval /= 10;
            if (value < minval && value > -minval)   value = 0.0;

             UCS_string data = format_float_by_spec(value, precision);
             if (width && data.ssize() > width)   // overflow
                {
                  if (Workspace::get_FC(3) == UNI_0)   DOMAIN_ERROR;
                  data = UCS_string(width, Workspace::get_FC(3));
                }

             add_row(ret, r, has_char, has_num, UNI_E, data);
           }
        else                  // exponential format
           {
             // precision < 0 is the TOTAL (!) number of digits, which is
             // one more than the digits AFTER the decimal point.
             //
             const int fract_digits = (-precision) - 1;
             UCS_string data = UCS_string::from_double_to_expo(value,
                                                               fract_digits);
             add_row(ret, r, has_char, has_num, UNI_E, data);
          }
      }

   if (width && ret.get_column_count() < width)
      ret.pad_l(UNI_SPACE, width - ret.get_column_count());

   return ret;
}
//────────────────────────────────────────────────────────────────────────────
void
Bif_F12_FORMAT::add_row(PrintBuffer & ret, int row, bool has_char,
                        bool has_num, Unicode align_char, UCS_string & data)
{
   if (row == 0)         // first row: don't  align
      {
        ret.append_ucs(data);
      }
   else if (!has_char)   // only numbers: align at '.' or 'E'
      {
        ret.append_aligned(data, align_char);
      }
   else if (!has_num)    // only chars: align left
      {
        const int d = ret.get_column_count() - data.ssize();
        if      (d < 0)   ret.pad_r(UNI_SPACE, -d);
        else if (d > 0)   data << UCS_string(d, UNI_SPACE);
        ret.append_ucs(data);
      }
   else                 // chars and numbers: align right
      {
        const int d = ret.get_column_count() - data.size();
        if      (d < 0)   ret.pad_l(UNI_SPACE, -d);
        else if (d > 0)   data = UCS_string(d, UNI_SPACE) + data;
        ret.append_ucs(data);
      }
}
//────────────────────────────────────────────────────────────────────────────
UCS_string
Bif_F12_FORMAT::format_float_by_spec(APL_Float value, int precision)
{
UCS_string ret = UCS_string::from_double_to_fixed(value, precision);

   // Note: the examples in the apl standard use a leading 0 (like  0.00)
   // while lrm shows .00 instead. We follow lrm and remove a leading 0.
   // Interestingly, the exponential format leaves a leading 0 in both
   // the standard and lrm
   //
   if (precision)   // we have a '.'
      {
        if (ret[0] == UNI_0    &&
            ret[1] == UNI_FULLSTOP)        // 0.xxx → .xxx
           {
             ret.erase(0);
           }
        else if (ret[0] == UNI_OVERBAR     &&
                 ret[1] == UNI_0     &&
                 ret[2] == UNI_FULLSTOP)   //  ¯0.x → ¯.x
           {
             ret[1] = UNI_OVERBAR;
             ret.erase(0);
           }
      }
   else if (ret.size() == 2       &&
            ret[0] == UNI_OVERBAR &&
            ret[1] == UNI_0)               // ¯0 → 0
           {
             ret.erase(0);
           }

   return ret;
}
//════════════════════════════════════════════════════════════════════════════
ostream &
operator <<(ostream & out, const Format_sub & fmt)
{
   return fmt.print(out);
}
//════════════════════════════════════════════════════════════════════════════
