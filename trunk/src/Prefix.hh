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

#ifndef __PREFIX_HH_DEFINED__
#define __PREFIX_HH_DEFINED__

#include "Common.hh"
#include "PrintOperator.hh"
#include "Token.hh"

class DerivedFunction;
class Prefix;
struct ReduceArg;
class StateIndicator;
class Token_string;

/// the max. number of tokens in one reduction
enum { MAX_REDUCTION_LEN = 4 };   // == MAX_PHRASE_LEN in Prefix.def

//────────────────────────────────────────────────────────────────────────────
/// how to continue after return from a reduce_XXX() function
enum R_action
{
   /** repeat phrase matching with the current stack. Returned  after the
       current stack was modified and phrase matching should be repeated
       without shifting a new token into the stack of \b this parser.

       This is the "normal" case after a phrase has reduced the stack by means
       of some reduce_XXX() function.
    **/
   RA_CONTINUE = 0,

   /** shift the next token into the stack and repeat phrase matching with
       the current stack.

        This is the "normal" action when no matching phrase was found for
        the current stack.
    **/
   RA_PUSH_NEXT = 1,   // aka. SHIFT

   /// return from \b parser to the calling parser with result \b arg[0]
   RA_RETURN = 2,

   /// suspend \b this parser and continue in the parser of the pushed SI
   /// entry (until the pushed parser returns with RA_RETURN).
   RA_SI_PUSHED = 3,

   /// internal error: action was not set by some reduce_XXX() function
   RA_FIXME  = 4,
};
//════════════════════════════════════════════════════════════════════════════
/// a class for reducing all statements of an Executable
class Prefix
{
   friend class XML_Loading_Archive;   // to )LOAD a parser from an .xml file
   friend class XML_Saving_Archive;    // to )SAVE a parser into an .xml file

public:
   /// constructor
   /// @param _si the StateIndicator entry that owns this parser
   /// @param body the token sequence to be parsed
   Prefix(StateIndicator & _si, const Token_string & body);

   /// destructor
   void clean_up();

   /// max. number of lookahead tokens
   enum { MAX_CONTENT   = 10*MAX_REDUCTION_LEN,
          MAX_CONTENT_1 = MAX_CONTENT - 1 };

   /// return true if ufun is on the stack
   /// @param ufun user-defined function to search for on the parser stack
   bool uses_function(const UserFunction * ufun) const;

   /// return true if the content of \b this Prefix has ⎕R (and maybe ⎕L or ⎕X).
   bool has_quad_LRX() const;

   /// print the state of this parser
   /// @param out output stream to print to
   /// @param indent indentation level for formatting
   void print(ostream & out, int indent) const;

   /// throw an E_LEFT_SYNTAX_ERROR or an E_SYNTAX_ERROR
   /// @param loc caller location for diagnostics
   void syntax_error(const char * loc);

   /// release any DerivedFunction (e.g. from f⍨, f¨, f⍤y, f⍣N) still
   /// sitting in the FIFO, so a statement being abandoned by an error
   /// does not leak its DerivedFunctionCache slot (and whatever Value_P
   /// it owns, e.g. POWER's literal N). Shared by syntax_error() and any
   /// other raw throw_apl_error() site in this file that abandons the
   /// statement without going through syntax_error() itself.
   void destroy_derived_in_FIFO();

   /// clear the mark flag of all values in \b this Prefix
   void unmark_all_values() const;

   /// print all owners of \b value
   /// @param prfx string prefix printed before each owner line
   /// @param out output stream to print to
   /// @param value the APL value whose owners are to be listed
   int show_owners(const char * prfx, ostream & out, const Value & value) const;

   /// highest PC in current statement
   Function_PC get_range_high() const
      { return put ? content[put- 1].get_PC() : get_lookahead_PC(); }

   /// lowest PC in current statement
   Function_PC get_range_low() const
      { return put ? content[0].get_PC() : get_lookahead_PC(); }

   /// print the current range (PC from - PC to) on out
   /// @param out output stream to print to
   void print_range(ostream & out) const;

   /// return the left argument of a failed primitive function (if any)
   /// @param function output: name of the function whose left arg is returned
   Value_P * locate_L(UCS_string & function) const;

   /// return the right argument of a failed primitive function (if any)
   /// @param function output: name of the function whose right arg is returned
   Value_P * locate_R(UCS_string & function) const;

   /// return the axis argument of a failed primitive function (if any)
   /// @param function output: name of the function whose axis arg is returned
   Value_P * locate_X(UCS_string & function) const;

   /// return the current monadic function (if any)
   const Function * get_dyadic_fun() const
      { return at1().get_ValueType() == TV_FUN ? at1().get_function() : 0; }

   /// return the current dyadic function (if any)
   const Function * get_monadic_fun() const
      { return at0().get_ValueType() == TV_FUN ? at0().get_function() : 0; }

   /// execute one context (user defined function or operator, execute,
   /// or immediate execution)
   Token reduce_body();

   /// return the number of tokens currently in the FIFO
   int ssize() const
      { return put; }

   /// return the leftmost token (in APL order, e.g. A in A←B or in A+B)
   const Token & at0() const
      { Assert1(put);   return content[put - 1].get_token(); }

   /// return the leftmost token (in APL order, e.g. A in A←B or in A+B)
   Token & at0()
      { Assert1(put);   return content[put - 1].get_token(); }

   /// return the second token from the left (e.g. ← in A←B)
   const Token & at1() const
      { Assert1(put >= 2);   return content[put - 2].get_token(); }

   /// return the second token from the left (e.g. ← in A←B)
   Token & at1()
      { Assert1(put >= 2);   return content[put - 2].get_token(); }

   /// return the third token from the left (e.g. B in A←B)
   Token & at2()
      { Assert1(put >= 3);   return content[put - 3].get_token(); }

   /// return the fourth token from the left (e.g. B in A+/B)
   Token & at3()
      { Assert1(put >= 4);   return content[put - 4].get_token(); }

   /// return true if the next token binds stronger than the best match
   /// @param next token class of the upcoming lookahead token
   bool do_shift(TokenClass next) const;

   /// return the current assignment state
   Assign_state get_assign_state() const
      { return assign_state; }

   /// set the current assignment state
   /// @param new_state the new assignment state to set
   void set_assign_state(Assign_state new_state)
      { assign_state = new_state; }

   /* return the highest PC seen in the current statement. This includes the
      lookahead token, therefore it could be (a little) > get_range_high().
      More importantly, it can be Function_PC_invalid.
    */
   Function_PC get_lookahead_PC() const
      { return saved_MISC.get_PC(); }

   /// set the PC of the lookahead token
   /// @param pc program counter value to assign to the lookahead token
   void set_lookahead_PC(Function_PC pc)
      { saved_MISC.set_PC(pc); }

   /// reset statement to empty state (e.g. after →N)
   /// @param loc caller location for diagnostics
   void reset(const char * loc);

   /// return the current PC
   Function_PC get_PC() const
      { return PC; }

   /// print the current stack to \b out
   /// @param out output stream to print to
   /// @param loc caller location for diagnostics
   void print_stack(ostream & out, const char * loc) const;

   /// print the current stack before or after shift/reduce to \b out
   /// @param out output stream to print to
   /// @param which 0 for before shift/reduce, 1 for after
   ostream & print_patterns(ostream & out, int which);

   /// print a token and its value in a brief form.
   /// @param out output stream to print to
   /// @param tok the token whose tag and value are printed
   ostream & print_token_value(ostream & out, const Token & tok);

   /// return the leftmost (top-of-stack) Token_loc (at put position)
   Token_loc & tos()
       { Assert1(put);   return content[put - 1]; }

   /// store one more token. push(A) in e.g. F B would produce A F B.
   /// IOW, push() works right-to-left of XXX in reduce_XXX().
   /// @param tl token with location to push onto the parser stack
   void push(const Token_loc & tl)
      {
        if (put >= MAX_CONTENT_1)   LIMIT_ERROR_PREFIX;
        content[put++].copy(tl, LOC);
      }

   /// if \b tok is an error token (from some eval_XXX() function) then
   /// push its Token_loc and return \b true. Otherwise return \b false.
   /// @param tok token to inspect; pushed only if it carries TOK_ERROR
   bool push_error(const Token & tok)
      {
        if (tok.get_tag() != TOK_ERROR)   return false;   // no error pushed
        const Token_loc tl(tok, get_range_low());
        push(tl);
        set_action(RA_RETURN);   // return from context;
        return true;             // error pushed
      }

   /// pop \b prefix_len items
   void pop_args()
        {
          Assert1(put >= prefix_len);
          put -= prefix_len;
        }

   /// pop \b prefix_len items and push \b result.
   /* For example:

      Before:   A + B    prefix_len=3, put→A,
                └─PC─┐
      After:         Z   prefix_len=1, put→Z, PC(Z_ = PC(A)
    */
   /// @param result token produced by the reduction that replaces the phrase
   void pop_args_push_result(const Token & result)
        {
          Assert1(put >= prefix_len);
          const int ZB = put - prefix_len;             // positions of B and Z
          content[ZB].get_token().copy(result, LOC);   // replace B with Z

          // release any APL value(s) held by the discarded tokens above
          // ZB (e.g. A and F in "A F B") -- put is about to drop below
          // them, and clean_up()/reset() (bounded by the *current* put)
          // will then never see them again. Usually harmless (the same
          // value normally stays reachable some other way too), but if
          // one of them was the sole reference to a value (e.g. a bare
          // temporary computed earlier in the same statement) and the
          // statement is then abandoned by an exception before this
          // Prefix's storage is reused for a later reduction, that
          // value leaks -- unreachable, yet never released (Bugs28 #12).
          //
          for (int s = ZB + 1; s < put; ++s)
              {
                Token & discarded = content[s].get_token();
                if (discarded.get_Class() == TC_VALUE)
                   discarded.release_apl_val(LOC);
              }

          content[ZB].set_PC(at(0).get_PC());          // PC(Z) = PC(A)
          put -= prefix_len - 1;                       // discard A and +
        }

   /// remove the leftmost token (e,g, A in A←B) from the stack and return it.
   /// In e.g. reduce_A_F_B_(), the first pop() would remove A.
   /// IOW, pop() works left-to-right of XXX in reduce_XXX().
   Token_loc & pop()
      {  Assert1(put);   return content[--put]; }

   /// discard the leftmost token (e,g, A in A←B) from the stack
   void pop_and_discard()
      {  Assert1(put);   --put; }

   /// return TOK_LSYMB2 tokens ahead (excluding the leftmost one
   /// already read into \b content). The result \b symbols is empty
   /// for selective specifications and non-empty for vector specifications.
   /// @param symbols output: vector filled with collected symbol pointers
   void collect_symbols(vector<Symbol *> & symbols);

   /// clear the saved_MISC token
   /// @param loc caller location for diagnostics
   void clear_MISC(const char * loc)
      {
        saved_MISC.set_PC(Function_PC_invalid);
        saved_MISC.get_token().clear(loc);
      }

   /// lookahead is a complete index. return \b true if it belongs to a value
   /// (as opposed to a function axis).
   bool value_expected() const;

   /// jump to new PC
   /// @param new_pc program counter value to jump to
   void goto_PC(Function_PC new_pc)
      { reset(LOC);   PC = new_pc; }

   /// adjust the right caret after a SYNTAX_ERROR
   /// @param range program counter range for the failed statement; updated in place
   /// @param failed_statement the token sequence that caused the syntax error
   static void adjust_right_caret(Function_PC2 & range,
                                  const Token_string & failed_statement);

   /// the signature of the current eval_XXX() function (0 if none).
   static Fun_signature get_current_signature();

protected:
   /// construct a new derived function and return a pointer to it.
   /// @param LO token for the left operand (or 0 if absent)
   /// @param F_or_M_or_D the function, monadic, or dyadic operator
   /// @param RO token for the right operand (or 0 if absent)
   /// @param X optional axis value (empty Value_P if absent)
   /// @param loc caller location for diagnostics
   DerivedFunction * get_fun_oper_slot(Token * LO, cFunction_P F_or_M_or_D,
                                       Token * RO, Value_P X,
                                       const char * loc) const;

   /// set the prefix parser action
   inline void set_action(R_action ra)
      {
        action = ra;
      }

   /// if end is \b TOK_IF_ELSe then jump over the ELSE clause
   /// @param maybe_else token that may be a TOK_IF_ELSE marker
   /// @param num number of tokens to skip if the ELSE branch is taken
   inline void handle_ELSE(const Token & maybe_else, int num);

   /// set the prefix parser action according to (result-) Token type.
   // Called (typically after some eval_XXX()) if the return class can not
   // be predicted token classes.
   /// @param result the token returned by an eval_XXX() function
   void set_action(const Token & result);

   /// read and resolve the token class left of [ ... ], PC is at ']'
   bool is_value_bracket() const;

   /// return true if \b this prefix has a valid lookahead token
   int has_MISC() const
      { return saved_MISC.get_token().get_tag() == TOK_VOID ? 0 : 1; }

   /// read and resolve the token class left of )
   /// @param pc program counter of the closing parenthesis token
   bool is_value_parenthesis(int pc) const;

   /// return the idx'th Token_loc (from put position)
   /// @param idx zero-based index from the left (APL order)
   Token_loc & at(int idx)
       { Assert1(idx < put);   return content[put - idx - 1]; }

   /// return the TokenClass of \b at(idx)
   /// @param idx zero-based index from the left (APL order)
   const TokenClass Class_at(int idx) const
       { Assert1(idx < put);
         return content[put - idx - 1].get_token().get_Class(); }

   /// return the idx'th Token_loc (from put position)
   /// @param idx zero-based index from the left (APL order)
   const Token_loc & at(int idx) const
       { Assert1(idx < put);   return content[put - idx - 1]; }

   /// one phrase in the phrase table
   struct Phrase
      {
        const char *   phrase_name;     ///< phrase name
        const char *   reduce_name;     ///< reduce function name
        void (Prefix::*reduce_fun)();   ///< reduce function
        unsigned int   phrase_hash;     ///< phrase hash
        int            prio;            ///< phrase priority
        int            misc;            ///< 1 if MISC phrase
        int            phrase_len;      ///< phrase length
        int            can_shift;       ///< 1 iff a SHIFT is even possible
                                         ///< for this phrase: prio <
                                         ///< BS_ANY_BRA (else nothing
                                         ///< binds tighter) AND the
                                         ///< leading token class (A/B,
                                         ///< F/G, or V) is one do_shift()
                                         ///< does not unconditionally
                                         ///< reject. 0 means REDUCE is
                                         ///< the only possible outcome,
                                         ///< so bind_to_next() need not
                                         ///< be called at all.
        int            phrase_number;   ///< index (0..PHRASE_COUNT-1) of
                                         ///< this phrase in phrase_gen's
                                         ///< phrase_table[], i.e. the number
                                         ///< shown in the PHRASE TABLE
                                         ///< comment at the top of Prefix.def
      };

   /// push the next token onto the stack. Return \b true iff )SI was pushed.
   //  called often but from the same place in reduce_body()
   inline bool push_next_token();

   /// find a phrase that matches the current stack. Return \b true iff found.
   //  called often but from the same place in reduce_body()
   inline void find_best_phrase();

   /// return true iff the next token binds stronger (so we need to shift).
   inline bool bind_to_next();

   /// check if ^C or attention was raised (and throw if so)
   /// @param end_of_line true if the parser has reached the end of a statement
   void check_interrupt_or_attention(bool end_of_line);

   /// perform a branch within a function.
   /// @param end_of_line true if the parser has reached the end of a statement
   void branch_within_function(bool end_of_line)
      {
        check_interrupt_or_attention(end_of_line);
        set_action(RA_PUSH_NEXT);
      }

   /// construct the )MORE info for a SYNTAX_ERROR.
   //  called rarely, so not inlined
   void push_END_error();

   /// push a symbol token (of class TC_SYMBOL). Return true iff )SI was pushed.
   /// @param tl token with location for the symbol to push
   inline bool push_Symbol(Token_loc & tl);

   /// Bugs28 #55: push_Symbol() calls this instead of pushing \b symbol
   /// (the first/rightmost name of a value-member chain "A.B.C...symbol")
   /// as a bare TC_SYMBOL, whenever something with strong-enough binding
   /// priority (currently: a pending bracket index) is already on the
   /// stack and would otherwise reduce the bare symbol on its own before
   /// the chain's '.' ever gets a chance to combine it into a proper
   /// member reference. Resolves the whole chain right here (mirroring
   /// Prefix::reduce_D_V__()'s member-reference branch, just triggered
   /// earlier) and pushes a plain value or an lvalue-capable cellref
   /// array in its place.
   /// @param tl token with location for the symbol that would have been
   ///        pushed
   /// @param symbol the same symbol, already extracted from tl
   void push_member_chain(Token_loc & tl, Symbol * symbol);

   /// return true if the left (back-)slash in M M means F M.
   /// @param PC program counter of the left slash/backslash token
   inline bool MM_is_FM(Function_PC PC);

   /// a unique identifier
   const uint64_t instance;

   // member declarations of all 'void reduce_XXX()' functions...
   //
#define P_(_name, _suffix, _idx, _prio, _misc, _len)
#include "Prefix.def"

   /// ⎕ES result helper
   static void handle_QUAD_ES_COM(const Token & result);

   /// ⎕ES result helper
   static void handle_QUAD_ES_ESC();

   /// ⎕ES result helper
   static void handle_QUAD_ES_BRA(const Token & result);

   /// ⎕ES result helper
   static void handle_QUAD_ES_ERR(const Token & result);

   /// ⎕EA helper shared by handle_QUAD_ES_ERR() and handle_QUAD_ES_BRA():
   /// execute statement_A (⎕EA's left/fallback argument) and place its
   /// value at the top of the caller's pending call site. Also records
   /// why B failed onto ⎕ET/⎕EM/)MORE (unconditionally, before A runs)
   /// so that A -- or whoever later queries ⎕ET/⎕EM -- can see it; if A
   /// itself subsequently also fails, A's own (nearer) error naturally
   /// takes precedence over this one, with no extra logic needed here.
   /// See Bugs27 #36.
   /// @param statement_A source text of ⎕EA's left argument
   /// @param ec_on_failure why B failed
   /// @param why short, human-readable reason for )MORE (e.g. "B failed
   ///        to execute")
   /// @param b_line2 B's own failed-statement text (⎕EM[2;]), if
   ///        available (from ⎕EC's own error-message construction,
   ///        forwarded through the ⎕EA macro) -- set on the
   ///        constructed Error alongside b_lcaret/b_rcaret instead of
   ///        leaving it (and therefore ⎕EM[2;]/[3;]) blank. 0 when
   ///        unavailable (e.g. the →N out-of-range fallback, which was
   ///        never a caught runtime error to begin with).
   /// @param b_lcaret / @param b_rcaret B's own caret column positions
   ///        (⎕EM[3;]), meaningful only when b_line2 is non-null.
   ///        -1/-1 for no carets (matching Error::set_left_caret()'s
   ///        own "-1 means none" convention).
   /// See Bugs28 #56 (Bugs27 #36 residual).
   static void execute_EA_fallback(UCS_string statement_A,
                                   ErrorCode ec_on_failure,
                                   const char * why,
                                   const UCS_string * b_line2 = 0,
                                   int b_lcaret = -1, int b_rcaret = -1);

   /// the StateIndicator that contains this parser
   StateIndicator & si;

   /// put pointer (for the next token at PC). Since content is a stack,
   /// its put position is also its size.
   int put;

   /** the lookahead tokens (tokens that were shifted but not yet reduced).
       \b content is in body order, that is, content[0] is the oldest 
       (= rightmost in APL order) token and content[put - 1] is the latest.
    **/
   Token_loc content[MAX_CONTENT];

   /// the X token (leftmost token in MISC phrase, if any)
   Token_loc saved_MISC;

   /// the function body. \b this parser parses tokens body[pc_from] ...
   const Token_string & body;

   /// the current pc (+1)
   Function_PC PC;

   /// assignment state
   Assign_state assign_state;

   /// length of matched prefix (before calling a reduce_XXX() function)
   int prefix_len;

   /// the action to be taken after returning from a reduce_XXX() function
   R_action action;

   /// the best (longest) phrase that matches the current stack,
   /// or 0 if no phrase matches (so the parser will SHIFT)
   const Phrase * best_phrase;

   /// a generator for unique identifiers
   static uint64_t instance_counter;
};
//════════════════════════════════════════════════════════════════════════════

#endif // __PREFIX_HH_DEFINED__
