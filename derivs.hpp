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

#include "utils/flags.hpp"

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
	
	// Forward declarations of expression node types
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
	class map_expr;
	class alt_expr;
	class seq_expr;
	class back_expr;
	
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
		map_type,
		alt_type,
		seq_type,
		back_type
	}; // enum expr_type
	
	/// Nullability mode
	enum nbl_mode {
		NBL,      ///< Matches on all strings
		EMPTY,    ///< Matches on empty string, but not all strings
		SHFT      ///< Does not match on empty string
	}; // enum nbl_mode
	
	/// Lookahead mode
	enum lk_mode {
		LOOK,    ///< Lookahead expression
		PART,    ///< Partial lookahead expression
		READ     ///< Non-lookahead expression
	}; // enum lk_mode
	
	/// Type of generation value
	using gen_type = uint8_t;
	
	/// Type of generation flags
	using gen_flags = uint64_t;
	
	/// Abstract base class of all derivative visitors
	class visitor {
	public:
		virtual void visit(fail_expr&)  = 0;
		virtual void visit(inf_expr&)   = 0;
		virtual void visit(eps_expr&)   = 0;
		virtual void visit(look_expr&)  = 0;
		virtual void visit(char_expr&)  = 0;
		virtual void visit(range_expr&) = 0;
		virtual void visit(any_expr&)   = 0;
		virtual void visit(str_expr&)   = 0;
		virtual void visit(rule_expr&)  = 0;
		virtual void visit(not_expr&)   = 0;
		virtual void visit(and_expr&)   = 0;
		virtual void visit(map_expr&)   = 0;
		virtual void visit(alt_expr&)   = 0;
		virtual void visit(seq_expr&)   = 0;
		virtual void visit(back_expr&)  = 0;
	}; // class visitor
	
	/// Abstract base class for parsing expressions
	class expr {
	protected:
		expr() = default;
		
	public:
		template<typename T, typename... Args>
		static ptr<expr> make_ptr(Args&&... args) {
			return std::static_pointer_cast<expr>(std::make_shared<T>(args...));
		}
		
		/// Derivative of this expression with respect to x
		virtual ptr<expr> d(char x) const = 0;
		
		/// Accept visitor
		virtual void accept(visitor*) = 0;
		
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
		using table = std::unordered_map<memo_expr*, ptr<expr>>;
	
	protected:
		static const uint8_t  NBL_VAL   = 0x1;          ///< value for NBL
		static const uint8_t  EMPTY_VAL = 0x3;          ///< value for EMPTY
		static const uint8_t  SHFT_VAL  = 0x2;          ///< value for SHFT
		
		static const uint8_t  LOOK_VAL  = 0x1;          ///< value for LOOK
		static const uint8_t  PART_VAL  = 0x3;          ///< value for PART
		static const uint8_t  READ_VAL  = 0x2;          ///< value for READ
		
		/// Constructor providing memoization table reference
		memo_expr(table& memo) : memo(memo), g(0) { flags = {0,0,false}; }
		
		/// Actual derivative calculation
		virtual ptr<expr> deriv(char) const = 0;
		
		/// Actual computation of nullability mode
		virtual nbl_mode nullable() const = 0;
		
		/// Actual computation of lookahead mode
		virtual lk_mode lookahead() const = 0;
		
		/// Actual computation of maximum generation
		virtual gen_type generation() const = 0;
		
	public:
		/// Resets the memoziation fields of a memoized expression
		void reset_memo() { g = 0; flags = {0,0,false}; }
		
		virtual void accept(visitor*) = 0;
		
		virtual ptr<expr> d(char x) const {
			auto ix = memo.find(const_cast<memo_expr* const>(this));
			if ( ix == memo.end() ) {
				// no such item; compute and storequit
				ptr<expr> dx = deriv(x);
				memo.insert(ix, std::make_pair(const_cast<memo_expr* const>(this), dx));
				return dx;
			} else {
				// derivative found, return
				return ix->second;
			}
		}
		
		virtual nbl_mode nbl() const {
			switch ( flags.nbl ) {
			case SHFT_VAL:  return SHFT;
			case NBL_VAL:   return NBL;
			case EMPTY_VAL: return EMPTY;
			
			default:                         // no nullability set yet
				nbl_mode mode = nullable();  // calculate nullability
				switch ( mode ) {            // set nullability
				case SHFT:    flags.nbl = SHFT_VAL;  break;
				case NBL:     flags.nbl = NBL_VAL;   break;
				case EMPTY:   flags.nbl = EMPTY_VAL; break;
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
				case READ:   flags.lk = READ_VAL; break;
				case LOOK:   flags.lk = LOOK_VAL; break;
				case PART:   flags.lk = PART_VAL; break;
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
		table&           memo;  ///< Memoization table for derivatives
		mutable gen_type g;     ///< Maximum generation value
		mutable struct {
			uint8_t nbl : 2;  ///< Nullable value: one of 0 (unset), 1 (NBL), 2(SHFT), 3(EMPTY)
			uint8_t lk  : 2;  ///< Lookahead value: one of 0 (unset), 1(LOOK), 2(READ), 3(PART)
			bool    gen : 1;  ///< Generation set?
		} flags;                ///< Memoization status flags
	}; // class memo_expr
	
	/// A failure parsing expression
	class fail_expr : public expr {
	public:
		fail_expr() = default;
		
		static ptr<expr> make() { return expr::make_ptr<fail_expr>(); }
		
		void accept(visitor* v) { v->visit(*this); }
		
		// A failure expression can't un-fail - no strings to match with any prefix
		virtual ptr<expr> d(char) const { return fail_expr::make(); }
		
		virtual nbl_mode  nbl()   const { return SHFT; }
		virtual lk_mode   lk()    const { return READ; }
		virtual gen_type  gen()   const { return gen_type(0); }
		virtual expr_type type()  const { return fail_type; }
	}; // class fail_expr
	
	/// An infinite loop failure parsing expression
	class inf_expr : public expr {
	public:
		inf_expr() = default;
		
		static ptr<expr> make() { return expr::make_ptr<inf_expr>(); }
		
		void accept(visitor* v) { v->visit(*this); }
		
		// An infinite loop expression never breaks, ill defined with any prefix
		virtual ptr<expr> d(char) const { return inf_expr::make(); }
		
		virtual nbl_mode  nbl()   const { return SHFT; }
		virtual lk_mode   lk()    const { return READ; }
		virtual gen_type  gen()   const { return gen_type(0); }
		virtual expr_type type()  const { return inf_type; }
	}; // class inf_expr
	
	/// An empty success parsing expression
	class eps_expr : public expr {
	public:
		eps_expr() = default;
	
		static ptr<expr> make() { return expr::make_ptr<eps_expr>(); }
		
		void accept(visitor* v) { v->visit(*this); }
		
		// No prefixes to remove from language containing the empty string; all fail
		virtual ptr<expr> d(char) const { return fail_expr::make(); }
		
		virtual nbl_mode  nbl()   const { return NBL; }
		virtual lk_mode   lk()    const { return READ; }
		virtual gen_type  gen()   const { return gen_type(0); }
		virtual expr_type type()  const { return eps_type; }
	}; // class eps_expr
	
	/// A lookahead success parsing expression
	class look_expr : public expr {
	public:
		look_expr(gen_type g = 1) : g(g) {}
		
		static ptr<expr> make(gen_type g = 1) {
			// gen-0 lookahead success is just an epsilon
			return ( g == 0 ) ? 
				eps_expr::make() :
				expr::make_ptr<look_expr>(g);
		}
		
		void accept(visitor* v) { v->visit(*this); }
		
		// Lookahead success is just a marker, so persists (character will be parsed by sequel)
		virtual ptr<expr> d(char) const { return expr::make_ptr<look_expr>(g); }
		
		virtual nbl_mode  nbl()   const { return NBL; }
		virtual lk_mode   lk()    const { return LOOK; }
		virtual gen_type  gen()   const { return gen_type(g); }
		virtual expr_type type()  const { return look_type; }
		
		gen_type g;  ///< Lookahead generation
	}; // class look_expr
	
	/// A single-character parsing expression
	class char_expr : public expr {
	public:
		char_expr(char c) : c(c) {}
		
		static ptr<expr> make(char c) { return expr::make_ptr<char_expr>(c); }
		
		void accept(visitor* v) { v->visit(*this); }
		
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
	public:
		range_expr(char b, char e) : b(b), e(e) {}
		
		static ptr<expr> make(char b, char e) { return expr::make_ptr<range_expr>(b, e); }
		
		void accept(visitor* v) { v->visit(*this); }
		
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
	public:
		any_expr() = default;
		
		static ptr<expr> make() { return expr::make_ptr<any_expr>(); }
		
		void accept(visitor* v) { v->visit(*this); }
		
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
				auto it = s.rbegin();
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
					n = n->next.get();
				}
				
				return std::string(a, len);
			}
		}; // struct str_node
		
	public:
		/// Constructs a string expression from a string node and a length
		str_expr(str_node s, unsigned long len) : s(s), len(len) {}
		/// Constructs a string expression from a standard string (should be non-empty)
		str_expr(std::string t) : s(t), len(t.size()) {}
		
		/// Makes an appropriate expression node for the length of the string
		static ptr<expr> make(std::string s) {
			switch ( s.size() ) {
			case 0:  return eps_expr::make();
			case 1:  return char_expr::make(s[0]);
			default: return expr::make_ptr<str_expr>(s);
			}
		}
		
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char x) const {
			// Check that the first character matches
			if ( s.c != x ) return fail_expr::make();
			
			// Otherwise return a character or string expression, as appropriate
			return ( len == 2 ) ? 
				char_expr::make(s.next->c) :
				expr::make_ptr<str_expr>(*s.next, len - 1);
		}
		
		virtual nbl_mode  nbl()  const { return SHFT; }
		virtual lk_mode   lk()   const { return READ; }
		virtual gen_type  gen()  const { return gen_type(0); }
		virtual expr_type type() const { return str_type; }
		
		/// Gets the string represented by this node
		std::string str() const { return s.str(len); }
		/// Gets the length of the string represented by this node
		unsigned long size() const { return len; }
		
	private:
		str_node s;         ///< Internal string representation
		unsigned long len;  ///< length of internal string
	}; // class str_expr
#else
	/// A parsing expression representing a character string
	class str_expr : public expr {
	public:
		str_expr(std::string s) : s(s) {}
		
		static ptr<expr> make(std::string s) {
			switch ( t.size() ) {
			case 0:  return eps_expr::make();
			case 1:  return char_expr::make(s[0]);
			default: return expr::make_ptr<str_expr>(s);
			}
		}
		
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char x) const {
			// Check that the first character matches
			if ( s[0] != x ) return fail_expr::make();
			
			// Otherwise return a character or string expression, as appropriate
			if ( s.size() == 2 ) return char_expr::make(s[1]);
			
			std::string t(s, 1);
			return expr::make_ptr<str_expr>(t);
		}
		
		virtual nbl_mode  nbl()  const { return SHFT; }
		virtual lk_mode   lk()   const { return READ; }
		virtual gen_type  gen()  const { return gen_type(0); }
		virtual expr_type type() const { return str_type; }
		
		std::string str() const { return s; }
		unsigned long size() const { return s.size(); }
		
	private:
		std::string s;
	}; // class str_expr
#endif
	
	/// A parsing expression representing a non-terminal
	class rule_expr : public memo_expr {
	public:
		rule_expr(memo_expr::table& memo, ptr<expr> r = nullptr) : memo_expr(memo), r(r) {}
		
		static  ptr<expr> make(memo_expr::table& memo, ptr<expr> r = nullptr);
		void accept(visitor* v) { v->visit(*this); }
		virtual ptr<expr> deriv(char x) const;
		virtual nbl_mode  nullable() const;
		virtual lk_mode   lookahead() const;
		virtual gen_type  generation() const;
		virtual expr_type type() const { return rule_type; }
		
		ptr<expr> r;  ///< Expression corresponding to this rule
	}; // class rule_expr
	
	/// A parsing expression representing negative lookahead
	class not_expr : public memo_expr {
	public:
		not_expr(memo_expr::table& memo, ptr<expr> e)
			: memo_expr(memo), e(e) { g = 1; flags.gen = true; }
		
		static  ptr<expr> make(memo_expr::table& memo, ptr<expr> e);
		void accept(visitor* v) { v->visit(*this); }
		virtual ptr<expr> deriv(char x) const;
		virtual nbl_mode  nullable() const;
		virtual lk_mode   lk() const;
		virtual lk_mode   lookahead() const { return not_expr::lk(); }
		virtual gen_type  gen() const;
		virtual gen_type  generation() const { return not_expr::gen(); }
		virtual expr_type type() const { return not_type; }
		
		ptr<expr> e;  ///< Subexpression to negatively match
	}; // class not_expr
	
	/// A parsing expression representing positive lookahead
	class and_expr : public memo_expr {
	public:
		and_expr(memo_expr::table& memo, ptr<expr> e) 
			: memo_expr(memo), e(e) { g = 1; flags.gen = true; }
		
		static  ptr<expr> make(memo_expr::table& memo, ptr<expr> e);
		void accept(visitor* v) { v->visit(*this); }
		virtual ptr<expr> deriv(char x) const;
		virtual nbl_mode  nullable() const;
		virtual lk_mode   lk() const;
		virtual lk_mode   lookahead() const { return and_expr::lk(); }
		virtual gen_type  gen() const;
		virtual gen_type  generation() const { return and_expr::gen(); }
		virtual expr_type type() const { return and_type; }
		
		ptr<expr> e;  ///< Subexpression to match
	}; // class and_expr
	
	/// Maintains generation mapping from collapsed alternation expression.
	class map_expr : public memo_expr {
	public:
		static const gen_flags READ_FLAGS = gen_flags(0x8000000000000000);
		static const gen_flags LOOK_FLAGS = gen_flags(0x4000000000000000);
		static const gen_flags PART_FLAGS = gen_flags(0xC000000000000000);
		
		map_expr(memo_expr::table& memo, ptr<expr> e, gen_flags eg)
			: memo_expr(memo), e(e), eg(eg) {}
		
		static  ptr<expr> make(memo_expr::table& memo, ptr<expr> e, gen_flags eg);
		void accept(visitor* v) { v->visit(*this); }
		virtual ptr<expr> deriv(char x) const;
		virtual nbl_mode  nullable() const;
		virtual lk_mode   lookahead() const;
		virtual gen_type  generation() const;
		virtual expr_type type() const { return map_type; }
		
		ptr<expr> e;   ///< Subexpression
		gen_flags eg;  ///< Generation flags for subexpression
	}; // class map_expr
	
	/// A parsing expression representing the alternation of two parsing expressions
	class alt_expr : public memo_expr {
	public:
		alt_expr(memo_expr::table& memo, ptr<expr> a, ptr<expr> b)
			: memo_expr(memo), a(a), b(b) {
			flags::clear(ag);
			flags::clear(bg);
		}
		
		alt_expr(memo_expr::table& memo, ptr<expr> a, ptr<expr> b, gen_flags ag, gen_flags bg)
			: memo_expr(memo), a(a), b(b), ag(ag), bg(bg) {}
		
		/// Make an expression using the default generation rules
		static  ptr<expr> make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b);
		void accept(visitor* v) { v->visit(*this); }
		virtual ptr<expr> deriv(char x) const;
		virtual nbl_mode  nullable() const;
		virtual lk_mode   lookahead() const;
		virtual gen_type  generation() const;
		virtual expr_type type() const { return alt_type; }
		
		ptr<expr> a;   ///< First subexpression
		ptr<expr> b;   ///< Second subexpression
		gen_flags ag;  ///< Generation flags for a
		gen_flags bg;  ///< Generation flags for b
	}; // class alt_expr
	
	/// A parsing expression representing the concatenation of two parsing expressions
	class seq_expr : public memo_expr {
	public:
		seq_expr(memo_expr::table& memo, ptr<expr> a, ptr<expr> b)
			: memo_expr(memo), a(a), b(b) {}
	
		static  ptr<expr> make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b);
		void accept(visitor* v) { v->visit(*this); }
		virtual ptr<expr> deriv(char x) const;
		virtual nbl_mode  nullable() const;
		virtual lk_mode   lookahead() const;
		virtual gen_type  generation() const;
		virtual expr_type type() const { return seq_type; }
		
		ptr<expr> a;  ///< First subexpression
		ptr<expr> b;  ///< Second subexpression
	}; // class seq_expr
	
	/// A parsing expression to backtrack from a partial lookahead first sequence element
	class back_expr : public seq_expr {
	public:
		struct look_node {
			look_node(gen_type g, gen_flags eg, ptr<expr> e) : g(g), eg(eg), e(e) {}
			
			gen_type  g;   ///< Generation of lookahead this derivation corresponds to
			gen_flags eg;  ///< Map of generations from this node to the containing node
			ptr<expr> e;   ///< Following expression for this lookahead generation
		}; // struct look_list
		using look_list = std::list<look_node>;
	
		back_expr(memo_expr::table& memo, ptr<expr> a, ptr<expr> b, look_list bs) 
			: seq_expr(memo, a, b), bs(bs) {}
		
		/// Makes an expression given the memoization table, partial lookahead expression a, 
		/// following expression b, and lookahead-following expression b1
		static  ptr<expr> make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b, ptr<expr> b1);
		void accept(visitor* v) { v->visit(*this); }
		virtual ptr<expr> deriv(char x) const;
		virtual nbl_mode  nullable() const;
		virtual lk_mode   lookahead() const;
		virtual gen_type  generation() const;
		virtual expr_type type() const { return back_type; }
		
		look_list bs;  ///< List of following subexpressions for each lookahead generation
	}; // class back_expr
	
	////////////////////////////////////////////////////////////////////////////////
	//
	//  Implementations
	//
	////////////////////////////////////////////////////////////////////////////////
	
	// rule_expr ///////////////////////////////////////////////////////////////////
	
	ptr<expr> rule_expr::make(memo_expr::table& memo, ptr<expr> r) {
		return expr::make_ptr<rule_expr>(memo, r);
	}
	
	ptr<expr> rule_expr::deriv(char x) const {
		// signal infinite loop if we try to take this derivative again
		memo[const_cast<rule_expr* const>(this)] = inf_expr::make();
		// calculate derivative
		return r->d(x);
	}
	
	nbl_mode rule_expr::nullable() const {
		// Stop this from infinitely recursing
		flags.nbl = SHFT_VAL;
		// Calculate nullability
		return r->nbl();
	}
	
	lk_mode rule_expr::lookahead() const {
		// Stop this from infinitely recursing
		flags.lk = READ_VAL;
		// Calculate lookahead
		return r->lk();
	}
	
	gen_type rule_expr::generation() const {
		// Statically defined, can't have lookahead generation greater than 1
		return gen_type(lk() == READ ? 0 : 1);
	}
	
	// not_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> not_expr::make(memo_expr::table& memo, ptr<expr> e) {
		switch ( e->type() ) {
		// return match on subexpression failure
		case fail_type: return look_expr::make(1);
		// propegate infinite loop
		case inf_type:  return e; // an inf_expr
		// collapse nested lookaheads
		case not_type:
			return expr::make_ptr<and_expr>(memo, std::static_pointer_cast<not_expr>(e)->e);
		case and_type:
			return expr::make_ptr<not_expr>(memo, std::static_pointer_cast<and_expr>(e)->e);
		default:        break; // do nothing
		}
		// return failure on subexpression success
		if ( e->nbl() == NBL ) return fail_expr::make();
		
		return expr::make_ptr<not_expr>(memo, e);
	}
	
	// Take negative lookahead of subexpression derivative
	ptr<expr> not_expr::deriv(char x) const { return not_expr::make(memo, e->d(x)); }
	
	nbl_mode not_expr::nullable() const {
		// not-expression generally matches on empty string, but not all strings; it will only 
		// fail to match on the empty string if its subexpression does
		switch ( e->nbl() ) {
		case EMPTY:   return SHFT;
		default:      return EMPTY;
		}
	}
	
	lk_mode  not_expr::lk()  const { return LOOK; }
	gen_type not_expr::gen() const { return gen_type(1); }
	
	// and_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> and_expr::make(memo_expr::table& memo, ptr<expr> e) {
		switch ( e->type() ) {
		// return failure on subexpression failure
		case fail_type: return e; // a fail_expr
		// propegate infinite loop
		case inf_type:  return e; // an inf_expr
		// collapse nested lookaheads
		case not_type:  return e; // a not_expr wrapping e->e
		case and_type:  return e; // an and_expr wrapping e->e
		default:        break; // do nothing
		}
		// return success on subexpression success
		if ( e->nbl() == NBL ) return look_expr::make(1);
		
		return expr::make_ptr<and_expr>(memo, e);
	}
	
	// Take positive lookahead of subexpression derivative
	ptr<expr> and_expr::deriv(char x) const { return and_expr::make(memo, e->d(x)); }
	
	nbl_mode and_expr::nullable() const {
		// and-expression matches on the same set as its subexpression
		return e->nbl();
	}
	
	lk_mode  and_expr::lk()  const { return LOOK; }
	gen_type and_expr::gen() const { return gen_type(1); }
	
	// map_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> map_expr::make(memo_expr::table& memo, ptr<expr> e, gen_flags eg) {
		switch ( e->type() ) {
		case eps_type:  return e; // an eps_expr
		// a look expression with the generation translated
		case look_type: return look_expr::make(flags::select(eg, e->gen()));
		case fail_type: return e; // a fail_expr
		case inf_type:  return e; // an inf_expr
		default:        break; // do nothing
		}
		
		// check if map isn't needed
		switch ( eg ) {
		case READ_FLAGS:
			if ( e->lk() == READ ) return e;
			break;
		case LOOK_FLAGS:
			if ( e->lk() == LOOK ) return e;
			break;
		case PART_FLAGS:
			if ( e->lk() == PART ) return e;
			break;
		default: break;
		}
		
		return expr::make_ptr<map_expr>(memo, e, eg);
	}
	
	ptr<expr> map_expr::deriv(char x) const {
		ptr<expr> de = e->d(x);
		
		// Check conditions on de [same as make]
		switch ( de->type() ) {
		case eps_type:  return de; // an eps_expr
		// a look expression with the generation translated
		case look_type: return look_expr::make(flags::select(eg, de->gen()));
		case fail_type: return de; // a fail_expr
		case inf_type:  return de; // an inf_expr
		default:        break; // do nothing
		}
		
		return expr::make_ptr<map_expr>(memo, de, eg);
	}
	
	nbl_mode map_expr::nullable()   const { return e->nbl(); }
	lk_mode  map_expr::lookahead()  const { return e->lk(); }
	gen_type map_expr::generation() const {
		return gen_type(flags::last(eg));  // Last bit is highest generation present
	}
	
	// alt_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> alt_expr::make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b) {
		switch ( a->type() ) {
		// if first alternative fails, use second
		case fail_type: return b;
		// if first alternative is infinite loop, propegate
		case inf_type:  return a; // an inf_expr
		default:        break; // do nothing
		}
		// if first alternative is nullable, use first
		if ( a->nbl() == NBL ) return a;
		// if second alternative fails, use first
		if ( b->type() == fail_type ) return a;
		
		gen_flags ag, bg;
		flags::clear(ag); flags::clear(bg);
			
		// in both cases, 0 for READ, 1 for LOOK, 0 & 1 for PART
		switch ( a->lk() ) {
		case READ: ag = map_expr::READ_FLAGS; break;
		case LOOK: ag = map_expr::LOOK_FLAGS; break;
		case PART: ag = map_expr::PART_FLAGS; break;
		}
		switch ( b->lk() ) {
		case READ: bg = map_expr::READ_FLAGS; break;
		case LOOK: bg = map_expr::LOOK_FLAGS; break;
		case PART: bg = map_expr::PART_FLAGS; break;
		}
		
		return expr::make_ptr<alt_expr>(memo, a, b, ag, bg);
	}
	
	ptr<expr> alt_expr::deriv(char x) const {
		ptr<expr> da = a->d(x);
		
		// Check conditions on a before we calculate dx(b) [same as make()]
		switch ( da->type() ) {
		case fail_type: return map_expr::make(memo, b->d(x), bg);
		case inf_type:  return da; // an inf_expr
		default:        break; // do nothing
		}
		if ( da->nbl() == NBL ) return map_expr::make(memo, da, ag);
		
		ptr<expr> db = b->d(x);
		if ( db->type() == fail_type ) return map_expr::make(memo, da, ag);
		
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
		
		return expr::make_ptr<alt_expr>(memo, da, db, dag, dbg);
	}
	
	nbl_mode alt_expr::nullable() const {
		nbl_mode an = a->nbl(), bn = b->nbl();
		
		if ( an == NBL || bn == NBL ) return NBL;
		else if ( an == EMPTY || bn == EMPTY ) return EMPTY;
		else return SHFT;
	}
	
	lk_mode alt_expr::lookahead() const {
		lk_mode al = a->lk(), bl = b->lk();
		
		if ( al == LOOK && bl == LOOK ) return LOOK;
		else if ( al == READ && bl == READ ) return READ;
		else return PART;
	}
	
	gen_type alt_expr::generation() const {
		gen_flags tg;
		flags::clear(tg);
		flags::set_union(ag, bg, tg);      // Union generation flags into tg
		return gen_type(flags::last(tg));  // Count set bits for maximum generation
	}
	
	// seq_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> seq_expr::make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b) {
		switch ( b->type() ) {
		// empty second element leaves just first
		case eps_type:  return a;
		// failing second element propegates
		case fail_type: return b; // a fail_expr
		default:        break; // do nothing
		}
		
		switch ( a->type() ) {
		// empty first element leaves just second
		case eps_type:  return b;
		case look_type: return b;
		// failing or infinite loop first element propegates
		case fail_type: return a; // a fail_expr
		case inf_type:  return a; // an inf_expr
		default:        break; // do nothing
		}
		
		return expr::make_ptr<seq_expr>(memo, a, b);
	}
	
	ptr<expr> seq_expr::deriv(char x) const {
		ptr<expr> da = a->d(x);
		lk_mode al = a->lk();
		
		if ( al == LOOK ) {
			// Precludes nullability; only nullable lookahead expression is look_expr, and that 
			// one can't be `a` by the make rules. Therefore match both in sequence
			return seq_expr::make(memo, da, b->d(x));
		}
		
		if ( al == PART ) {
			ptr<expr> db = b->d(x);
			
			// Track the lookahead generations of this sequence
			ptr<expr> dx = back_expr::make(memo, da, b, db);
			
			if ( a->nbl() == NBL ) {
				// Set up backtracking for nullable first expression
				return alt_expr::make(memo, 
				                      dx, 
				                      seq_expr::make(memo, not_expr::make(memo, da), db));
			} else {
				return dx;
			}
		}
		
		// Derivative of this sequence
		ptr<expr> dx = seq_expr::make(memo, da, b);
		
		if ( a->nbl() == NBL ) {
			// Set up backtracking for nullable first expression
			return alt_expr::make(memo,
			                      dx,
			                      seq_expr::make(memo, not_expr::make(memo, da), b->d(x)));
		}
		
		return dx;
	}
	
	nbl_mode seq_expr::nullable() const {
		nbl_mode an = a->nbl(), bn = b->nbl();
		
		if ( an == SHFT || bn == SHFT ) return SHFT;
		else if ( an == NBL && bn == NBL ) return NBL;
		else return EMPTY;
	}
	
	lk_mode seq_expr::lookahead() const {
		lk_mode al = a->lk(), bl = b->lk();
		
		if ( al == LOOK && bl == LOOK ) return LOOK;
		else if ( (al == READ && a->nbl() == SHFT) 
		          || ( bl == READ && b->nbl() == SHFT ) ) return READ;
		else return PART;
	}
	
	gen_type seq_expr::generation() const {
		// If either a or b is READ and SHFT, the whole expression is, otherwise we take the 
		// generation of the second element
		if ( a->lk() == READ && a->nbl() == SHFT ) return gen_type(0);
		else return b->gen();
	}
	
	// back_expr ///////////////////////////////////////////////////////////////////
	
	ptr<expr> back_expr::make(memo_expr::table& memo, 
	                                 ptr<expr> a, ptr<expr> b, ptr<expr> b1) {
		switch ( a->type() ) {
		// empty first element leaves just follower
		case eps_type:  return b;
		// lookahead first element leaves just lookahead-follower
		case look_type: return b1;
		// failing or infinite loop first element propegates
		case fail_type: return a; // a fail_expr
		case inf_type:  return a; // an inf_expr
		default:        break; // do nothing
		}
		
		// If a is no longer a partial lookahead expression, form the appropriate sequence
		switch ( a->lk() ) {
		case READ: return seq_expr::make(memo, a, b);
		case LOOK: return seq_expr::make(memo, a, b1);
		default:   break; // do nothing
		}
		
		look_list bs;
		
		if ( b1->type() != fail_type ) {
			// If the lookahead-follower did not fail immediately, map in its generation flags
			gen_flags eg;
			flags::clear(eg);
			
			switch ( b1->lk() ) {
			case READ: flags::set(eg, 0);                    break;
			case LOOK: flags::set(eg, 1);                    break;
			case PART: flags::set(eg, 0); flags::set(eg, 1); break;
			}
			
			// Add the lookahead generation to the list
			bs.emplace_back(1, eg, b1);
		}
		
		return expr::make_ptr<back_expr>(memo, a, b, bs);
	}
	
	ptr<expr> back_expr::deriv(char x) const {
		ptr<expr> da  = a->d(x);
		
		switch ( da->type() ) {
		case eps_type: { 
			return b; // non-lookahead success leaves just follower
		} case look_type: { 
			// lookahead success leaves appropriate lookahead follower
			gen_type i = da->gen();
			for (const look_node& bi : bs) {
				if ( bi.g == i ) return map_expr::make(memo, bi.e, bi.eg);
				else if ( bi.g > i ) break;  // generation list is sorted
			}
			return fail_expr::make();  // if none found, fail
		} case fail_type: { 
			// failing or infinite loop element propegates
			return da; // a fail_expr
		} case inf_type: { 
			return da; // an inf_expr
		} default: break; // do nothing
		}
		
		lk_mode   dal = da->lk();
		
		// Fall back to regular sequence expression if first expression ceases to be lookahead
		if ( dal == READ ) return seq_expr::make(memo, da, b);
		
		// Free storage for sequential follower if first expression is all lookahead
		ptr<expr> bn = ( dal == LOOK ) ? fail_expr::make() : b;
		
		// Take derivative of all lookahead options
		look_list dbs;
		for (const look_node& bi : bs) {
			ptr<expr> dbi = bi.e->d(x);
			if ( dbi->type() != fail_type ) {
				// Map new lookahead generations into the space of the backtracking expression
				gen_flags dbig = bi.eg;
				if ( dbi->gen() > bi.e->gen() ) flags::set(dbig, gen()+1);
				dbs.emplace_back(bi.g, dbig, dbi);
			}
		}
		
		// Add any new lookahead generations to the list
		gen_type dag = da->gen();
		if ( dag > a->gen() ) {
			ptr<expr> db = b->d(x);
			if ( db->type() != fail_type ) {
				gen_flags bg;
				flags::clear(bg);
				
				switch ( db->lk() ) {
				case READ: flags::set(bg, 0);                    break;
				case LOOK: flags::set(bg, 1);                    break;
				case PART: flags::set(bg, 0); flags::set(bg, 1); break;
				}
				
				dbs.emplace_back(dag, bg, db);
			}
		}
		
		return expr::make_ptr<back_expr>(memo, da, bn, dbs);
	}
	
	nbl_mode back_expr::nullable() const {
		switch ( a->nbl() ) {
		case NBL:
			if ( b->nbl() == NBL ) return NBL;
			// fallthrough
		case EMPTY:
			if ( b->nbl() != SHFT ) return EMPTY;
			for (const look_node& bi : bs) if ( bi.e->nbl() != SHFT ) return EMPTY;
			// fallthrough
		case SHFT:
			return SHFT;
		}
	}
	
	lk_mode back_expr::lookahead() const {
		// Scan the b_i for followers that don't always consume characters - PART if so
		if ( b->nbl() != SHFT || b->lk() != READ ) return PART;
		for (const look_node& bi : bs) {
			if ( bi.e->nbl() != SHFT || bi.e->lk() != READ ) return PART;
		}
		// READ otherwise
		return READ;
	}
	
	gen_type back_expr::generation() const {
		gen_flags tg;
		flags::clear(tg);
		flags::set(tg, 0);
		
		// Take union of all generation flags into tg
		for (const look_node& bi : bs) { flags::set_union(tg, bi.eg, tg); }
		
		return gen_type(flags::last(tg));  // Count set bits for maximum generation
	}
	
} // namespace derivs

