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

#include <memory>
#include <sys/types.h>

#include "Bif_F12_PARTITION_PICK.hh"
#include "CDR_string.hh"
#include "CharCell.hh"
#include "ComplexCell.hh"
#include "Common.hh"
#include "Error.hh"
#include "FloatCell.hh"
#include "Heapsort.hh"
#include "IndexExpr.hh"
#include "IndexIterator.hh"
#include "IntCell.hh"
#include "IO_Files.hh"
#include "LvalCell.hh"
#include "Macro.hh"
#include "Output.hh"
#include "PointerCell.hh"
#include "Parallel.hh"
#include "PrintOperator.hh"
#include "Quad_XML.hh"
#include "StateIndicator.hh"
#include "SystemVariable.hh"
#include "UCS_string.hh"
#include "UserFunction.hh"
#include "Value.hh"
#include "ValueHistory.hh"
#include "Workspace.hh"

#include "Workspace.icc"

extern uint64_t top_of_memory();

uint64_t Value::value_count = 0;
uint64_t Value::total_ravel_count = 0;

// most static members of class Value are defined in StaticObjects.cc

_deleted_value * Value::deleted_values = 0;

int Value::deleted_values_count = 0;

uint64_t Value::fast_new_count = 0;

uint64_t Value::slow_new_count = 0;

uint64_t Value::alloc_size = 0;

//════════════════════════════════════════════════════════════════════════════
Value::Value(const char * loc)
   : DynamicObject(loc, &all_values)
{
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();
}
//────────────────────────────────────────────────────────────────────────────
Value::Value(const Cell & cell, const char * loc)
   : DynamicObject(loc, &all_values)
{
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();

   set_ravel_Cell(0, cell);
   check_value(LOC);
}
//────────────────────────────────────────────────────────────────────────────
Value::Value(ShapeItem len, const char * loc)
   : DynamicObject(loc, &all_values)
{
   shape = Shape(len);
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();
}
//────────────────────────────────────────────────────────────────────────────
Value::Value(ShapeItem rows, ShapeItem cols, const char * loc)
   : DynamicObject(loc, &all_values)
{
   shape = Shape(rows, cols);
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();
}
//────────────────────────────────────────────────────────────────────────────
Value::Value(const Shape & sh, const char * loc)
   : DynamicObject(loc, &all_values)
{
   shape = sh;
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();
}
//────────────────────────────────────────────────────────────────────────────
Value::Value(const Shape & sh, uint64_t * bits, const char * loc)
   : DynamicObject(loc, &all_values),
     owner_count(0),
     pointer_cell_count(0)
{
   shape = sh;
   fetcher = &packed_fetcher;
   flags = VF_packed;
   valid_ravel_items = sh.get_nz_volume();
   ravel = reinterpret_cast<Cell *>(bits);
   ADD_EVENT(this, VHE_Create, 0, loc);
   check_ptr = charP(this) + 7;

   if (ravel)   return;   // caller has allocated

   // round the size up to the next 64 bit boundary.
const size_t uint64_count = (sh.get_nz_volume() + 63) >> 6;
   bits = new uint64_t[uint64_count];
   loop(u, uint64_count)   bits[u] = 0;
   ravel = reinterpret_cast<Cell *>(bits);
   set_complete();
}
//────────────────────────────────────────────────────────────────────────────
Value::Value(const UCS_string & ucs, const char * loc)
   : DynamicObject(loc, &all_values)
{
   shape = Shape(ucs.size());
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();

   set_proto_Spc();                          // prototype
   loop(l, ucs.size())   next_ravel_Char(ucs[l]);
   set_complete();
}
//────────────────────────────────────────────────────────────────────────────
Value::Value(const UTF8_string & utf, const char * loc)
   : DynamicObject(loc, &all_values)
{
   shape = Shape(utf.size());
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();

   set_proto_Spc();                          // prototype
   loop(l, utf.size())   next_ravel_Char(Unicode(utf[l] & 0xFF));
   set_complete();
}
//────────────────────────────────────────────────────────────────────────────
Value::Value(const CDR_string & ui8, const char * loc)
   : DynamicObject(loc, &all_values)
{
   shape = Shape(ui8.size());
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();

   set_proto_Spc();                          // prototype
   loop(l, ui8.size())   next_ravel_Char(Unicode(ui8[l]));
   set_complete();
}
//────────────────────────────────────────────────────────────────────────────
Value::Value(const PrintBuffer & pb, const char * loc)
   : DynamicObject(loc, &all_values)
{
   shape = Shape(pb.get_row_count(), pb.get_column_count());
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();

   set_proto_Spc();                          // prototype

const ShapeItem height = pb.get_row_count();
const ShapeItem width = pb.get_column_count();

   loop(y, height)
   loop(x, width)   next_ravel_Char(pb.get_char(x, y));

   set_complete();
}
//────────────────────────────────────────────────────────────────────────────
Value::Value(const char * loc, const Shape * sh)
   : DynamicObject(loc, &all_values)
{
   shape = Shape(ShapeItem(sh->get_rank()));
   ADD_EVENT(this, VHE_Create, 0, loc);
   init_ravel();

   IntCell::z0(&get_wproto());   // prototype

   loop(r, sh->get_rank())   next_ravel_Int(sh->get_shape_item(r));

   set_complete();
}
//────────────────────────────────────────────────────────────────────────────
Value::~Value()
{
   ADD_EVENT(this, VHE_Destruct, 0, LOC);
   unlink();

   if (flags & VF_packed)
      {
        uint8_t * bits = reinterpret_cast<uint8_t *>(ravel);
        delete[] bits;
        ravel = 0;
        Assert(check_ptr == charP(this) + 7);
        check_ptr = 0;
        return;
      }

const ShapeItem length = nz_element_count();

#if APL_Float_is_class

   //  APL_Float is a class, therefore we need to release all Cells

#else

   // APL_Float is NOT a class, Therefore release only PointerCells
   if (get_pointer_cell_count() > 0)

#endif
      {
        Cell * cZ = &get_wfirst();
        loop(c, length)
            {
              if (get_pointer_cell_count() == 0)   break;
              cZ++->release(LOC);
            }
      }

   Assert1(get_pointer_cell_count() == 0);

   --value_count;

   if (ravel == 0)   return;   // new() failed

   if (ravel != short_value)   // long value
      {
        total_ravel_count -= length;
        std::allocator<Cell>{}.deallocate(ravel, length);
      }

   Assert(check_ptr == charP(this) + 7);
   check_ptr = 0;
}
//────────────────────────────────────────────────────────────────────────────
Value_P
Value::get_cellrefs(const char * loc)
{
   /* Create a non-nested (!) (left-) value left_Z from this (right-) value.
      left_Z has the same shape and consists entirely of LvalCells that point
      to the corresponding cells of this value.

      NOTE that any PointerCell in this are NOT handled differently than
      simple cells (and get_cellrefs() will simply point to them)
      rather than recursing into its sub-value. For that reason:

      1. the result left_Z below is flat (not nested), and
      2. assign_cellrefs() below must (at its later point in time),call
         get_cellrefs() when it encounters an LvalCell whose target is a
         PointerCell.

      The reason for delaying the recursion into sub-values is that the
      subsequent processing of left_Z may erase those sub-values and therefore
      doing it now may render useless later.
   */

Value_P left_Z(get_shape(), loc);

   // next_ravel_Cell() is 0 for empty values.
   // Therefore empty values (with element_count == 0) must use
   // set_ravel_Cell() instead of next_ravel_Cell() !
   //
   if (const ShapeItem ec = element_count())
      {
        loop(e, ec)
            {
              Cell & right_cell = get_wravel(e);
              const LvalCell left_cell(&right_cell, this);
              left_Z->next_ravel_Cell(left_cell);
            }
      }
   else   // prototype
      {
        left_Z->set_ravel_Cell(0, LvalCell(&get_wproto(), this));
      }

   left_Z->check_value(LOC);
   return left_Z;
}
//────────────────────────────────────────────────────────────────────────────
void
Value::assign_cellrefs(Value_P new_value)
{
   // assign new_value to this (left-) value

const ShapeItem new_value_count = new_value->nz_element_count();
const ShapeItem dest_count  = element_count();
   if (dest_count == 0)   return;   // nothing to assign

const int dest_incr = (dest_count == 1)                    ? 0 : 1;
const int src_incr  = (new_value->nz_element_count() == 1) ? 0 : 1;

   if (src_incr  &&   // this is not a acalar or 1-element value, and
       dest_incr &&   // new_value  is not a acalar or 1-element value, and
       !this->conforms_to(*new_value))   // non-trivial shape mismatch
      {
        // NOTE:  M←3 2ρι6 ◊ (1 0/M) ← 'abc' succeeds in IBM APL2, even
        //        though the shapes of 1 0/M and 'abc' differ. We therefore
        //
        MORE_ERROR() << "in (f Z)←B: length(X⊃F Z) is " << dest_count
                     << ", but length(X⊃V) is " << new_value_count
                     << " (for some X)";
        LENGTH_ERROR;
      }

   /* this: a value containing LvalCells and possibly PointerCells.

      test with:

      C←"001011010010010110010100100001000010001101" ◊ ((C='0')/C)←' ' ◊ C
      K←'RED' 'WHITE' 'BLUE'   ◊ (↑K)←'YELLOW' ◊ K
      V←'v' 'vw' 'vwx' 'vwxyz' ◊ (2↑¨V)←5      ◊ 4 ⎕CR V

    */

   // consistency check: all items are LvalCells or PointerCells.
   //
   check_lval_consistency();

   if (is_scalar() && !new_value->is_scalar())
      {
        const Cell * C0 = &get_cscalar();
        if (!C0->is_lval_cell())   LEFT_SYNTAX_ERROR;
        const LvalCell * LVC0 = reinterpret_cast<const LvalCell *>(C0);
        if (Cell * target = LVC0->get_lval_value())   // valid right Cell
           {
             Value & owner = *LVC0->get_cell_owner();
             target->release(LOC);   // free sub-values etc (if any)
             new (target)   PointerCell(new_value.get(), owner);
           }
        return;
      }

   loop(d, dest_count)
      {
        const Cell & src = new_value->get_cravel(d * src_incr);
        Cell & dest = get_wravel(d);
        if (dest.is_pointer_cell())
           {
             /*
                NOTE: At this point dest is one of the left cells in a
                      selective assignment, say:

                      (f1 f2 ... fn VAR) ← new_value.

                The initial cellrefs (i.e. of VAR) contains no PointerCells
                (though possibly Lval cells pointing to PointerCells).

                Therefore dest must have been created by one of the fi and
                then all sub-values of dest must also be Lval Cells.
              */
              Value_P left_sub = dest.get_pointer_value();
              Value_P right_sub;
              if (src.is_pointer_cell())   right_sub = src.get_pointer_value();
              if (+right_sub)
                 {
                   if (left_sub->get_rank() != right_sub->get_rank())
                      {
                        MORE_ERROR() <<
                        "In selective specification: left rank is " <<
                        left_sub->get_rank() << ", but right rank is " <<
                        right_sub->get_rank();
                        RANK_ERROR;
                      }
                   if (left_sub->get_shape() != right_sub->get_shape())
                      {
                        const Shape & left_shape = left_sub->get_shape();
                        const Shape & right_shape = right_sub->get_shape();

                        MORE_ERROR()                                      <<
                            "In selective specification: left shape is: " <<
                            left_shape                                    <<
                            ", but right shape is: "                      <<
                            right_shape;
                        LENGTH_ERROR;
                      }
                 }
              loop(s, left_sub->nz_element_count())
                  {
                    Cell * Csub = &left_sub->get_wravel(s);
                    if (!Csub->is_lval_cell())   LEFT_SYNTAX_ERROR;
              
                    const LvalCell * LVC =
                          reinterpret_cast<const LvalCell *>(Csub);
                    Cell * target = LVC->get_lval_value();
                    if (target)   // target can be 0!
                       {
                         target->release(LOC);   // free sub-values etc.
                         Value & owner = *LVC->get_cell_owner();

                         // if src is simple, then scalar extend it.
                         // Otherwise use item s of src.
                         //
                         const Cell & right_cell = +right_sub ?
                                     right_sub->get_cravel(s) : src;
                         target->init(right_cell, owner, LOC);
                       }
                  }
           }
        else if (dest.is_lval_cell())
           {
             const LvalCell & LVC = reinterpret_cast<const LvalCell &>(dest);
             if (Cell * target = dest.get_lval_value())   // target can be 0!
                {
                  target->release(LOC);   // free sub-values etc (if any)

                  // erase the pointee when overriding a pointer-cell.
                  //
                  Value & owner = *LVC.get_cell_owner();
                  target->init(src, owner, LOC);
                }
           }
        else   LEFT_SYNTAX_ERROR;
      }
}
//────────────────────────────────────────────────────────────────────────────
Cell *
Value::get_member(const vector<const UCS_string *> & members,
                  Value * & owner, bool throw_error)
{
   owner = this;

   // members[members.size() - 1] == this
   //
   for (int m = members.size() - 2; m >= 0; --m)
       {
         if (!owner->is_member())
            {
              UCS_string & more = MORE_ERROR()
                  << "member access: non-structured variable "
                  << *members.back() << " has no member ";
              more.append_members(members, 0);
              if (throw_error)   DOMAIN_ERROR;
              else               return 0;
            }

         const UCS_string & member_ucs = *members[m];
         if (owner->get_rank() != 2)
            {
              if (!throw_error)   return 0;
              UCS_string & more = MORE_ERROR() << "member access: the rank of ";
              more.append_members(members, m);
              more << " is not 2.\n"
                      "Expecting an N×2 matrix of member,value pairs.";
              RANK_ERROR;
            }

         if (owner->get_cols() != 2)
            {
              if (!throw_error)   return 0;
              UCS_string & more = MORE_ERROR()
                 << "member access: the number of columns of ";
              more.append_members(members, m);
              more << " is not 2.\n"
                      "Expecting an N×2 matrix of member,value pairs.";
              LENGTH_ERROR;
            }

         Cell * member_cell = owner->get_member_data(member_ucs);
         if (member_cell)   // existing member
            {
              if (m == 0)   return member_cell; // final member

              // more members coming. Then member_cell should point to a
              // structured sub-member
              //
              if (!member_cell->is_pointer_cell() ||
                  !member_cell->get_pointer_value()->is_member())
                 {
                   if (!throw_error)   return 0;
                   UCS_string & more = MORE_ERROR()
                                << "member access: member " << member_ucs
                                << " exists in ";
                   more.append_members(members, m);
                   more << " but its (internal) value is not nested";
                   DOMAIN_ERROR;
                 }

              // next member
              //
              owner = member_cell->get_pointer_value().get();
            }
         else   // member does not exist
            {
              // create new member row...

              // find an unused slot in owner. get_new_member() is guaranteed
              // to find one (by possibly increasing the ravel of owner.
              //
              Cell * member_data = owner->get_new_member(member_ucs);
              if (m == 0)   return member_data;

              // more members coming
              //
              Value_P member_sub = EmptyStruct(LOC);
              new (member_data)   PointerCell(member_sub.get(), *owner);
              owner = member_sub.get();
            }   // if (member_cell == 0)
       }

   FIXME;   // not reached
}
//────────────────────────────────────────────────────────────────────────────
Cell *
Value::get_member_data(const UCS_string & member_name)
{
const ShapeItem rows = get_rows();

ShapeItem row = member_name.FNV_hash() % rows;

   // loop over the member rows. Since we hash the start position this will
   // typically return quickly and the slow case where all rows are tested
   // will throw a VALUE ERROR right after returning 0.
   //
   loop(r, rows)
       {
         if (++row >= rows)   row = 0;
         const Cell & name_cell = get_cravel(2*row);

         if (name_cell.is_pointer_cell() &&
             name_cell.get_pointer_value()->equal_string(member_name))
            return &get_wravel(2*row + 1);
       }

   // member row not found
   //
   return 0;
}
//────────────────────────────────────────────────────────────────────────────
Cell *
Value::get_new_member(const UCS_string & new_member_name)
{
   // find an unused slot in owner. Return its data cell (= its name cell + 1)
   //
   for (;;)   // until an unused row was found
       {
         const ShapeItem rows = get_rows();   // changed by double_ravel() !
         ShapeItem row = new_member_name.FNV_hash() % rows;
         loop(r, 14)
             {
               if (++row >= rows)   row = 0;
               Cell * cell = &get_wravel(2*row);
               if (cell->is_integer_cell() &&
                   cell->get_int_value() == 0)   // unused row
                  {
                    Value_P name_val(new_member_name, LOC);
                    new (cell) PointerCell(name_val.get(), *this);
                    return cell + 1;
                  }
             }

          /* at this point a cluster of 14 consecutive used rows was hit.
             Common wisdom (Knuth) has it, that a hash table should maintain
             fill level (i.e. used rows ÷ all rows) below 50%.

             For a hash table with N rows and an unknown fill level (like this
             one) we could either:

             A1. first determine the fill level (an O(N) operation), and
             A2. double the table size (another O(n) operation) if the fill
                 level is > 50 %,

                 or:

             B.   double the table size regardless of the fill level (and
                  an 1:16384 chance that the doubling was premature).


             We choose B. because it is simpler and because prematurely
             doubling the table could very well amortize itself later.
          */

         double_ravel(LOC);
       }
}
//────────────────────────────────────────────────────────────────────────────
void
Value::double_ravel(const char * loc)
{
Cell * const old_ravel = ravel;

   Assert(is_member());
const char * del = 0;
   if (ravel != short_value)   del = reinterpret_cast<char *>(ravel);

   Assert(get_rank() == 2);
   Assert(get_cols() == 2);

const ShapeItem old_rows  = get_rows();
const ShapeItem new_rows  = 2*old_rows;
const ShapeItem new_cells = 2*new_rows;

Cell * doubled = new Cell[new_cells];
   loop(n, new_cells)   IntCell::z0(doubled + n);
   valid_ravel_items = new_cells;
   shape.set_shape_item(0, new_rows);
   ravel = doubled;

   loop(r, old_rows)
       {
         // transfer the member name. The old member_name_cell must be released
         // since get_new_member() below creates a new one in the new ravel.
         //
         Cell & member_name_cell = old_ravel[2*r];
         if (!member_name_cell.is_pointer_cell())   continue;   // unused
         Assert(member_name_cell.is_pointer_cell());
         UCS_string member_name(*member_name_cell.get_pointer_value());
         member_name_cell.release(LOC);

         // transfer the member value. memcpy() should work because ownership
         // remains the same
         //
         void * dest = get_new_member(member_name);
         void * old_member_data_cell = old_ravel + 2*r + 1;
         memcpy(dest, old_member_data_cell, sizeof(Cell));
       }

   delete [] del;   // del is char *, so no cell destructor is called
}
//────────────────────────────────────────────────────────────────────────────
bool
Value::is_or_contains(const cValue * value, const cValue * sub)
{
   if (value == 0)     return false;   // value is not a valid value
   if (value == sub)   return true;    // value is sub

   for (ConstRavel_P v(*value, true); +v; ++v)
      {
        if (v->is_pointer_cell() &&
            is_or_contains(v->get_pointer_value().get(), sub))   return true;
      }

   return false;
}
//────────────────────────────────────────────────────────────────────────────
void
Value::init()
{
   Log(LOG_startup)
      CERR << "Max. Rank            is " << MAX_RANK << endl;
};
//────────────────────────────────────────────────────────────────────────────
void
Value::add_member(const UCS_string & member_name, Value * member_value)
{
   if (!is_structured())
      {
        MORE_ERROR() << "attempt to add member (" << member_name
                     << "to a non-structured value";
        DOMAIN_ERROR;
      }

vector<const UCS_string *> members;
   members.push_back(&member_name);
   members.push_back(&member_name);   // ignored

Value * member_owner = 0;
Cell * data = get_member(members, member_owner, false);
   Assert(member_owner);
   Assert(member_owner == this);
   new (data) PointerCell(member_value, *this);
}
//────────────────────────────────────────────────────────────────────────────
void
Value::mark_all_dynamic_values()
{
   for (DynamicObject * dob = DynamicObject::all_values.get_prev();
        dob != &all_values; dob = dob->get_prev())
       {
         dob->rValue().set_marked();
       }
}
//────────────────────────────────────────────────────────────────────────────
void
Value::rollback(ShapeItem items, const char * loc)
{
   ADD_EVENT(this, VHE_Unroll, 0, loc);

   // this value has only items < nz_element_count() valid items.
   // init the rest...
   //
   while (more())   next_ravel_0();

   alloc_loc = loc;
}
//────────────────────────────────────────────────────────────────────────────
void
Value::check_value(const char * loc)
{
#ifdef cfg_VALUE_CHECK_WANTED

   // if value was initialized by means of a next_ravel_XXX() mechanism,
   // then all cells are supposed to be OK.
   //
   if (valid_ravel_items && valid_ravel_items >= element_count())
      {
        set_complete();
        return;
      }

uint32_t error_count = 0;
const Cell * C = &get_cfirst();

const ShapeItem ec = nz_element_count();
    loop(c, ec)
       {
         const CellType ctype = C->get_cell_type();
         switch(ctype)
            {
              case CT_CHAR:
              case CT_POINTER:
              case CT_CELLREF:
              case CT_INT:
              case CT_FLOAT:
              case CT_COMPLEX:   break;   // OK

              default:
                 CERR << endl
                      << "*** check_value(" << loc << ") detects:" << endl
                      << "   bad ravel[" << c << "] (CellType "
                      << ctype << ")" << endl;

                 ++error_count;
            }

         if (error_count >= 10)
            {
              CERR << endl << "..." << endl;
              break;
            }

         ++C;
       }

   if (error_count)
      {
        CERR << "Shape: " << get_shape() << endl;
        print(CERR) << endl
           << "************************************************"
           << endl;
        Assert(0 && "corrupt ravel ");
      }
#endif

   set_complete();
}
//────────────────────────────────────────────────────────────────────────────
int
Value::erase_stale(const char * loc)
{
int count = 0;

   Log(LOG_Value__erase_stale)
      CERR << endl << endl << "erase_stale() called from " << loc << endl;

   for (DynamicObject * dob = all_values.get_next();
        dob != &all_values; dob = dob->get_next())
       {
         Value & v = dob->rValue();
         if (dob == dob->get_next())   // a loop
            {
              CERR << "A loop in DynamicObject::all_values (detected in "
                      "function erase_stale() at object "
                   << voidP(dob) << "): " << endl;
              all_values.print_chain(CERR);
              CERR << endl;

              // use separate CERR << in case this crashes
              //
              CERR << " DynamicObject: " << dob << endl;
              CERR << " Value:         " << v   << endl;
              CERR << v                         << endl;
            }

         Assert(dob != dob->get_next());
         if (v.owner_count)   continue;

         ADD_EVENT(&v, VHE_Stale, v.owner_count, loc);

         Log(LOG_Value__erase_stale)
            {
              CERR << "Erasing stale Value "
                   << voidP(dob) << ":" << endl
                   << "  Allocated by " << v.where_allocated() << endl
                   << "  ";
              v.list_one(CERR, false);
            }

         // count it unless we know it is dirty
         //
         ++count;

         dob->unlink();

         // v->erase(loc) could mess up the chain, so we start over
         // rather than continuing
         //
         dob = &all_values;
       }

   return count;
}
//────────────────────────────────────────────────────────────────────────────
void
Value::erase_all(ostream & out)
{
   for (const DynamicObject * dob = DynamicObject::all_values.get_next();
        dob != &DynamicObject::all_values; dob = dob->get_next())
       {
         const Value & value = dob->rValue();
         out << "erase_all() sees Value:" << endl
             << "  Allocated by " << value.where_allocated() << endl
             << "  ";
         value.list_one(CERR, false);
       }
}
//────────────────────────────────────────────────────────────────────────────
ostream &
Value::list_all(ostream & out, bool show_owners)
{
int num = 0;
   for (const DynamicObject * dob = all_values.get_prev();
        dob != &all_values; dob = dob->get_prev())
       {
         out << "Value #" << num++ << ":";
         dob->rValue().list_one(out, show_owners);
       }

   return out << endl;
}
//────────────────────────────────────────────────────────────────────────────
void
Value::to_type(bool force_numeric)
{
   loop(e, nz_element_count())
      {
        Cell & cell = get_wravel(e);
        if (cell.is_pointer_cell())
           {
             PointerCell & ptr_cell = reinterpret_cast<PointerCell &>(cell);
             ptr_cell.isolate(LOC);
             ptr_cell.get_pointer_value()->to_type(force_numeric);
           }
        else if (cell.is_character_cell() && ! force_numeric)
           {
             set_ravel_Char(e, UNI_SPACE);
           }
        else
           {
             set_ravel_Int(e, 0);
           }
      }
}
//────────────────────────────────────────────────────────────────────────────
void
Value::explode()
{
   Assert(is_packed());
const uint8_t * bits = reinterpret_cast<const uint8_t *>(ravel);

IntCell * new_ravel = 0;
   try           { new_ravel = new IntCell[element_count()]; }
   catch (std::bad_alloc &) { WS_FULL }
   catch (...)              { FIXME; }

   loop(b, element_count())
      if (bits[b >> 3] & (1 << (b & 7)))   new_ravel[b].set_int_value(1);

   delete [] bits;
   ravel = new_ravel;
   clear_packed();
   fetcher = &cell_fetcher;
}
//────────────────────────────────────────────────────────────────────────────
const char *
Value::try_implode()
{
   if (is_packed())          return "value already packed";
   if (pointer_cell_count)   return "value not simple";

   // at this point we are optimistic that all B-items are boolean.
   //
const ShapeItem Z_len = (nz_element_count() + 63) >> 6;
uint64_t * bits = new uint64_t[Z_len];
const char * error_reason = 0;

   if (bits == 0)   return "WS FULL";

   // first set all bits to 0
   //
   loop(z, Z_len)   bits[z] = 0;

   // then set some bits to 1
   //
uint64_t zchunk = 0;
uint64_t zbit  = 1;

ShapeItem z = 0;
   for (ConstRavel_P b(*this, true); +b; ++b)
       {
         if (!b->is_near_bool())
            {
              error_reason = "value has non-boolean items";
              break;
            }

         // OK, bit is 0 or 1. Already done if bit is 0.
         if (b->get_near_bool())   zchunk |= zbit;   // bit in B is set

         zbit += zbit;                   // next bit
         if (zbit)   continue;           // more bits in same chunk
         bits[z++] = zchunk;
         zchunk = 0;
       }

   if (zchunk)   bits[Z_len - 1] = zchunk;   // rest bits

   if (error_reason)
      {
        delete[] bits;
      }
   else
      {
        if (ravel != short_value)   std::allocator<Cell>{}.deallocate(ravel, nz_element_count());
        ravel = reinterpret_cast<Cell *>(bits);
        flags |= VF_packed;
      }

   return error_reason;
}
//────────────────────────────────────────────────────────────────────────────
int
Value::print_incomplete(ostream & out)
{
std::vector<const cValue *> incomplete;
bool goon = true;

   for (const DynamicObject * dob = all_values.get_prev();
        goon && (dob != &all_values); dob = dob->get_prev())
       {
         const Value & value = dob->rValue();
         goon = (dob != dob->get_prev());

         if (value.is_complete())   continue;

         out << "incomplete value at " << voidP(&value) << endl;
         incomplete.push_back(&value);

         if (!goon)
            {
              out << "Value::print_incomplete() : endless loop in "
                     "Value::all_values; stopping display." << endl;
            }
       }

   // then print more info...
   //
int count = 0;
   loop(s, incomplete.size())
      {
        { const Value * vp = static_cast<const Value *>(incomplete[s]);
          vp->print_stale_info(out, vp); }
        if (++count > 20)   // its getting boring
           {
             CERR << endl << " ... ( " << (incomplete.size() - count) 
                  << " more incomplete values)..." << endl;
             break;
           }
      }

   return incomplete.size();
}
//────────────────────────────────────────────────────────────────────────────
int
Value::print_stale(ostream & out)
{
std::vector<const cValue *> stale_vals;
std::vector<const DynamicObject *> stale_dobs;
bool goon = true;
int count = 0;

   // first print addresses and remember stale values
   //
   for (const DynamicObject * dob = all_values.get_prev();
        goon && (dob != &all_values); dob = dob->get_prev())
       {
         const Value & value = dob->rValue();
         goon = (dob != dob->get_prev());

         if (value.owner_count)   continue;

         out << "stale value at " << voidP(&value) << endl;
         stale_vals.push_back(&value);
         stale_dobs.push_back(dob);

         if (!goon)
            {
              out << "Value::print_stale() : endless loop in "
                     "Value::all_values; stopping display." << endl;
            }
       }

   // then print more info...
   //
   loop(s, stale_vals.size())
      {
        const DynamicObject * dob = stale_dobs[s];
        const cValue * val = stale_vals[s];
        val->print_stale_info(out, dob);
        if (++count > 20)   // its getting boring
           {
             CERR << endl << " ... ( " << (stale_vals.size() - count) 
                  << " more stale values)..." << endl;
             break;
           }
       }

   // mark all dynamic values, and then unmark those known in the workspace
   //
   mark_all_dynamic_values();
   Workspace::unmark_all_values();
   Macro::unmark_all_macros();

   // print all values that are still marked
   //
   for (const DynamicObject * dob = all_values.get_prev();
        dob != &all_values; dob = dob->get_prev())
       {
         const Value & value = dob->rValue();

         // don't print values found in the previous round.
         //
         bool known_stale = false;
         loop(s, stale_vals.size())
            {
              if (&value == stale_vals[s])
                 {
                   known_stale = true;
                   break;
                 }
            }
         if (known_stale)   continue;

         if (value.is_marked())
            {
              ++count;
              if (count < 20)   value.print_stale_info(out, dob);
              else if (count == 20)
              CERR << endl << " ... (more stale values)..." << endl;
              value.unmark();
            }
       }

   return count;
}
//────────────────────────────────────────────────────────────────────────────
int
Value::check_all_Cells(ostream & out)
{
int errors = 0;

   for (const DynamicObject * dob = all_values.get_prev();
        dob != &all_values; dob = dob->get_prev())
       {
         const Value & value = dob->rValue();
         if (value.check_Cells(out))   ++errors;
       }

   return errors;
}
//────────────────────────────────────────────────────────────────────────────
Value_P
Value::glue_item_item(Value & item_A, Value & item_B, const char * loc)
{
   // glue two items together, starting a strand
   //
   Log(LOG_glue)
      {
        CERR << "gluing two items " << endl << item_A
             << " and " << endl << item_B << endl;
      }

Value_P Z(2, LOC);
   Z->next_ravel_Value(&item_A);
   Z->next_ravel_Value(&item_B);

   Z->check_value(LOC);
   return Z;
}
//────────────────────────────────────────────────────────────────────────────
Value_P
Value::glue_item_strand(Value & item_A, const Value & strand_B,
                        const char * loc)
{
   // glue a new item A to the strand B
   //
const ShapeItem len_A = item_A.element_count();
const ShapeItem len_B = strand_B.element_count();
   Log(LOG_glue)
      {
        CERR << "gluing item[" << len_A << "] " << endl << item_A
             << " to strand[" << len_B << "] " << endl << strand_B << endl;
      }

   Assert(strand_B.is_scalar_or_vector());

Value_P Z(len_B + 1, LOC);
   Z->next_ravel_Value(&item_A);

   loop(b, len_B)   Z->next_ravel_Cell(strand_B.get_cravel(b));

   Z->check_value(LOC);
   return Z;
}
//────────────────────────────────────────────────────────────────────────────
Value_P
Value::glue_strand_item(const Value & strand_A, Value & item_B,
                        const char * loc)
{
   // glue a strand A to new item B
   //
   Log(LOG_glue)
      {
        CERR << "gluing strand " << endl << strand_A
             << " to item " << endl << item_B << endl;
      }

   Assert(strand_A.is_scalar_or_vector());

const ShapeItem len_A = strand_A.element_count();
Value_P Z(len_A + 1, LOC);

   loop(a, len_A)   Z->next_ravel_Cell(strand_A.get_cravel(a));
   Z->next_ravel_Value(&item_B);

   Z->check_value(LOC);
   return Z;
}
//────────────────────────────────────────────────────────────────────────────
Value_P
Value::glue_strand_strand(const Value & strand_A, const Value & strand_B,
                          const char * loc)
{
   // glue two strands A and B together
   //
const ShapeItem len_A = strand_A.element_count();
const ShapeItem len_B = strand_B.element_count();

   Log(LOG_glue)
      {
        CERR << "gluing strands " << endl << strand_A
             << "with shape " << strand_A.get_shape() << endl
             << " and " << endl << strand_B << endl
             << "with shape " << strand_B.get_shape() << endl;
      }

   Assert(strand_A.is_scalar_or_vector());
   Assert(strand_B.is_scalar_or_vector());

Value_P Z(len_A + len_B, LOC);

   loop(a, len_A)   Z->next_ravel_Cell(strand_A.get_cravel(a));
   loop(b, len_B)   Z->next_ravel_Cell(strand_B.get_cravel(b));

   Z->check_value(LOC);
   return Z;
}
//────────────────────────────────────────────────────────────────────────────
bool
Value::check_WS_FULL(const char * args, ShapeItem requested_cell_count,
                     const char * loc)
{
const int64_t used_memory
               = (total_ravel_count + requested_cell_count) * sizeof(Cell)
               + (value_count + 1) * sizeof(Value)
               + Workspace::SI_entry_count() * sizeof(StateIndicator);

   if (int64_t((Quad_WA::total_memory*Quad_WA::WA_scale/100)) >
       (used_memory + Quad_WA::WA_margin))   return false;   // OK

   Log(LOG_Value_alloc) CERR
   << "    value_count:       " << value_count             << endl
   << "    total_ravel_count: " << total_ravel_count       << " cells" << endl
   << "    new cell_count:    " << requested_cell_count    << " cells" << endl
   << "    total_memory:      " << Quad_WA::total_memory   << " bytes" << endl
   << "    used_memory:       " << used_memory             << " bytes" << endl
   << "    ⎕WA margin:        " << Quad_WA::WA_margin      << " bytes" << endl
   << "    ⎕WA scale:         " << Quad_WA::WA_scale       << "%" << endl

           << " at " << LOC << endl;
   return true;
}
//────────────────────────────────────────────────────────────────────────────
void
Value::catch_Error(const Error & error, const char * args, const char * loc)
{
   Log(LOG_Value_alloc)   CERR << "Ravel allocation failed" << endl;
   MORE_ERROR() << "new Value(" << args
                << ") failed (APL error in ravel allocation)";
   throw error;   // rethrow
}
//────────────────────────────────────────────────────────────────────────────
void
Value::catch_exception(const exception & ex, const char * args,
                      const char * caller,  const char * loc)
{
const int64_t used_memory
               = total_ravel_count * sizeof(Cell)
               + value_count * sizeof(Value)
               + Workspace::SI_entry_count() * sizeof(StateIndicator);

   Log(LOG_Value_alloc)
      CERR << "Value_P::Value_P(" << args << ") failed at " << loc
           << " (caller: "        << caller << ")" << endl
           << " what: "           << ex.what() << endl
           << " initial sbrk(): 0x" << hex << Quad_WA::initial_sbrk << endl
           << " current sbrk(): 0x" << top_of_memory() << endl
           << " alloc_size:     0x" << alloc_size << dec << " ("
                                    << alloc_size << ")" << endl
           << " used memory:    0x" << hex  << used_memory << dec
                                    << " (" << used_memory << ")" << endl;

   MORE_ERROR() << "new Value(" << args << ") failed (" << ex.what() << ")";
   WS_FULL;
}
//────────────────────────────────────────────────────────────────────────────
void
Value::catch_ANY(const char * args, const char * caller, const char * loc)
{
   Log(LOG_Value_alloc)
      CERR << "Value_P::Value_P(Shape " << args << " failed at " << loc
           << " (caller: " << caller << ")" << endl;
   MORE_ERROR() << "new Value(" << args << ") failed (ANY)";
   WS_FULL;
}
//────────────────────────────────────────────────────────────────────────────
void
Value::init_ravel()
{
   owner_count = 0;
   fetcher = &cell_fetcher;
   pointer_cell_count = 0;
   nz_subcell_count = 0;
   check_ptr = 0;
   IntCell::z0(short_value);
   ravel = short_value;

   ++value_count;
   if (Quad_SYL::value_count_limit &&
       Quad_SYL::value_count_limit < APL_Integer(value_count))
      {
        MORE_ERROR() <<
"the system limit on the APL value count (as set in ⎕SYL["
<< (Quad_SYL::SYL_VALUE_COUNT_LIMIT + Workspace::get_IO())
<< ";2]) was reached\n"
"(and to avoid lock-up, this system limit in ⎕SYL was automatically cleared).";

        // reset the limit so that we don't get stuck here.
        //
        Quad_SYL::value_count_limit = 0;
        InterruptContext::set_attention_raised(LOC);
        InterruptContext::set_interrupt_raised(LOC);
      }

const ShapeItem length = shape.get_volume();

   // small values always succeed...
   //
   if (length <= cfg_SHORT_VALUE_LENGTH_WANTED)
      {
        check_ptr = charP(this) + 7;
        return;
      }

   // large Value. If the compiler uses 4-byte pointers, then do not allow APL
   // values that are too large. The reason is that new[] may return
   // a non-0 pointer (thus pretending everything is OK), but a subsequent
   // attempt to initialize the value might then throw a segfault.
   //
   enum { MAX_LEN = 2000000000 / sizeof(Cell) };
   if (length > MAX_LEN && sizeof(void *) < 6)
      throw_apl_error(E_WS_FULL, alloc_loc);

   if (Quad_SYL::ravel_count_limit &&
       Quad_SYL::ravel_count_limit < APL_Integer(total_ravel_count + length))
      {
CERR << "*** Quad_SYL::ravel_count_limit hit ***" << endl;
        // make sure that the value is properly initialized
        //
        new (&shape) Shape();
        ravel = short_value;
        IntCell::zI(ravel, 42);

        MORE_ERROR() <<
"the system limit on the total ravel size (as set in ⎕SYL["
<< (Quad_SYL::SYL_RAVEL_BYTES_LIMIT + Workspace::get_IO())
<< ";2]) was reached\n"
"(and to avoid lock-up, this system limit in ⎕SYL was automatically cleared).";
        // reset the limit so that we don't get stuck here.
        //
        Quad_SYL::ravel_count_limit = 0;
        InterruptContext::set_attention_raised(LOC);
        InterruptContext::set_interrupt_raised(LOC);
      }

   alloc_size = length * sizeof(Cell);
   ravel = std::allocator<Cell>{}.allocate(length);

/*
   ravel = 0;   // assume new() fails
// try
//    {
        ravel = reinterpret_cast<Cell *>(new char[length * sizeof(Cell)]);
        if (ravel == 0)   // new failed (without trowing an exception)
           {
              Log(LOG_Value_alloc)
                 CERR << "new char[" << (length * sizeof(Cell))
                      << "] (aka. long ravel allocation) returned 0 at " LOC
                      << endl;

              MORE_ERROR() << "The instatiation of a Value object succeeded, "
                              "but allocation of its (large) ravel failed.";
              new (&shape) Shape();
              ravel = short_value;
              throw_apl_error(E_WS_FULL, alloc_loc);
           }
      }
   catch (...)
      {
        // for some unknown reason, this object gets cleared after
        // throwing the E_WS_FULL below (which destroys the prev and
        // next pointers. We therefore unlink() the object here (where
        // prev and next pointers are still intact).
        //
        unlink();

        Log(LOG_Value_alloc)
           {
             CERR << "new char[" << (length * sizeof(Cell))
                  << "] (aka. long ravel allocation) threw an exception at " LOC
                  << endl;
           }

        MORE_ERROR() << "The instatiation of a Value object succeeded, "
                        "but allocation of its (large) ravel failed.";
        new (&shape) Shape();
        ravel = short_value;
        throw_apl_error(E_WS_FULL, alloc_loc);
      }
*/

   // init the first ravel element to (prototype) 0 so that we can avoid
   // many empty checks all over the place
   //
   IntCell::z0(&get_wproto());
   check_ptr = charP(this) + 7;
   total_ravel_count += length;
}
//════════════════════════════════════════════════════════════════════════════
ostream &
operator <<(ostream & out, const Value & v)
{
   v.print(out);
   return out;
}
//════════════════════════════════════════════════════════════════════════════
Value_P
IntScalar(APL_Integer val, const char * loc)
{
Value_P Z(loc);
   Z->next_ravel_Int(val);
   Z->check_value(LOC);
   return Z;
}
//════════════════════════════════════════════════════════════════════════════
Value_P
FloatScalar(APL_Float val, const char * loc)
{
Value_P Z(loc);
   Z->next_ravel_Float(val);
   Z->check_value(LOC);
   return Z;
}
//════════════════════════════════════════════════════════════════════════════
Value_P
ComplexScalar(APL_Complex val, const char * loc)
{
Value_P Z(loc);
   Z->next_ravel_Complex(val);
   Z->check_value(loc);
   return Z;
}
//════════════════════════════════════════════════════════════════════════════
Value_P
ComplexScalar(APL_Float real, APL_Float imag, const char * loc)
{
Value_P Z(loc);
   Z->next_ravel_Complex(real, imag);
   Z->check_value(loc);
   return Z;
}
//════════════════════════════════════════════════════════════════════════════
Value_P
CharScalar(Unicode uni, const char * loc)
{
Value_P Z(loc);
   Z->next_ravel_Char(uni);
   Z->check_value(loc);
   return Z;
}
//════════════════════════════════════════════════════════════════════════════
Value_P
Idx0(const char * loc)
{
Value_P Z(ShapeItem(0), loc);
   // default proto is 0, so no need to set it here
   Z->check_value(loc);
   return Z;
}
//════════════════════════════════════════════════════════════════════════════
Value_P
Str0(const char * loc)
{
Value_P Z(ShapeItem(0), loc);
   Z->set_proto_Spc();
   Z->check_value(LOC);
   return Z;
}
//════════════════════════════════════════════════════════════════════════════
Value_P
Str0_0(const char * loc)
{
Shape sh(ShapeItem(0), ShapeItem(0));
Value_P Z(sh, loc);
   Z->set_proto_Spc();
   Z->check_value(loc);
   return Z;
}
//════════════════════════════════════════════════════════════════════════════
Value_P
Idx0_0(const char * loc)
{
Shape sh(ShapeItem(0), ShapeItem(0));
Value_P Z(sh, loc);
   Z->check_value(loc);
   return Z;
}
//════════════════════════════════════════════════════════════════════════════
Value_P
EmptyStruct(const char * loc)
{
   enum {
          rows = 8,
          cols = 2,
          items = rows * cols
        };

const Shape shape_Z(rows, cols);
Value_P Z(shape_Z, loc);
   loop(z, items)  Z->next_ravel_0();
   Z->check_value(LOC);
   Z->set_member();
   return Z;
}
//════════════════════════════════════════════════════════════════════════════
/// declared in PrintOperator.hh
ostream &
operator << (ostream & out, const AP_num3 & ap3)
{
   return out << ap3.proc << "." << ap3.parent << "." << ap3.grand;
}
//════════════════════════════════════════════════════════════════════════════

