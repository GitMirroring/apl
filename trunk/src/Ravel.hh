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

#include "CharCell.hh"
#include "ComplexCell.hh"
#include "FloatCell.hh"
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
     cells(0),
     fetch_cache(0)
   {}

   //══════════════════════════════════════════════════════════════════════════
   // Read-only (const) methods
   //══════════════════════════════════════════════════════════════════════════

   /// return the idx'th cell of the ravel (no bounds check).
   /// @param idx ravel index (0-based)
   const Cell & get_cravel(ShapeItem idx) const
      { return fetcher(idx, cells, cell_fetch_cache); }

   /// return the first ravel cell (for non-empty ravels).
   const Cell & get_cfirst() const   { return get_cravel(0); }

   /// return the first ravel cell (the prototype of an empty value).
   const Cell & get_cproto() const   { return get_cravel(0); }

   /// return the single ravel cell of a scalar.
   const Cell & get_cscalar() const  { return get_cravel(0); }

   /// fetch function for unpacked (Cell-array) ravels.
   /// @param offset ravel index (0-based)
   /// @param cells pointer to the Cell array
   /// @param cache unused (required by fetcher signature)
   static const Cell & cell_fetcher(ShapeItem offset, const Cell * cells, Cell &)
      { return cells[offset]; }

   /// fetch function for packed (bit-array) boolean ravels.
   /// @param offset ravel index (0-based)
   /// @param cells pointer to the packed bit array cast to Cell *
   /// @param cache unused (required by fetcher signature)
   static const Cell & packed_fetcher(ShapeItem offset, const Cell * cells, Cell &)
      { return 1 << (offset & 7) &
               reinterpret_cast<const uint8_t *>(cells)[offset >> 3]
             ? IntCell::boolean_TRUE : IntCell::boolean_FALSE;
      }

   /// fetch function for packed int64_t ravels.
   static const Cell & int64_fetcher(ShapeItem offset, const Cell * cells, Cell & cache)
      { new (&cache) IntCell(reinterpret_cast<const int64_t *>(cells)[offset]);
        return cache; }

   /// fetch function for packed double ravels.
   static const Cell & float64_fetcher(ShapeItem offset, const Cell * cells, Cell & cache)
      { new (&cache) FloatCell(reinterpret_cast<const double *>(cells)[offset]);
        return cache; }

   /// fetch function for packed complex (2×double) ravels.
   static const Cell & complex_fetcher(ShapeItem offset, const Cell * cells, Cell & cache)
      { const double * p = reinterpret_cast<const double *>(cells) + 2*offset;
        new (&cache) ComplexCell(p[0], p[1]);
        return cache; }

   /// fetch function for packed 16-bit Unicode ravels.
   static const Cell & char16_fetcher(ShapeItem offset, const Cell * cells, Cell & cache)
      { new (&cache) CharCell(Unicode(reinterpret_cast<const uint16_t *>(cells)[offset]));
        return cache; }

   /// fetch function for packed 32-bit Unicode ravels.
   static const Cell & char32_fetcher(ShapeItem offset, const Cell * cells, Cell & cache)
      { new (&cache) CharCell(reinterpret_cast<const Unicode *>(cells)[offset]);
        return cache; }

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
   const Cell & (*fetcher)(ShapeItem offset, const Cell * cells, Cell & cache);

   /// number of initialized cells in the ravel (excluding prototype).
   ShapeItem valid_ravel_items;

   /// number of cells in nested sub-values.
   ShapeItem nz_subcell_count;

   /// pointer to the ravel cells (points to short_value for short ravels,
   /// or to a heap-allocated array for longer ones).
   Cell * cells;

   /// inline storage for short (≤ cfg_SHORT_VALUE_LENGTH_WANTED) ravels.
   Cell short_value[cfg_SHORT_VALUE_LENGTH_WANTED];

   /// per-ravel scratch int64 used by fetch_ravel_i64() for sub-word types
   /// (RT_BOOL, RT_UNICODE16, RT_UNICODE32) that cannot return a direct pointer.
   mutable int64_t fetch_cache;

   /// per-ravel Cell cache for get_cravel() on packed (non-Cell) ravels.
   mutable Cell cell_fetch_cache;
};
//════════════════════════════════════════════════════════════════════════════

#endif // __RAVEL_HH_DEFINED__
