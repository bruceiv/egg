#pragma once

/*
 * Copyright (c) 2014 Aaron Moss
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <algorithm>
#include <cstdint>
#include <list>
#include <string>
#include <memory>
#include <unordered_map>
#include <utility>

/**
 * Implements derivative parsing for parsing expression grammars, according to the algorithm 
 * described by Aaron Moss in 2014 (currently unpublished - contact the author for a manuscript).
 * 
 * The basic idea of this derivative parsing algorithm is to repeatedly take the "derivative" of a 
 * parsing expression with respect to the next character in the input sequence, where the 
 * derivative is a parsing expression which matches the suffixes of all strings in the language of 
 * the original expression which start with the given prefix.
 */
namespace derivs {
	template <typename T>
	using ptr = std::shared_ptr<T>;
	
	/// Type of expression node
	enum expr_type {
		fail_type,
		inf_type,
		eps_type,
		look_type,
		char_type,
		range_type,
		any_type,
		str_type,
		rule_type,
		not_type,
		and_type,
		seq_type,
		alt_type,
		e_back_type,
		l_back_type,
		el_back_type
	}; // enum expr_type
	
	/// Nullability mode
	enum nbl_mode {
		NBL,    ///< Matches on all strings
		EMPTY,  ///< Matches on empty string, but not all strings
		SHFT    ///< Does not match on empty string
	}; // enum nbl_mode
	
	/// Lookahead mode
	enum lk_mode {
		LOOK,  ///< Lookahead expression
		PART,  ///< Partial lookahead expression
		READ   ///< Non-lookahead expression
	}; // enum lk_mode
	
	/// Generation of an expression
//	struct gen_type {
//		/// Constructs a gen_type with matching bounds
//		gen_type(uint16_t x = 0) : min(x), max(x) {}
//		
//		/// Constructs a gen_type with the given minimum and maximum
//		gen_type(uint16_t min, uint16_t max) : min(min), max(max) {}
//		
//		/// Constructs a gen_type with the union of the two bounds
//		gen_type(const gen_type& a, const gen_type& b) {
//			min = std::min(a.min, b.min);
//			max = std::max(a.max, b.max);
//		}
//		
//		/// Unions the given bounds with this object
//		gen_type& operator |= (const gen_type& o) {
//			min = std::min(min, o.min);
//			max = std::max(max, o.max);
//			return *this;
//		}
//		
//		uint16_t min;  ///< Minimum subexpression generation
//		uint16_t max;  ///< Maximum subexpression generation
//	};

	/// Type of generation value
	using gen_type = uint8_t;
	
	/// Type of generation flags
	using gen_flags = uint64_t;
	
	
	/// Abstract base class for parsing expressions
	class expr {
	protected:
		expr() = default;
		
		/// Makes an expression pointer given the constructor arguments for some subclass T (less 
		/// the memo-table)
		template <typename T, typename... Args>
		static inline ptr<expr> as_ptr(Args... args) {
			return std::static_ptr_cast<expr>(std::make_shared<T>(args...));
		}	
	
	public:			
		/// Derivative of this expression with respect to x
		virtual ptr<expr> d(char x) const = 0;
		
		/// Is this expression nullable?
		virtual nbl_mode nbl() const = 0;
		
		/// Is this a lookahead expression?
		virtual lk_mode lk() const = 0;
		
		/// Maximum lookahead generation of this expression
		virtual gen_type gen() const = 0;
		
		/// Expression node type
		virtual expr_type type() const = 0;
	}; // class expr
	
	/// Abstract base class for memoized parsing expressions
	class memo_expr : public expr {
	public:
		/// Memoization table type
		using table = std::unordered_map<expr*, ptr<expr>>;
	
	protected:
		static const uint8_t NBL_VAL      = 0x1;  ///< value for NBL
		static const uint8_t EMPTY_VAL    = 0x3;  ///< value for EMPTY
		static const uint8_t SHFT_VAL     = 0x2;  ///< value for SHFT
		
		static const uint8_t LOOK_VAL     = 0x1;  ///< value for LOOK
		static const uint8_t PART_VAL     = 0x3;  ///< value for PART
		static const uint8_t READ_VAL     = 0x2;  ///< value for READ
		
		/// Constructor providing memoization table reference
		expr(memo_table& memo) : memo(memo), g(0) { flags = {0,0,false}; }
		
		/// Actual derivative calculation
		virtual ptr<expr> deriv(char) const = 0;
		
		/// Actual computation of nullability mode
		virtual nbl_mode nullable() const = 0;
		
		/// Actual computation of lookahead mode
		virtual lk_mode lookahead() const = 0;
		
		/// Actual computation of maximum generation
		virtual gen_type generation() const = 0;
		
	public:
		ptr<expr> d(char x) const final {
			ptr<expr>& dx = memo[this];
			if ( ! dx ) dx = deriv(x);
			return dx;
		}
		
		virtual nbl_mode nbl() const {
			switch ( flags.nbl ) {
			case SHFT_VAL:  return SHFT;
			case NBL_VAL:   return NBL;
			case EMPTY_VAL: return EMPTY;
			
			default:                         // no nullability set yet
				nbl_mode mode = nullable();  // calculate nullability
				switch ( mode ) {            // set nullability
				case SHFT:  flags.nbl = SHFT_VAL;  break;
				case NBL:   flags.nbl = NBL_VAL;   break;
				case EMPTY: flags.nbl = EMPTY_VAL; break;
				}
				return mode;
			}
		}
		
		virtual lk_mode lk() const {
			switch ( flags.lk ) {
			case READ_VAL: return READ;
			case LOOK_VAL: return LOOK;
			case PART_VAL: return PART;
			
			default:                         // no lookahead set yet
				lk_mode mode = lookahead();  // calculate lookahead
				switch ( mode ) {            // set nullability
				case READ: flags.lk = READ_VAL; break;
				case LOOK: flags.lk = LOOK_VAL; break;
				case PART: flags.lk = PART_VAL; break;
				}
				return mode;
			}
		}
		
		virtual gen_type gen() const {
			// Set generation value if unset
			if ( ! flags.gen ) {
				g = generation(); 
				flags.gen = true;
			}
			
			return g;
		}
	
	protected:
		mutable table&   memo;     ///< Memoization table for derivatives
		mutable gen_type g;        ///< Maximum generation value
		mutable struct {
			uint8_t nbl : 2;  ///< Nullable value: one of 0 (unset), 1 (NBL), 2(SHFT), 3(EMPTY)
			uint8_t lk  : 2;  ///< Lookahead value: one of 0 (unset), 1(LOOK), 2(READ), 3(PART)
			bool    gen : 1;  ///< Generation set?
		} flags;                   ///< Memoization status flags
	}; // class memo_expr
	
	class fail_expr;
	class inf_expr;
	class eps_expr;
	class look_expr;
	class char_expr;
	class range_expr;
	class any_expr;
	class str_expr;
	class rule_expr;
	class not_expr;
	class and_expr;
	class seq_expr;
	class alt_expr;
	class e_back_expr;
	class l_back_expr;
	class el_back_expr;
	
	/// A failure parsing expression
	class fail_expr : public expr {
		fail_expr() = default;
	
	public:
		static ptr<expr> make() { return expr::as_ptr<fail_expr>(); }
		
		// A failure expression can't un-fail - no strings to match with any prefix
		virtual ptr<expr> d(char) const { return fail_expr::make(); }
		
		virtual nbl_mode  nbl()   const { return SHFT; }
		virtual lk_mode   lk()    const { return READ; }
		virtual gen_type  gen()   const { return gen_type(0); }
		virtual expr_type type()  const { return fail_type; }
	}; // class fail_expr
	
	/// An infinite loop failure parsing expression
	class inf_expr : public expr {
		inf_expr() = default;
		
	public:
		static ptr<expr> make() { return expr::as_ptr<inf_expr>(); }
		
		// An infinite loop expression never breaks, ill defined with any prefix
		virtual ptr<expr> d(char) const { return inf_expr::make(); }
		
		virtual nbl_mode  nbl()   const { return SHFT; }
		virtual lk_mode   lk()    const { return READ; }
		virtual gen_type  gen()   const { return gen_type(0); }
		virtual expr_type type()  const { return inf_type; }
	}; // class inf_expr
	
	/// An empty success parsing expression
	class eps_expr : public expr {
		eps_expr() = default;
	
	public:
		static ptr<expr> make() { return expr::as_ptr<eps_expr>(); }
		
		// No prefixes to remove from language containing the empty string; all fail
		virtual ptr<expr> d(char) const { return fail_expr::make(); }
		
		virtual nbl_mode  nbl()   const { return NBL; }
		virtual lk_mode   lk()    const { return READ; }
		virtual gen_type  gen()   const { return gen_type(0); }
		virtual expr_type type()  const { return eps_type; }
	}; // class eps_expr
	
	/// A lookahead success parsing expression
	class look_expr : public expr {
		look_expr(gen_type g = 1) : g(g) {}
		
	public:
		static ptr<expr> make(gen_type g = 1) {
			// gen-0 lookahead success is just an epsilon
			return ( g == 0 ) ? 
				expr::as_ptr<eps_expr>() :
				expr::as_ptr<look_expr>(g);
		}
		
		// Lookahead success is just a marker, so persists (character will be parsed by sequel)
		virtual ptr<expr> d(char) const { return expr_ptr<look_expr>(g); }
		
		virtual nbl_mode  nbl()   const { return NBL; }
		virtual lk_mode   lk()    const { return LOOK; }
		virtual gen_t     gen()   const { return gen_type(g); }
		virtual expr_type type()  const { return look_type; }
		
		gen_type g;  ///< Lookahead generation
	}; // class look_expr
	
	/// A single-character parsing expression
	class char_expr : public expr {
		char_expr(char c) : c(c) {}
		
	public:
		static ptr<expr> make(char c) { return expr::as_ptr<char_expr>(c); }
		
		// Single-character expression either consumes matching character or fails
		virtual ptr<expr> d(char x) const {
			return ( c == x ) ? eps_expr::make() : fail_expr::make();
		}
		
		virtual nbl_mode  nbl()  const { return SHFT; }
		virtual lk_mode   lk()   const { return READ; }
		virtual gen_type  gen()  const { return gen_type(0); }
		virtual expr_type type() const { return char_type; }
		
		char c; ///< Character represented by the expression
	}; // class char_expr
	
	/// A character range parsing expression
	class range_expr : public expr {
		range_expr(char b, char e) : b(b), e(e) {}
		
	public:
		static ptr<expr> make(char b, char e) { return expr::as_ptr<range_expr>(b, e); }
		
		// Character range expression either consumes matching character or fails
		virtual ptr<expr> d(char x) const {
			return ( b <= x && x <= e ) ? eps_expr::make() : fail_expr::make();
		}
		
		virtual nbl_mode  nbl()  const { return SHFT; }
		virtual lk_mode   lk()   const { return READ; }
		virtual gen_type  gen()  const { return gen_type(0); }
		virtual expr_type type() const { return range_type; }
		
		char b;  ///< First character in expression range 
		char e;  ///< Last character in expression range
	}; // class range_expr
	
	/// A parsing expression which matches any character
	class any_expr : public expr {
		any_expr() = default;
		
	public:
		static ptr<expr> make() { return expr::as_ptr<any_expr>(); }
		
		// Any-character expression consumes any character
		virtual ptr<expr> d(char) const { return eps_expr::make(); }
		
		virtual nbl_mode  nbl()   const { return SHFT; }
		virtual lk_mode   lk()    const { return READ; }
		virtual gen_type  gen()   const { return gen_type(0); }
		virtual expr_type type()  const { return any_type; }
	}; // class any_expr

#if 1	
	/// A parsing expression representing a character string
	class str_expr : public expr {
		/// Ref-counted cons-list representation of a string
		struct str_node {
			char c;              ///< Character in node
			ptr<str_node> next;  ///< Next node
			
			/// Constructs a new string node with a null next character
			str_node(char c) : c(c), next() {}
			
			/// Constructs a new string node with the given next character
			str_node(char c, ptr<str_node> next) : c(c), next(next) {}
			
			/// Constructs a new series of string nodes for the given string.
			/// Warning: string should have length of at least one
			str_node(const std::string& s) {
				// make node for end of string
				std::string::const_iterator it = s.rbegin();
				ptr<str_node> last = std::make_shared<str_node>(*it);
				
				// make nodes for internal characters
				++it;
				while ( it != s.rend() ) {
					last = std::make_shared<str_node>(*it, last);
					++it;
				}
				
				// setup this node
				c = last->c;
				next = last->next;
			}
			
			/// Gets the string of the given length out of a list of nodes
			std::string str(unsigned long len) const {
				const str_node* n = this;
				
				char a[len];
				for (unsigned long i = 0; i < len; ++i) {
					a[i] = n->c;
					n = n->next;
				}
				
				return std::string(a, len);
			}
		}; // struct str_node
		
		/// Constructs a string expression from a string node and a length
		str_expr(str_node s, unsigned long len) : s(s), len(len) {}
		/// Constructs a string expression from a standard string (should be non-empty)
		str_expr(std::string t) : s(t), len(t.size()) {}
		
	public:
		/// Makes an appropriate expression node for the length of the string
		static ptr<expr> make(std::string s) {
			switch ( t.size() ) {
			case 0:  return eps_expr::make();
			case 1:  return char_expr::make(s[0]);
			default: return expr::as_ptr<str_expr>(s);
			}
		}
		
		virtual ptr<expr> d(char x) const {
			// Check that the first character matches
			if ( s.c != x ) return fail_expr::make();
			
			// Otherwise return a character or string expression, as appropriate
			return ( len == 2 ) ? 
				char_expr::make(s.next.c) :
				expr::as_ptr<str_expr>(s.next, len - 1);
		}
		
		virtual nbl_mode  nbl()  const { return SHFT; }
		virtual lk_mode   lk()   const { return READ; }
		virtual gen_type  gen()  const { return gen_type(0); }
		virtual expr_type type() const { return str_type; }
		
		/// Gets the string represented by this node
		std::string str() const { return s.str(); }
		/// Gets the length of the string represented by this node
		unsigned long size() const { return len; }
		
	private:
		str_node s;         ///< Internal string representation
		unsigned long len;  ///< length of internal string
	}; // class str_expr
#else
	/// A parsing expression representing a character string
	class str_expr : public expr {
		str_expr(std::string s) : s(s) {}
		
	public:
		static ptr<expr> make(std::string s) {
			switch ( t.size() ) {
			case 0:  return eps_expr::make();
			case 1:  return char_expr::make(s[0]);
			default: return expr::as_ptr<str_expr>(s);
			}
		}
		
		virtual ptr<expr> d(char x) const {
			// Check that the first character matches
			if ( s[0] != x ) return fail_expr::make();
			
			// Otherwise return a character or string expression, as appropriate
			if ( s.size() == 2 ) return char_expr::make(s[1]);
			
			std::string t(s, 1);
			return expr::as_ptr<str_expr>(t);
		}
		
		virtual nbl_mode  nbl()  const { return SHFT; }
		virtual lk_mode   lk()   const { return READ; }
		virtual gen_type  gen()  const { return gen_type(0); }
		virtual expr_type type() const { return str_type; }
		
		std::string s;
	}; // class str_expr
#endif
	
	/// A parsing expression representing a non-terminal
	class rule_expr : public memo_expr {
		rule_expr(memo_expr::table& memo, ptr<expr> r) : memo_expr(memo), r(r) {}
		
	public:
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> r) {
			return expr::as_ptr<rule_expr>(memo, r);
		}
		
		virtual ptr<expr> deriv(char x) const {
			// signal infinite loop if we try to take this derivative again
			memo[this] = inf_expr::make();
			// calculate derivative
			return r->d(x);
		}
		
		virtual nbl_mode nullable() const {
			// Stop this from infinitely recursing
			flags.nbl = SHFT_VAL;
			// Calculate nullability
			return r.nbl();
		}
		
		virtual lk_mode lookahead() const {
			// Stop this from infinitely recursing
			flags.lk = READ_VAL;
			// Calculate lookahead
			return r.lk();
		}
		
		virtual gen_type generation() const {
			// Statically defined, can't have lookahead generation greater than 1
			return gen_type(lk() == READ ? 0 : 1);
		}
		
		virtual expr_type type() const { return rule_type; }
		
		ptr<expr> r;  ///< Expression corresponding to this rule
	}; // class rule_expr
	
	/// A parsing expression representing negative lookahead
	class not_expr : public memo_expr {
		friend and_expr;
		
		not_expr(memo_expr::table& memo, ptr<expr> e)
			: memo_expr(memo), e(e) { g = 1; flags.gen = true; }
		
	public:
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> e) {
			switch ( e->type() ) {
			// return match on subexpression failure
			case fail_type: return look_expr::make(1);
			// propegate infinite loop
			case inf_type:  return e; // an inf_expr
			// collapse nested lookaheads
			case not_type:  return expr::as_ptr<and_expr>(memo, e->e);
			case and_type:  return expr::as_ptr<not_expr>(memo, e->e);
			}
			// return failure on subexpression success
			if ( e->nbl() ) return fail_expr::make();
			
			return expr::as_ptr<not_expr>(memo, e);
		}
		
		// Take negative lookahead of subexpression derivative
		virtual ptr<expr> deriv(char x) const { return not_expr::make(memo, e->d(x)); }
		
		virtual nbl_mode nullable() const {
			// not-expression generally matches on empty string, but not all strings; it will only 
			// fail to match on the empty string if its subexpression does
			return ( e.nbl() == EMPTY ) ? SHFT : EMPTY;
		}
		
		virtual lk_mode   lk()   const { return LOOK; }
		virtual gen_type  gen()  const { return gen_type(1); }
		virtual expr_type type() const { return not_type; }
		
		ptr<expr> e;  ///< Subexpression to negatively match
	}; // class not_expr
	
	/// A parsing expression representing positive lookahead
	class and_expr : public memo_expr {
		friend not_expr;
		
		and_expr(memo_expr::table& memo, ptr<expr> e) 
			: memo_expr(memo), e(e) { g = 1; flags.gen = true; }
		
	public:
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> e) {
			switch ( e->type() ) {
			// return failure on subexpression failure
			case fail_type: return e; // a fail_expr
			// propegate infinite loop
			case inf_type:  return e; // an inf_expr
			// collapse nested lookaheads
			case not_type:  return e; // a not_expr wrapping e->e
			case and_type:  return e; // an and_expr wrapping e->e
			}
			// return success on subexpression success
			if ( e->nbl() ) return look_expr::make(1);
			
			return expr::as_ptr<and_expr>(memo, e);
		}
		
		// Take positive lookahead of subexpression derivative
		virtual ptr<expr> deriv(char x) const { return and_expr::make(memo, e->d(x)); }
		
		virtual nbl_mode nullable() const {
			// and-expression matches on the same set as its subexpression
			return e.nbl();
		}
		
		virtual lk_mode   lk()   const { return LOOK; }
		virtual gen_type  gen()  const { return gen_type(1); }
		virtual expr_type type() const { return and_type; }
		
		ptr<expr> e;  ///< Subexpression to match
	}; // class and_expr
	
	/// A parsing expression representing the concatenation of two parsing expressions
	class seq_expr : public memo_expr {
		seq_expr(memo_expr::table& memo, ptr<expr> a, ptr<expr> b)
			: memo_expr(memo), a(a), b(b) {}
	
	public:
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b) {
			switch ( a->type() ) {
			// empty first element leaves just second
			case eps_type:  return b;
			case look_type: return b;
			// failing or infinite loop first element propegates
			case fail_type: return a; // a fail_expr
			case inf_type:  return a; // an inf_expr
			}
			
			switch ( b->type() ) {
			// empty second element leaves just first
			case eps_type:  return a;
			// failing second element propegates
			case fail_type: return b; // a fail_expr
			}
			
			return expr::as_ptr<seq_expr>(memo, a, b);
		}
		
		virtual ptr<expr> deriv(char x) const {}
		
		virtual nbl_mode nullable() const {
			nbl_mode an = a->nbl(), bn = b->nbl();
			
			if ( an == SHFT || bn == SHFT ) return SHFT;
			else if ( an == NBL && bn == NBL ) return NBL;
			else return EMPTY;
		}
		
		virtual lk_mode lookahead() const {
			lk_mode al = a->lk(), bl = b->lk();
			
			if ( al == LOOK && bl == LOOK ) return LOOK;
			else if ( al == READ && bl == READ 
			          && a->nbl() == SHFT && b->nbl() == SHFT ) return READ;
			else return PART;
		}
		
		virtual gen_type generation() const { return gen_type(max(a.gen(), b.gen())); }
		
		virtual expr_type type() const { return seq_type; }
		
		ptr<expr> a;  ///< First subexpression
		ptr<expr> b;  ///< Second subexpression
	}; // class seq_expr
	
	/// A parsing expression representing the alternation of two parsing expressions
	class alt_expr : public memo_expr {
		expr(memo_expr::table& memo, ptr<expr> a, ptr<expr> b, gen_flags ag, gen_flags bg)
			: memo_expr(memo), a(a), b(b), ag(ag), bg(bg) {}
		
	public:
		/// Make an expression using the default generation rules
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b) {
			switch ( a->type() ) {
			// if first alternative fails, use second
			case fail_type: return b;
			// if first alternative is infinite loop, propegate
			case inf_type:  return a; // an inf_expr
			}
			// if first alternative is nullable, use first
			if ( a->nbl() ) return a;
			// if second alternative fails, use first
			if ( b->type() == fail_type ) return a;
			
			gen_flags ag, bg;
				
			// in both cases, 0 for READ, 1 for LOOK, 0 & 1 for PART
			switch ( a->lk() ) {
			case READ: flags::set(ag, 0);                    break;
			case LOOK: flags::set(ag, 1);                    break;
			case PART: flags::set(ag, 0); flags::set(ag, 1); break;
			}
			switch ( b->lk() ) {
			case READ: flags::set(bg, 0);                    break;
			case LOOK: flags::set(bg, 1);                    break;
			case PART: flags::set(bg, 0); flags::set(bg, 1); break;
			}
			
			return expr::as_ptr<alt_expr>(memo, a, b, ag, bg);
		}
		
		virtual ptr<expr> deriv(char x) const {
			ptr<expr> da = a->d(x);
			
			// Check conditions on a before we calculate dx(b) [same as make()]
			switch ( da->type() ) {
			case fail_type: return b->d(x);
			case inf_type:  return da; // an inf_expr
			}
			if ( da->nbl() ) return da;
			
			ptr<expr> db = b->d(x);
			if ( db->type() == fail_type ) return da;
			
			// Calculate generations of new subexpressions
			// - If we've added a lookahead generation that wasn't there before, map it into the 
			//   generation space of the derived alternation
			gen_flags dag = ag, dbg = bg;
			if ( da->gen() > a->gen() ) {
				flags::set(dag, gen()+1);
			}
			if ( db->gen() > b->gen() ) {
				flags::set(dbg, gen()+1);
			}
			
			return expr::as_ptr<alt_expr>(memo, da, db, dag, dbg);
		}
		
		virtual nbl_mode nullable() const {
			nbl_mode an = a->nbl(), bn = b->nbl();
			
			if ( an == NBL || bn == NBL ) return NBL;
			else if ( an == EMPTY || bn == EMPTY ) return EMPTY;
			else return SHFT;
		}
		
		virtual lk_mode lookahead() const {
			lk_mode al = a->lk(), bl = b->lk();
			
			if ( al == LOOK && bl == LOOK ) return LOOK;
			else if ( al == READ && bl == READ ) return READ;
			else return PART;
		}
		
		virtual gen_type generation() const {
			gen_flags tg;
			flags::set_union(ag, bg, tg);      // Union generation flags into tg
			return gen_type(flags::last(tg));  // Count set bits for maximum generation
		}
		
		virtual expr_type type() const { return alt_type; }
		
		ptr<expr> a;   ///< First subexpression
		ptr<expr> b;   ///< Second subexpression
		gen_flags ag;  ///< Generation flags for a
		gen_flags bg;  ///< Generation flags for b
	}; // class alt_expr
	
	/// A parsing expression to backtrack from a nullable first sequence element
	class e_back_expr : public memo_expr {
	public:
		static ptr<expr> make() {}
		
		virtual ptr<expr> deriv(char x) const {}
		
		virtual nbl_mode nullable() const {
			nbl_mode an = a->nbl();
			
			if ( an == EMPTY && b->nbl() != SHFT ) return EMPTY;
			else if ( an == SHFT && c->nbl() != SHFT ) return EMPTY;
			else return SHFT;
		}
		
		virtual lk_mode lookahead() const {
			lk_mode al = a->lk(), bl = b->lk(), cl = c->lk();
			
			if ( al == LOOK && bl == LOOK && cl == LOOK ) return LOOK;
			else if ( cl == READ && c->nbl() == SHFT
			          && ( ( al == READ && a->nbl() == SHFT ) 
			               || ( bl == READ && b->nbl() == SHFT ) ) ) return READ;
			else return PART;
		}
		
		virtual gen_type generation() const {
			gen_type g(a.gen(), b.gen());
			return x |= c.gen();
		}
		
		virtual expr_type type() const { return e_back_type; }
		
		ptr<expr> a;  ///< Test subexpression
		ptr<expr> b;  ///< Following subexpression
		ptr<expr> c;  ///< Parallel derivation of nullable branch
	}; // class e_back_expr
	
	/// A parsing expression to backtrack from a partial lookahead first sequence element
	class l_back_expr : public memo_expr {
	public:
		struct look_node {
			look_node(uint16_t g, ptr<expr> e) : g(g), e(e) {}
			
			uint16_t g;      ///< Generation of lookahead this derivation corresponds to
			ptr<expr> e;     ///< Following expression for this lookahead generation
		}; // struct look_list
		using look_list = std::list<look_node>;
		
		static ptr<expr> make() {}
		
		virtual ptr<expr> deriv(char x) const {}
		
		virtual nbl_mode nullable() const {}
		
		virtual lk_mode lookahead() const {}
		
		virtual gen_type generation() const {}
		
		virtual expr_type type() const { return l_back_type; }
		
		ptr<expr> a;   ///< Test subexpression
		look_list bs;  ///< List of following subexpressions for each lookahead generation
	}; // class l_back_expr
	
	/// A parsing expression to backtrack from a nullable and partial lookahead first sequence 
	/// element
	class el_back_expr : public l_back_expr {
	public:
		static ptr<expr> make() {}
		
		virtual ptr<expr> deriv(char x) const {}
		
		virtual nbl_mode nullable() const {}
		
		virtual lk_mode lookahead() const {}
		
		virtual gen_type generation() const {}
		
		virtual expr_type type() const { return el_back_type; }
		
		ptr<expr> c;  ///< Parallel derivation of nullable branch
	};
	
} // namespace derivs

