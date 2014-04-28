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

#include <iostream>

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
		static gen_set new_back_map(ptr<expr> e, gen_type gm, bool& did_inc);
		static inline gen_set new_back_map(ptr<expr> e, gen_type gm) {
			bool did_inc = true;
			return new_back_map(e, gm, did_inc);
		}
		
		/// Gets the default backtracking map for an expression 
		///   (zero_set if no lookahead gens, zero_one_set otherwise)
		static gen_set default_back_map(ptr<expr> e, bool& did_inc);
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
		                               gen_type gm, bool& did_inc);
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
		void reset_memo();
		
		virtual void accept(visitor*) = 0;
		
		virtual ptr<expr> d(char x) const;
		virtual gen_set   match()   const;
		virtual gen_set   back()    const;
	
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
		
		static ptr<expr> make();
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return fail_type; }
	}; // class fail_expr
	
	/// An infinite loop failure parsing expression
	class inf_expr : public expr {
	public:
		inf_expr() = default;
		
		static ptr<expr> make();
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return inf_type; }
	}; // class inf_expr
	
	/// An empty success parsing expression
	class eps_expr : public expr {
	public:
		eps_expr() = default;
	
		static ptr<expr> make();
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return eps_type; }
	}; // class eps_expr
	
	/// An lookahead success parsing expression
	class look_expr : public expr {
	public:
		look_expr(gen_type g = 1) : b{g} {}
	
		static ptr<expr> make(gen_type g = 1);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return look_type; }
		
		gen_type b;  ///< Generation of this success match
	}; // class look_expr
	
	/// A single-character parsing expression
	class char_expr : public expr {
	public:
		char_expr(char c) : c(c) {}
		
		static ptr<expr> make(char c);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return char_type; }
		
		char c; ///< Character represented by the expression
	}; // class char_expr
	
	/// A character range parsing expression
	class range_expr : public expr {
	public:
		range_expr(char b, char e) : b(b), e(e) {}
		
		static ptr<expr> make(char b, char e);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return range_type; }
		
		char b;  ///< First character in expression range 
		char e;  ///< Last character in expression range
	}; // class range_expr
	
	/// A parsing expression which matches any character
	class any_expr : public expr {
	public:
		any_expr() = default;
		
		static ptr<expr> make();
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
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
		
		static ptr<expr> make(std::string s);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
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
		
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> r = nullptr);
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
		         gen_set ag = expr::zero_set, gen_set bg = expr::zero_set, gen_type gm = 0)
			: memo_expr(memo), a(a), b(b), ag(ag), bg(bg), gm(gm) {}
		
		/// Make an expression using the default generation rules
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b);
		/// Make an expression with the given generation maps
		static ptr<expr> make(memo_expr::table& memo, ptr<expr> a, ptr<expr> b, 
		                      gen_set ag, gen_set bg, gen_type gm);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char) const;
		virtual gen_set   match_set() const;
		virtual gen_set   back_set()  const;
		virtual expr_type type()      const { return alt_type; }
		
		ptr<expr> a;   ///< First subexpression
		ptr<expr> b;   ///< Second subexpression
		gen_set   ag;  ///< Generation flags for a
		gen_set   bg;  ///< Generation flags for b
		gen_type  gm;  ///< Maximum generation
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
	
} // namespace derivs

