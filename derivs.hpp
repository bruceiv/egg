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
		
	public:
		static const utils::uint_set empty_set;
		static const utils::uint_set zero_set;
		static const utils::uint_set one_set;
		static const utils::uint_set zero_one_set;
		
		template<typename T, typename... Args>
		static ptr<expr> make_ptr(Args&&... args) {
			return std::static_pointer_cast<expr>(std::make_shared<T>(args...));
		}
		
		/// Derivative of this expression with respect to x
		virtual ptr<expr> d(char x) const = 0;
		
		/// Accept visitor
		virtual void accept(visitor*) = 0;
		
		/// At what backtracking generations does this expression match?
		virtual utils::uint_set match() const = 0;
		
		/// What backtracking generations does this expression expose?
		virtual utils::uint_set back() const = 0;
				
		/// Expression node type
		virtual expr_type type() const = 0;
	}; // class expr
	
	const utils::uint_set expr::empty_set{};
	const utils::uint_set expr::zero_set{0};
	const utils::uint_set expr::one_set{1};
	const utils::uint_set expr::zero_one_set{0,1};
	
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
		virtual utils::uint_set match_set() const = 0;
		
		/// Actual computation of backtrack set
		virtual utils::uint_set back_set() const = 0;
				
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
		
		virtual utils::uint_set match() const {
			if ( ! flags.match ) {
				memo_match = match_set();
				flags.match = true;
			}
			
			return memo_match;
		}
		
		virtual utils::uint_set back() const {
			if ( ! flags.back ) {
				memo_back = back_set();
				flags.back = true;
			}
			
			return memo_back;
		}
	
	protected:
		table&                  memo;        ///< Memoization table for derivatives
		mutable utils::uint_set memo_match;  ///< Stored match set
		mutable utils::uint_set memo_back;   ///< Stored backtracking set
		mutable struct {
			bool match : 1; ///< Is there a match set stored?
			bool back  : 1; ///< Is there a backtrack set stored?
		} flags;                             ///< Memoization status flags
	}; // class memo_expr
	
	/// A failure parsing expression
	class fail_expr : public expr {
	public:
		fail_expr() = default;
		
		static ptr<expr> make() { return expr::make_ptr<fail_expr>(); }
		
		void accept(visitor* v) { v->visit(*this); }
		
		// A failure expression can't un-fail - no strings to match with any prefix
		virtual ptr<expr> d(char) const { return fail_expr::make(); }
		
		virtual utils::uint_set match() const { return expr::empty_set; }
		virtual utils::uint_set back()  const { return expr::zero_set; }
		virtual expr_type       type()  const { return fail_type; }
	}; // class fail_expr
	
	/// An infinite loop failure parsing expression
	class inf_expr : public expr {
	public:
		inf_expr() = default;
		
		static ptr<expr> make() { return expr::make_ptr<inf_expr>(); }
		
		void accept(visitor* v) { v->visit(*this); }
		
		// An infinite loop expression never breaks, ill defined with any prefix
		virtual ptr<expr> d(char) const { return inf_expr::make(); }
		
		virtual utils::uint_set match() const { return expr::empty_set; }
		virtual utils::uint_set back()  const { return expr::zero_set; }
		virtual expr_type       type()  const { return inf_type; }
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
		
		virtual utils::uint_set match() const { return expr::zero_set; }
		virtual utils::uint_set back()  const { return expr::zero_set; }
		virtual expr_type       type()  const { return eps_type; }
	}; // class eps_expr
	
	/// An lookahead success parsing expression
	class look_expr : public expr {
	public:
		look_expr(utils::uint_set::value_type g = 1) : b{g} {}
	
		static ptr<expr> make(utils::uint_set::value_type g = 1) {
			return (g == 0) ? expr::make_ptr<eps_expr>() : expr::make_ptr<look_expr>(g);
		}
		
		void accept(visitor* v) { v->visit(*this); }
		
		// No prefixes to remove from language containing the empty string; all fail
		virtual ptr<expr> d(char x) const { return look_expr::make(b); }
		
		virtual utils::uint_set match() const { return utils::uint_set{b}; }
		virtual utils::uint_set back()  const { return utils::uint_set{b}; }
		virtual expr_type       type()  const { return look_type; }
		
		utils::uint_set::value_type b;  ///< Generation of this success match
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
		
		virtual utils::uint_set match() const { return expr::empty_set; }
		virtual utils::uint_set back()  const { return expr::zero_set; }
		virtual expr_type       type()  const { return char_type; }
		
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
		
		virtual utils::uint_set match() const { return expr::empty_set; }
		virtual utils::uint_set back()  const { return expr::zero_set; }
		virtual expr_type       type()  const { return range_type; }
		
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
		
		virtual utils::uint_set match() const { return expr::empty_set; }
		virtual utils::uint_set back()  const { return expr::zero_set; }
		virtual expr_type       type()  const { return any_type; }
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
		
		virtual utils::uint_set match() const { return expr::empty_set; }
		virtual utils::uint_set back()  const { return expr::zero_set; }
		virtual expr_type       type()  const { return str_type; }
		
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
		virtual utils::uint_set  match_set() const;
		virtual utils::uint_set  back_set()  const;
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
		virtual utils::uint_set  match_set() const;
		virtual utils::uint_set  back_set()  const;
		virtual expr_type type()      const { return not_type; }
		
		virtual utils::uint_set match() const { return not_expr::match_set(); }
		virtual utils::uint_set back()  const { return not_expr::back_set(); }
		
		ptr<expr> e;  ///< Subexpression to negatively match
	}; // class not_expr
	
	/// Maintains generation mapping from collapsed alternation expression.
	class map_expr : public memo_expr {
	public:
		map_expr(memo_expr::table& memo, ptr<expr> e, utils::uint_set::value_type gm, utils::uint_set eg)
			: memo_expr(memo), e(e), gm(gm), eg(eg) {}
		
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> e, utils::uint_set::value_type mg, utils::uint_set eg);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char) const;
		virtual utils::uint_set  match_set() const;
		virtual utils::uint_set  back_set()  const;
		virtual expr_type type()      const { return map_type; }
		
		ptr<expr> e;              ///< Subexpression
		utils::uint_set::value_type gm;  ///< Maximum generation from source expresssion
		utils::uint_set             eg;  ///< Generation flags for subexpression
	}; // class map_expr
	
	/// A parsing expression representing the alternation of two parsing expressions
	class alt_expr : public memo_expr {
	public:
		alt_expr(memo_expr::table& memo, ptr<expr> a, ptr<expr> b, 
		         utils::uint_set ag = expr::zero_set, utils::uint_set bg = expr::zero_set)
			: memo_expr(memo), a(a), b(b), ag(ag), bg(bg) {}
		
		/// Make an expression using the default generation rules
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b);
		/// Make an expression with the given generation maps
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b, 
		                      utils::uint_set ag, utils::uint_set bg);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char) const;
		virtual utils::uint_set  match_set() const;
		virtual utils::uint_set  back_set()  const;
		virtual expr_type type()      const { return alt_type; }
		
		ptr<expr> a;   ///< First subexpression
		ptr<expr> b;   ///< Second subexpression
		utils::uint_set ag;  ///< Generation flags for a
		utils::uint_set bg;  ///< Generation flags for b
	}; // class alt_expr
	
	/// A parsing expression representing the concatenation of two parsing expressions
	class seq_expr : public memo_expr {
	private:
		// Calculates the backtrack map for b
		utils::uint_set bg() const;
	public:
		struct look_node {
			look_node(utils::uint_set::value_type g, utils::uint_set eg, ptr<expr> e) : g(g), eg(eg), e(e) {}
			
			utils::uint_set::value_type g;   ///< Backtrack generation this follower corresponds to
			utils::uint_set             eg;  ///< Map of generations from this node to the containing node
			ptr<expr> e;              ///< Follower expression for this lookahead generation
		}; // struct look_list
		using look_list = std::list<look_node>;
		
		seq_expr(memo_expr::table& memo, ptr<expr> a, ptr<expr> b)
			: memo_expr(memo), a(a), b(b), bs(), c(fail_expr::make()), cg(expr::zero_set), gm(0) {}
		
		seq_expr(memo_expr::table& memo, ptr<expr> a, ptr<expr> b, look_list bs, 
		         ptr<expr> c, utils::uint_set cg, utils::uint_set::value_type gm)
			: memo_expr(memo), a(a), b(b), bs(bs), c(c), cg(cg), gm(gm) {}
	
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char) const;
		virtual utils::uint_set  match_set() const;
		virtual utils::uint_set  back_set()  const;
		virtual expr_type type()      const { return seq_type; }
		
		ptr<expr> a;   ///< First subexpression
		ptr<expr> b;   ///< Gen-zero follower
		look_list bs;  ///< List of following subexpressions for each backtrack generation
		ptr<expr> c;   ///< Matching backtrack value
		utils::uint_set  cg;  ///< Backtrack map for c
		utils::uint_set::value_type gm;  ///< Maximum backtrack generation
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
	
	utils::uint_set rule_expr::match_set() const {
		// Stop this from infinitely recursing
		flags.match = true;
		memo_match = expr::empty_set;
		
		// Calculate match set
		return r->match();
	}
	
	utils::uint_set rule_expr::back_set() const {
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
	
	utils::uint_set not_expr::match_set() const { return expr::empty_set; }
	
	utils::uint_set not_expr::back_set() const { return expr::one_set; }
	
	// map_expr ////////////////////////////////////////////////////////////////////
	
	ptr<expr> map_expr::make(memo_expr::table& memo, ptr<expr> e, 
	                         utils::uint_set::value_type gm, utils::uint_set eg) {
		// account for unmapped generations
		auto n = eg.count();
		for (auto i = e->back().max() + 1; i < n; ++i) eg |= ++gm;
		
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
		if ( gm == eg.count() ) return e;
		
		return expr::make_ptr<map_expr>(memo, e, gm, eg);
	}
	
	ptr<expr> map_expr::deriv(char x) const {
		ptr<expr> de = e->d(x);
		
		// Check conditions on de [same as make]
		switch ( e->type() ) {
		// Map expression match generation into exit generation
		case eps_type:  return look_expr::make(eg(0));
		case look_type: return look_expr::make(eg(e->match().max()));
		// Propegate fail and infinity errors
		case fail_type: return e; // a fail_expr
		case inf_type:  return e; // an inf_expr
		default:        break; // do nothing
		}
		
		// Calculate generations of new subexpressions
		// - If we've added a lookahead generation that wasn't there before, map it into the 
		//   generation space of the derived alternation
		utils::uint_set             deg = eg;
		utils::uint_set::value_type dgm = gm;
		if ( de->back().max() > e->back().max() ) {
			assert((de->back().max() == e->back().max() + 1) && "gen only grows by 1");
			deg |= ++dgm;
		}
		
		return expr::make_ptr<map_expr>(memo, de, dgm, deg);
	}
	
	utils::uint_set map_expr::match_set() const { return eg(e->match()); }
	
	utils::uint_set map_expr::back_set() const { return eg(e->back()); }
	
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
		
		return expr::make_ptr<alt_expr>(memo, a, b, a->back(), b->back());
	}
	
	ptr<expr> alt_expr::make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b, 
	                         utils::uint_set ag, utils::uint_set bg) {
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
		
		return expr::make_ptr<alt_expr>(memo, a, b, a->back(), b->back());
	}
	
	ptr<expr> alt_expr::deriv(char x) const {
		utils::uint_set::value_type gm = std::max(ag.max(), bg.max());
		bool did_inc = false;
		
		// Calculate derivative and map in new lookahead generations
		ptr<expr> da = a->d(x); utils::uint_set dag = ag;
		if ( da->back().max() > a->back().max() ) {
			assert((da->back().max() == a->back().max() + 1) && "gen only grows by 1");
			dag |= gm + 1;
			did_inc = true;
		}
				
		// Check conditions on a before we calculate dx(b) [same as make()]
		switch ( da->type() ) {
		case fail_type: {
			ptr<expr> db = b->d(x); utils::uint_set dbg = bg;
			if ( db->back().max() > b->back().max() ) {
				assert((db->back().max() == b->back().max() + 1) && "gen only grows by 1");
				dbg |= gm + 1;
				did_inc = true;
			}
			return map_expr::make(memo, db, gm + did_inc, dbg);
		}
		case inf_type:  return da; // an inf_expr
		default:        break; // do nothing
		}
		if ( ! da->match().empty() ) {
			return map_expr::make(memo, da, gm + did_inc, dag);
		}
		
		// Calculate other derivative and map in new lookahead generations
		ptr<expr> db = b->d(x); utils::uint_set dbg = bg;
		if ( db->type() == fail_type ) return map_expr::make(memo, da, gm + did_inc, dag);
		
		if ( db->back().max() > b->back().max() ) {
			dbg |= gm + 1;
		}
		
		return expr::make_ptr<alt_expr>(memo, da, db, dag, dbg);
	}
	
	utils::uint_set alt_expr::match_set() const { return ag(a->match()) | bg(b->match()); }
	
	utils::uint_set alt_expr::back_set() const { return ag(a->back()) | bg(b->back()); }
	
	// seq_expr ////////////////////////////////////////////////////////////////////
	
	utils::uint_set seq_expr::bg() const {
		utils::uint_set x = expr::zero_set;
		utils::uint_set bb = b->back();
		assert(!bb.empty() && "backtrack set is always non-empty");
		if ( bb.max() > 0 ) {
			x |= gm;
			assert(bb.max() == 1 && "follower has static gen <= 1");
		}
		return x;
	}
	
	ptr<expr> seq_expr::make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b) {
		switch ( b->type() ) {
		// empty second element just leaves first
		case eps_type:  return a;
		// failing second element propegates
		case fail_type: return b;
		default: break;  // do nothing
		}
		
		switch ( a->type() ) {
		// empty first element just leaves follower
		case eps_type:
			return b;
		// lookahead success first element gives the second if it was first-gen, 
		//   otherwise it fails for lack of successor
		case look_type: 
			return ( std::static_pointer_cast<look_expr>(a)->b == 1 ) ? b : fail_expr::make();
		// failure or infinite loop propegates
		case fail_type:
		case inf_type:
			return a;
		default: break;  // do nothing
		}
		
		// set up lookahead generations
		utils::uint_set::value_type gm = 0;
		utils::uint_set ab = a->back();
		assert(!ab.empty() && "backtrack set is always non-empty");
		
		// Set up follower if first expression isn't lookahead
		ptr<expr> bn;
		if ( ab.min() == 0 ) {
			bn = b;
			utils::uint_set bb = b->back();
			assert(!bb.empty() && "backtrack set is always non-empty");
			if ( bb.max() > 0 ) {
				assert(bb.max() == 1 && "static backtrack gen <= 1");
				gm = 1;
			}
		} else {
			bn = fail_expr::make();
		}
		
		// Set up lookahead follower if first expression is lookahead
		look_list bs;
		if ( ab.max() > 0 ) {
			assert(ab.max() == 1 && "static backtrack gen <= 1");
			bs.emplace_back(1, b->back(), b);
			gm = 1;
		}
		
		// set up match follower
		ptr<expr> c = fail_expr::make();
		utils::uint_set cg = expr::zero_set;
		
		// return constructed expression
		return expr::make_ptr<seq_expr>(memo, a, bn, bs, c, cg, gm);
	}
	
	ptr<expr> seq_expr::deriv(char x) const {
		bool did_inc = false;
	
		ptr<expr> da = a->d(x);
		
		switch ( da->type() ) {
		case eps_type: {
			return map_expr::make(memo, b, gm, bg());
		} case look_type: { 
			// lookahead success leaves appropriate lookahead follower
			auto i = std::static_pointer_cast<look_expr>(da)->b;
			for (const look_node& bi : bs) {
				if ( bi.g == i ) return map_expr::make(memo, bi.e, gm, bi.eg);
				else if ( bi.g > i ) break;  // generation list is sorted
			}
			return fail_expr::make();  // if none found, fail
		} case inf_type: {
			// infinite loop element propegates
			return da; // an inf_expr
		} default: break; // do nothing
		}
		
		// Match backtrack comes from b if a matches, or the previous match backtrack otherwise
		ptr<expr> dc;
		utils::uint_set dcg;
		
		utils::uint_set am = a->match();
		if ( am.empty() || am.min() > 0 ) {
			// no new match, so continue parsing previous match's backtrack
			dc = c->d(x); dcg = cg;
			if ( dc->back().max() > c->back().max() ) {
				assert((dc->back().max() == c->back().max() + 1) && "gen only grows by 1");
				dcg |= gm + 1;
				did_inc = true;
			}
		} else {
			// new match, start new backtrack
			dc = b->d(x); dcg = bg();
			
			if ( dc->back().max() > b->back().max() ) {
				assert((dc->back().max() == b->back().max() + 1) && "gen only grows by 1");
				dcg |= gm + 1;
				did_inc = true;
			}
		}
		
		// break out here if d(a) failed and just use the calculated failure successor
		if ( da->type() == fail_type ) return map_expr::make(memo, dc, gm + did_inc, dcg);
		
		// Build derivatives of lookahead backtracks
		look_list dbs;
		auto bit = bs.begin();
		utils::uint_set dab = da->back();
		auto dat = dab.begin();
		
		while ( dat != dab.end() && bit != bs.end() ) {
			auto dai = *dat;
			
			// Find matching backtrack list element
			auto bi = *bit;
			while ( bi.g < dai ) {
				++bit;
				if ( bit == bs.end() ) goto bsdone; //labelled break
				bi = *bit;
			}
			assert(bi.g = dai && "backtrack list includes back items");
			
			// Take derivative
			ptr<expr> dbi = bi.e->d(x);
			if ( dbi->type() != fail_type ) {
				// Map new lookahead generations into the space of the backtracking expression
				utils::uint_set dbig = bi.eg;
				
				if ( dbi->back().max() > bi.e->back().max() ) {
					assert((dbi->back().max() == bi.e->back().max() + 1) && "gen only grows by 1");
					dbig |= gm + 1;
					did_inc = true;
				}
				
				dbs.emplace_back(bi.g, dbig, dbi);
			}
		}
		
		// add new lookahead backtrack
bsdone:	if ( dat != dab.end() ) {
			auto dai = *dat;
			
			ptr<expr> dbi = b->d(x);
			if ( dbi->type() != fail_type ) {
				utils::uint_set dbig = bg();	
				
				if ( dbi->back().max() > b->back().max() ) {
					assert((dbi->back().max() == b->back().max() + 1) && "gen only grows by 1");
					dbig |= gm + 1;
					did_inc = true;
				}
				
				dbs.emplace_back(dai, dbig, dbi);
			}
			
			assert(++dat == dab.end() && "Only one new lookahead generation");
		}
		
		// return constructed expression
		return expr::make_ptr<seq_expr>(memo, da, b, dbs, dc, dcg, gm + did_inc);
	}
	
	utils::uint_set seq_expr::match_set() const {
		utils::uint_set x;
		
		utils::uint_set am = a->match();
		auto at = am.begin();
		
		// If a's match set is empty, return match set from backtrack
		if ( at == am.end() ) return cg(c->match());
		
		auto ai = *at;
		if ( ai == 0 ) {  // Matches on generation 0, look at b's matches
			x |= bg()(b->match());
			++at;
		} else {  // No match on generation 0, look at c's matches
			x |= cg(c->match());
		}
		
		auto bit = bs.begin();
		while (at != am.end()) {
			ai = *at;
			
			assert(bit != bs.end() && "match set subset of backtrack list");
			auto bi = *bit;
			while ( bi.g < ai ) {
				++bit; 
				assert(bit != bs.end() && "match set subset of backtrack list");
				bi = *bit;
			}
			assert(bi.g == ai && "backtrack list includes match items");
			
			x |= bi.eg(bi.e->match());
			
			++at;
		}
		
		return x;
	}
	
	utils::uint_set seq_expr::back_set() const {
		utils::uint_set x;
		
		utils::uint_set am = a->match();
		
		// If zero not included in match set, add backtracking from c
		if ( am.empty() || am.min() > 0 ) { x |= cg(c->back()); }
		// otherwise include backtracking from b
		else { x |= bg()(b->back()); }
		
		for (auto& bi : bs) {
			x |= bi.eg(bi.e->back());
		}
		
		return x;
	}
	
} // namespace derivs

