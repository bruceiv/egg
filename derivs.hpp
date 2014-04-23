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

//uncomment to disable asserts
//#define NDEBUG
#include <cassert>

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "utils/uint_set.hpp"

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
	
	/// set of backtrack generations
	using gen_set  = utils::uint_set;
	/// single backtrack generation
	using gen_type = gen_set::value_type;
	
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
	class map_expr;
	class alt_expr;
	class seq_expr;
	
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
		map_type,
		alt_type,
		seq_type
	}; // enum expr_type
	
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
		virtual void visit(map_expr&)   = 0;
		virtual void visit(alt_expr&)   = 0;
		virtual void visit(seq_expr&)   = 0;
	}; // class visitor
	
	/// Abstract base class for parsing expressions
	class expr {
	protected:
		expr() = default;
		
		/// Creates a new backtrack map to map an expression into a generation space
		static gen_set new_back_map(ptr<expr> e, gen_type gm, bool& did_inc) {
			assert(!e->back().empty() && "backtrack set never empty");
			
			gen_set eg = zero_set;
			if ( e->back().max() > 0 ) {
				assert(e->back().max() == 1 && "static lookahead gen <= 1");
				did_inc = true;
				eg |= gm + 1;
			}
			
			return eg;
		}
		
		static inline gen_set new_back_map(ptr<expr> e, gen_type gm) {
			bool did_inc = true;
			return new_back_map(e, gm, did_inc);
		}
		
		/// Gets the default backtracking map for an expression 
		///   (zero_set if no lookahead gens, zero_one_set otherwise)
		static gen_set default_back_map(ptr<expr> e, bool& did_inc) {
			assert(!e->back().empty() && "backtrack set never empty");
			if ( e->back().max() > 0 ) {
				assert(e->back().max() == 1 && "static lookahead gen <= 1");
				did_inc = true;
				return zero_one_set;
			} else {
				return zero_set;
			}
		}
		
		static inline gen_set default_back_map(ptr<expr> e) {
			bool did_inc = true;
			return default_back_map(e, did_inc);
		}
		
		/// Gets an updated backtrack map
		/// @param e        The original expression
		/// @param de		The derivative of the expression to produce the new backtrack map for
		/// @param eg		The backtrack map for e
		/// @param gm		The current maximum generation
		/// @param did_inc	Set to true if this operation involved a new backtrack gen
		/// @return The backtrack map for de
		static gen_set update_back_map(ptr<expr> e, ptr<expr> de, gen_set eg, 
		                               gen_type gm, bool& did_inc) {
			assert(!e->back().empty() && !de->back().empty() && "backtrack set never empty");
			
			gen_set deg = eg;
			if ( de->back().max() > e->back().max() ) {
				assert(de->back().max() == e->back().max() + 1 && "gen only grows by 1");
				deg |= gm+1;
				did_inc = true;
			}
			
			return deg;
		}
		
		static inline gen_set update_back_map(ptr<expr> e, ptr<expr> de, gen_set eg, gen_type gm) {
			bool did_inc = true;
			return update_back_map(e, de, eg, gm, did_inc);
		}
		
	public:
		static const gen_set empty_set;
		static const gen_set zero_set;
		static const gen_set one_set;
		static const gen_set zero_one_set;
		
		template<typename T, typename... Args>
		static ptr<expr> make_ptr(Args&&... args) {
			return std::static_pointer_cast<expr>(std::make_shared<T>(args...));
		}
		
		/// Derivative of this expression with respect to x
		virtual ptr<expr> d(char x) const = 0;
		
		/// Accept visitor
		virtual void accept(visitor*) = 0;
		
		/// At what backtracking generations does this expression match?
		virtual gen_set match() const = 0;
		
		/// What backtracking generations does this expression expose?
		virtual gen_set back() const = 0;
				
		/// Expression node type
		virtual expr_type type() const = 0;
	}; // class expr
	
	const gen_set expr::empty_set{};
	const gen_set expr::zero_set{0};
	const gen_set expr::one_set{1};
	const gen_set expr::zero_one_set{0,1};
	
	/// Abstract base class for memoized parsing expressions
	class memo_expr : public expr {
	public:
		/// Memoization table type
		using table = std::unordered_map<memo_expr*, ptr<expr>>;
	
	protected:
		/// Constructor providing memoization table reference
		memo_expr(table& memo) : memo(memo) { flags = {false,false}; }
		
		/// Actual derivative calculation
		virtual ptr<expr> deriv(char) const = 0;
		
		/// Actual computation of match set
		virtual gen_set match_set() const = 0;
		
		/// Actual computation of backtrack set
		virtual gen_set back_set() const = 0;
				
	public:
		/// Resets the memoziation fields of a memoized expression
		void reset_memo() { flags = {false,false}; }
		
		virtual void accept(visitor*) = 0;
		
		virtual ptr<expr> d(char x) const {
			auto ix = memo.find(const_cast<memo_expr* const>(this));
			if ( ix == memo.end() ) {
				// no such item; compute and store
				ptr<expr> dx = deriv(x);
				memo.insert(ix, std::make_pair(const_cast<memo_expr* const>(this), dx));
				return dx;
			} else {
				// derivative found, return
				return ix->second;
			}
		}
		
		virtual gen_set match() const {
			if ( ! flags.match ) {
				memo_match = match_set();
				flags.match = true;
			}
			
			return memo_match;
		}
		
		virtual gen_set back() const {
			if ( ! flags.back ) {
				memo_back = back_set();
				flags.back = true;
			}
			
			return memo_back;
		}
	
	protected:
		table&          memo;        ///< Memoization table for derivatives
		mutable gen_set memo_match;  ///< Stored match set
		mutable gen_set memo_back;   ///< Stored backtracking set
		mutable struct {
			bool match : 1; ///< Is there a match set stored?
			bool back  : 1; ///< Is there a backtrack set stored?
		} flags;                     ///< Memoization status flags
	}; // class memo_expr
	
	/// A failure parsing expression
	class fail_expr : public expr {
	public:
		fail_expr() = default;
		
		static ptr<expr> make() { return expr::make_ptr<fail_expr>(); }
		
		void accept(visitor* v) { v->visit(*this); }
		
		// A failure expression can't un-fail - no strings to match with any prefix
		virtual ptr<expr> d(char) const { return fail_expr::make(); }
		
		virtual gen_set   match() const { return expr::empty_set; }
		virtual gen_set   back()  const { return expr::zero_set; }
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
		
		virtual gen_set   match() const { return expr::empty_set; }
		virtual gen_set   back()  const { return expr::zero_set; }
		virtual expr_type type()  const { return inf_type; }
	}; // class inf_expr
	
	/// An empty success parsing expression
	class eps_expr : public expr {
	public:
		eps_expr() = default;
	
		static ptr<expr> make() { return expr::make_ptr<eps_expr>(); }
		
		void accept(visitor* v) { v->visit(*this); }
		
		// No prefixes to remove from language containing the empty string; all fail
		virtual ptr<expr> d(char x) const {
			return ( x == '\0' ) ? eps_expr::make() : fail_expr::make();
		}
		
		virtual gen_set   match() const { return expr::zero_set; }
		virtual gen_set   back()  const { return expr::zero_set; }
		virtual expr_type type()  const { return eps_type; }
	}; // class eps_expr
	
	/// An lookahead success parsing expression
	class look_expr : public expr {
	public:
		look_expr(gen_type g = 1) : b{g} {}
	
		static ptr<expr> make(gen_type g = 1) {
			return (g == 0) ? expr::make_ptr<eps_expr>() : expr::make_ptr<look_expr>(g);
		}
		
		void accept(visitor* v) { v->visit(*this); }
		
		// No prefixes to remove from language containing the empty string; all fail
//		virtual ptr<expr> d(char x) const { return look_expr::make(b); }
		virtual ptr<expr> d(char x) const {  
			return ( x == '\0' ) ? look_expr::make(b) : fail_expr::make();
		}
		
		virtual gen_set   match() const { return gen_set{b}; }
		virtual gen_set   back()  const { return gen_set{b}; }
		virtual expr_type type()  const { return look_type; }
		
		gen_type b;  ///< Generation of this success match
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
		
		virtual gen_set   match() const { return expr::empty_set; }
		virtual gen_set   back()  const { return expr::zero_set; }
		virtual expr_type type()  const { return char_type; }
		
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
		
		virtual gen_set   match() const { return expr::empty_set; }
		virtual gen_set   back()  const { return expr::zero_set; }
		virtual expr_type type()  const { return range_type; }
		
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
		virtual ptr<expr> d(char x) const {
			return ( x == '\0' ) ? fail_expr::make() : eps_expr::make();
		}
		
		virtual gen_set   match() const { return expr::empty_set; }
		virtual gen_set   back()  const { return expr::zero_set; }
		virtual expr_type type()  const { return any_type; }
	}; // class any_expr

#if 0
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
			switch ( s.size() ) {
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
		
		virtual gen_set   match() const { return expr::empty_set; }
		virtual gen_set   back()  const { return expr::zero_set; }
		virtual expr_type type()  const { return str_type; }
		
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
		
		virtual ptr<expr> deriv(char) const;
		virtual gen_set   match_set() const;
		virtual gen_set   back_set()  const;
		virtual expr_type type()      const { return rule_type; }
		
		ptr<expr> r;  ///< Expression corresponding to this rule
	}; // class rule_expr
	
	/// A parsing expression representing negative lookahead
	class not_expr : public memo_expr {
	public:
		not_expr(memo_expr::table& memo, ptr<expr> e)
			: memo_expr(memo), e(e) { flags.match = flags.back = true; }
		
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> e);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char) const;
		virtual gen_set   match_set() const;
		virtual gen_set   back_set()  const;
		virtual expr_type type()      const { return not_type; }
		
		virtual gen_set match() const { return not_expr::match_set(); }
		virtual gen_set back()  const { return not_expr::back_set(); }
		
		ptr<expr> e;  ///< Subexpression to negatively match
	}; // class not_expr
	
	/// Maintains generation mapping from collapsed alternation expression.
	class map_expr : public memo_expr {
	public:
		map_expr(memo_expr::table& memo, ptr<expr> e, gen_type gm, gen_set eg)
			: memo_expr(memo), e(e), gm(gm), eg(eg) {}
		
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> e, gen_type mg, gen_set eg);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char) const;
		virtual gen_set   match_set() const;
		virtual gen_set   back_set()  const;
		virtual expr_type type()      const { return map_type; }
		
		ptr<expr> e;   ///< Subexpression
		gen_type  gm;  ///< Maximum generation from source expresssion
		gen_set   eg;  ///< Generation flags for subexpression
	}; // class map_expr
	
	/// A parsing expression representing the alternation of two parsing expressions
	class alt_expr : public memo_expr {
	public:
		alt_expr(memo_expr::table& memo, ptr<expr> a, ptr<expr> b, 
		         gen_set ag = expr::zero_set, gen_set bg = expr::zero_set)
			: memo_expr(memo), a(a), b(b), ag(ag), bg(bg) {}
		
		/// Make an expression using the default generation rules
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b);
		/// Make an expression with the given generation maps
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b, 
		                      gen_set ag, gen_set bg);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char) const;
		virtual gen_set   match_set() const;
		virtual gen_set   back_set()  const;
		virtual expr_type type()      const { return alt_type; }
		
		ptr<expr> a;   ///< First subexpression
		ptr<expr> b;   ///< Second subexpression
		gen_set   ag;  ///< Generation flags for a
		gen_set   bg;  ///< Generation flags for b
	}; // class alt_expr
	
	/// A parsing expression representing the concatenation of two parsing expressions
	class seq_expr : public memo_expr {
	public:
		struct look_node {
			look_node(gen_type g, gen_set eg, ptr<expr> e, gen_type gl = 0) 
				: g(g), eg(eg), e(e), gl(gl) {}
			
			gen_type  g;   ///< Backtrack generation this follower corresponds to
			gen_set   eg;  ///< Map of generations from this node to the containing node
			ptr<expr> e;   ///< Follower expression for this lookahead generation
			gen_type  gl;  ///< Generation of last match
		}; // struct look_list
		using look_list = std::list<look_node>;
		
		seq_expr(memo_expr::table& memo, ptr<expr> a, ptr<expr> b)
			: memo_expr(memo), a(a), b(b), bs(), c(fail_expr::make()), cg(expr::zero_set), gm(0) {}
		
		seq_expr(memo_expr::table& memo, ptr<expr> a, ptr<expr> b, look_list bs, 
		         ptr<expr> c, gen_set cg, gen_type gm)
			: memo_expr(memo), a(a), b(b), bs(bs), c(c), cg(cg), gm(gm) {}
	
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char) const;
		virtual gen_set  match_set() const;
		virtual gen_set  back_set()  const;
		virtual expr_type type()      const { return seq_type; }
		
		ptr<expr> a;   ///< First subexpression
		ptr<expr> b;   ///< Gen-zero follower
		look_list bs;  ///< List of following subexpressions for each backtrack generation
		ptr<expr> c;   ///< Matching backtrack value
		gen_set   cg;  ///< Backtrack map for c
		gen_type  gm;  ///< Maximum backtrack generation
	}; // class seq_expr
		
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
	
	gen_set rule_expr::match_set() const {
		// Stop this from infinitely recursing
		flags.match = true;
		memo_match = expr::empty_set;
		
		// Calculate match set
		return r->match();
	}
	
	gen_set rule_expr::back_set() const {
		// Stop this from infinitely recursing
		flags.back = true;
		memo_back = expr::zero_set;
		
		// Calculate backtrack set
		return r->back();
	}
	
	// not_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> not_expr::make(memo_expr::table& memo, ptr<expr> e) {
		switch ( e->type() ) {
		case fail_type: return look_expr::make(1);  // return match on subexpression failure
		case inf_type:  return e;                   // propegate infinite loop
		default:        break;
		}
		
		// return failure on subexpression success
		if ( ! e->match().empty() ) return fail_expr::make();
		
		return expr::make_ptr<not_expr>(memo, e);
	}
	
	// Take negative lookahead of subexpression derivative
	ptr<expr> not_expr::deriv(char x) const { return not_expr::make(memo, e->d(x)); }
	
	gen_set not_expr::match_set() const { return expr::empty_set; }
	
	gen_set not_expr::back_set() const { return expr::one_set; }
	
	// map_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> map_expr::make(memo_expr::table& memo, ptr<expr> e, gen_type gm, gen_set eg) {
		// account for unmapped generations
		assert(e->back().max() < eg.count() && "no unmapped generations");
//		auto n = eg.count();
//		for (auto i = e->back().max() + 1; i < n; ++i) eg |= ++gm;
		
		switch ( e->type() ) {
		// Map expression match generation into exit generation
		case eps_type:  return look_expr::make(eg(0));
		case look_type: return look_expr::make(eg(e->match().max()));
		// Propegate fail and infinity errors
		case fail_type: return e; // a fail_expr
		case inf_type:  return e; // an inf_expr
		default:        break; // do nothing
		}
		
		// check if map isn't needed (identity map)
		if ( gm + 1 == eg.count() ) return e;
		
		return expr::make_ptr<map_expr>(memo, e, gm, eg);
	}
	
	ptr<expr> map_expr::deriv(char x) const {
		ptr<expr> de = e->d(x);
		
		// Check conditions on de [same as make]
		switch ( de->type() ) {
		// Map expression match generation into exit generation
		case eps_type:  return look_expr::make(eg(0));
		case look_type: return look_expr::make(eg(de->match().max()));
		// Propegate fail and infinity errors
		case fail_type: return de; // a fail_expr
		case inf_type:  return de; // an inf_expr
		default:        break; // do nothing
		}
		
		// Calculate generations of new subexpressions
		// - If we've added a lookahead generation that wasn't there before, map it into the 
		//   generation space of the derived alternation
		bool did_inc = false;
		gen_set deg = expr::update_back_map(e, de, eg, gm, did_inc);
		return expr::make_ptr<map_expr>(memo, de, gm + did_inc, deg);
	}
	
	gen_set map_expr::match_set() const { return eg(e->match()); }
	
	gen_set map_expr::back_set() const { return eg(e->back()); }
	
	// alt_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> alt_expr::make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b) {
		switch ( a->type() ) {
		// if first alternative fails, use second
		case fail_type: return b;
		// if first alternative is infinite loop, propegate
		case inf_type:  return a; // an inf_expr
		default:        break; // do nothing
		}
		
		// if first alternative matches or second alternative fails, use first
		if ( b->type() == fail_type || ! a->match().empty() ) return a;
		
		return expr::make_ptr<alt_expr>(memo, a, b, 
		                                expr::default_back_map(a), expr::default_back_map(b));
	}
	
	ptr<expr> alt_expr::make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b, 
	                         gen_set ag, gen_set bg) {
		switch ( a->type() ) {
		// if first alternative fails, use second
		case fail_type: return map_expr::make(memo, b, std::max(ag.max(), bg.max()), bg);
		// if first alternative is infinite loop, propegate
		case inf_type:  return a; // an inf_expr
		default:        break; // do nothing
		}
		
		// if first alternative matches or second alternative fails, use first
		if ( b->type() == fail_type || ! a->match().empty() ) {
			return map_expr::make(memo, a, std::max(ag.max(), bg.max()), ag);
		}
		
		return expr::make_ptr<alt_expr>(memo, a, b, ag, bg);
	}
	
	ptr<expr> alt_expr::deriv(char x) const {
		gen_type gm = std::max(ag.max(), bg.max());
		bool did_inc = false;
		
		// Calculate derivative and map in new lookahead generations
		ptr<expr> da = a->d(x);
		
		// Check conditions on a before we calculate dx(b) [same as make()]
		switch ( da->type() ) {
		case fail_type: {
			ptr<expr> db = b->d(x);
			gen_set dbg = expr::update_back_map(b, db, bg, gm, did_inc);
			return map_expr::make(memo, db, gm + did_inc, dbg);
		}
		case inf_type:  return da; // an inf_expr
		default:        break; // do nothing
		}
		
		// Map in new lookahead generations for derivative
		gen_set dag = expr::update_back_map(a, da, ag, gm, did_inc);
		
		if ( ! da->match().empty() ) {
			return map_expr::make(memo, da, gm + did_inc, dag);
		}
		
		// Calculate other derivative and map in new lookahead generations
		ptr<expr> db = b->d(x);
		if ( db->type() == fail_type ) return map_expr::make(memo, da, gm + did_inc, dag);
		gen_set dbg = expr::update_back_map(b, db, bg, gm, did_inc);
				
		return expr::make_ptr<alt_expr>(memo, da, db, dag, dbg);
	}
	
	gen_set alt_expr::match_set() const { return ag(a->match()) | bg(b->match()); }
	
	gen_set alt_expr::back_set() const { return ag(a->back()) | bg(b->back()); }
	
	// seq_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> seq_expr::make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b) {
		switch ( b->type() ) {
		// empty second element just leaves first
		case eps_type:  return a;
		// failing second element propegates
		case fail_type: return b;
		default: break;  // do nothing
		}
		
		switch ( a->type() ) {
		// empty first element or lookahead success just leaves follower
		case eps_type:
		case look_type:
			return b;
		// failure or infinite loop propegates
		case fail_type:
		case inf_type:
			return a;
		default: break;  // do nothing
		}
		
		bool did_inc = false;
		
		// Set up match-fail follower
		ptr<expr> c;
		gen_set   cg;
		gen_set am = a->match();
		if ( ! am.empty() && am.min() == 0 ) {
			c = b;
			cg = expr::default_back_map(c, did_inc);
		} else {
			c = fail_expr::make();
			cg = expr::zero_set;
		}
		
		// set up lookahead follower
		look_list bs;
		assert(!a->back().empty() && "backtrack set is always non-empty");
		if ( a->back().max() > 0 ) {
			assert(a->back().max() == 1 && "static backtrack gen <= 1");
			
			gen_type gl = 0;
			gen_set bm = b->match();
			if ( ! bm.empty() && bm.min() == 0 ) {
				gl = 1;
				did_inc = true;
			}
			
			bs.emplace_back(1, expr::default_back_map(b, did_inc), b, gl);
		}
		
		// return constructed expression
		return expr::make_ptr<seq_expr>(memo, a, b, bs, c, cg, 0 + did_inc);
	}
	
	ptr<expr> seq_expr::deriv(char x) const {
		bool did_inc = false;
		ptr<expr> da = a->d(x);
		
		// Handle empty or failure results from predecessor derivative
		switch ( da->type() ) {
		case eps_type: {
			// Return follower, but with any lookahead gens mapped to their proper generation
			gen_set bg = expr::new_back_map(b, gm, did_inc);
			return map_expr::make(memo, b, gm + did_inc, bg);
		} case look_type: {
			// Lookahead follower (or lookahead follower match-fail)
			auto i = std::static_pointer_cast<look_expr>(da)->b;
			for (const look_node& bi : bs) {
				if ( bi.g < i ) continue;
				if ( bi.g > i ) break;  // generation list is sorted
				
				ptr<expr> dbi = bi.e->d(x);  // found element, take derivative
				
				if ( dbi->type() == fail_type ) {  // straight path fails ...
					if ( bi.gl > 0 ) {                // ... but matched in the past
						return look_expr::make(bi.gl);   // return appropriate lookahead
					} else {                          // ... and no previous match
						return dbi;                      // return a failure expression
					}
				}
				
				gen_set dbig = expr::update_back_map(bi.e, dbi, bi.eg, gm, did_inc);
				
				// if there is no failure backtrack (or this generation is it) 
				// we don't have to track it
				gen_set dbim = dbi->match();
				if ( bi.gl == 0 || (! dbim.empty() && dbim.min() == 0) ) {
					return map_expr::make(memo, dbi, gm + did_inc, dbig);
				}
				
				// Otherwise return alt-expr of this lookahead and its failure backtrack
				return alt_expr::make(memo,
				                      dbi, 
				                      seq_expr::make(memo, 
				                                     not_expr::make(memo, dbi), 
				                                     look_expr::make()), 
	                                  dbig, gen_set{0,bi.gl});
			}
			return fail_expr::make(); // if lookahead follower not found, fail
		} case fail_type: {
			// Return match-fail follower
			ptr<expr> dc = c->d(x);
			gen_set dcg = expr::update_back_map(c, dc, cg, gm, did_inc);
			return map_expr::make(memo, dc, gm + did_inc, dcg);
		} case inf_type: {
			// Propegate infinite loop
			return da;
		} default: {}  // do nothing
		}
		
		// Construct new match-fail follower
		ptr<expr> dc;
		gen_set dcg;
		
		gen_set dam = da->match();
		if ( ! dam.empty() && dam.min() == 0 ) {
			// new failure backtrack
			dc = b;
			dcg = expr::new_back_map(b, gm, did_inc);
		} else {
			// continue previous failure backtrack
			dc = c->d(x);
			dcg = expr::update_back_map(c, dc, cg, gm, did_inc);
		}
		
		// Build derivatives of lookahead backtracks
		look_list dbs;
		for (const look_node& bi : bs) {
			ptr<expr> dbi = bi.e->d(x);
			gen_set dbig = expr::update_back_map(bi.e, dbi, bi.eg, gm, did_inc);
			gen_type dgl = bi.gl;
			gen_set dbim = dbi->match();
			if ( ! dbim.empty() && dbim.min() == 0 ) {  // set new match-fail backtrack if needed
				dgl = gm+1;
				did_inc = true;
			}
			dbs.emplace_back(bi.g, dbig, dbi, dgl);
		}
		
		// Add new lookahead backtrack if needed
		gen_type dabm = da->back().max();
		if ( dabm > a->back().max() ) {
			assert(dabm == a->back().max() + 1 && "backtrack gen only grows by 1");
			gen_type gl = 0;
			gen_set bm = b->match();
			if ( ! bm.empty() && bm.min() == 0 ) {  // set new match-fail backtrack if needed
				gl = gm+1;
				did_inc = true;
			}
			dbs.emplace_back(dabm, expr::new_back_map(b, gm, did_inc), b, gl);
		}
		
		return expr::make_ptr<seq_expr>(memo, da, b, dbs, dc, dcg, gm + did_inc);
	}
	
	gen_set seq_expr::match_set() const {
		// Include matches from match-fail follower
		gen_set x = cg(c->match());
		
		// Include matches from matching lookahead successors
		gen_set am = a->match();
		
		auto at = am.begin();
		auto bit = bs.begin();
		while ( at != am.end() && bit != bs.end() ) {
			auto& bi = *bit;
			auto  ai = *at;
			
			// Find matching generations
			if ( bi.g < ai ) { ++bit; continue; }
			else if ( bi.g > ai ) { ++at; continue; }
			
			// Add followers to match set as well as follower match-fail
			x |= bi.eg(bi.e->match());
			if ( bi.gl > 0 ) x |= bi.gl;
			
			++at; ++bit;
		}
		
		return x;
	}
		
	gen_set seq_expr::back_set() const {
		// Include backtrack from match-fail follower
		gen_set x = cg(c->back());
		
		// Include lookahead follower backtracks
		for (const look_node& bi : bs) {
			x |= bi.eg(bi.e->back());
			if ( bi.gl > 0 ) x |= bi.gl;
		}
		
		// Check for gen-zero backtrack from predecessor (successors included earlier)
		if ( a->back().min() == 0 ) {
			x |= 0;
		}
		
		return x;
	}
	
} // namespace derivs

