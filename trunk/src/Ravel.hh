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

#ifndef __RAVEL_HH_DEFINED__
#define __RAVEL_HH_DEFINED__

#include "IntCell.hh"

class ValueBase;
class cValue;
class Value;

//════════════════════════════════════════════════════════════════════════════
/// The ravel of an APL value: the flat array of Cell objects together with
/// the short-value inline buffer, the fill/fetch function pointer, and the
/// initialization bookkeeping counters.  Factored out of ValueBase so that
/// a future class Ravel can own and manage the storage independently.
class Ravel
{
   friend class ValueBase;
   friend class cValue;
   friend class Value;

public:
   Ravel()
   : fetcher(0),
     valid_ravel_items(0),
     nz_subcell_count(0),
     cells(0)
   {}

   //══════════════════════════════════════════════════════════════════════════
   // Read-only (const) methods
   //══════════════════════════════════════════════════════════════════════════

   /// return the idx'th cell of the ravel (no bounds check).
   /// @param idx ravel index (0-based)
   const Cell & get_cravel(ShapeItem idx) const
      { return fetcher(idx, cells); }

   /// return the first ravel cell (for non-empty ravels).
   const Cell & get_cfirst() const   { return get_cravel(0); }

   /// return the first ravel cell (the prototype of an empty value).
   const Cell & get_cproto() const   { return get_cravel(0); }

   /// return the single ravel cell of a scalar.
   const Cell & get_cscalar() const  { return get_cravel(0); }

   /// fetch function for unpacked (Cell-array) ravels.
   /// @param offset ravel index (0-based)
   /// @param cells pointer to the Cell array
   static const Cell & cell_fetcher(ShapeItem offset, const Cell * cells)
      { return cells[offset]; }

   /// fetch function for packed (bit-array) boolean ravels.
   /// @param offset ravel index (0-based)
   /// @param cells pointer to the packed bit array cast to Cell *
   static const Cell & packed_fetcher(ShapeItem offset, const Cell * cells)
      { return 1 << (offset & 7) &
               reinterpret_cast<const uint8_t *>(cells)[offset >> 3]
             ? IntCell::boolean_TRUE : IntCell::boolean_FALSE;
      }

   //══════════════════════════════════════════════════════════════════════════
   // Read-write methods
   //══════════════════════════════════════════════════════════════════════════

   /// return the idx'th cell of the ravel (no bounds check).
   /// @param idx ravel index (0-based)
   Cell & get_wravel(ShapeItem idx)   { return cells[idx]; }

   /// return the first ravel cell (for non-empty ravels).
   Cell & get_wfirst()    { return get_wravel(0); }

   /// return the single ravel cell of a scalar.
   Cell & get_wscalar()   { return get_wravel(0); }

   /// return the first ravel cell (the prototype of an empty value).
   Cell & get_wproto()    { return get_wravel(0); }

protected:
   //══════════════════════════════════════════════════════════════════════════
   // Data members
   //══════════════════════════════════════════════════════════════════════════

   /// function that fetches one cell; differs for packed vs. unpacked ravels.
   const Cell & (*fetcher)(ShapeItem offset, const Cell * cells);

   /// number of initialized cells in the ravel (excluding prototype).
   ShapeItem valid_ravel_items;

   /// number of cells in nested sub-values.
   ShapeItem nz_subcell_count;

   /// pointer to the ravel cells (points to short_value for short ravels,
   /// or to a heap-allocated array for longer ones).
   Cell * cells;

   /// inline storage for short (≤ cfg_SHORT_VALUE_LENGTH_WANTED) ravels.
   Cell short_value[cfg_SHORT_VALUE_LENGTH_WANTED];
};
//════════════════════════════════════════════════════════════════════════════

#endif // __RAVEL_HH_DEFINED__
