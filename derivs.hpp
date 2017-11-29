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

#include <limits>
#include <memory>
// #include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ast.hpp"
#include "fixer.hpp"

#include "utils/uint_set.hpp"

/**
 * Implements derivative parsing for parsing expression grammars, according to the algorithm 
 * described by Aaron Moss in 2014 (http://arxiv.org/abs/1405.4841).
 * 
 * The basic idea of this derivative parsing algorithm is to repeatedly take the "derivative" of a 
 * parsing expression with respect to the next character in the input sequence, where the 
 * derivative is a parsing expression which matches the suffixes of all strings in the language of 
 * the original expression which start with the given prefix.
 */
namespace derivs {
	template <typename T> using ptr = std::shared_ptr<T>;
	template <typename T> using memo_ptr = std::weak_ptr<T>;

	// /// single backtrack generation
	// using gen_type = unsigned long;
	// /// set of backtrack generations
	// using gen_set = std::set<gen_type>;

	// /// sentinel generation indicating none-such
	// static constexpr gen_type no_gen = std::numeric_limits<gen_type>::max();

	// /// Gets first index in a generation set; undefined for empty set
	// static inline gen_type first( const gen_set& gs ) { return *gs.begin(); }

	// /// Gets last index in a generation set; undefined for empty set
	// static inline gen_type last( const gen_set& gs ) { return *gs.rbegin(); }

	// /// Inserts a value into a generation set
	// static inline void set_add( gen_set& a, gen_type g ) { a.insert( g ); }

	// /// Takes set union of two generation sets
	// static inline void set_union( gen_set& a, const gen_set& b ) {
	// 	a.insert( b.begin(), b.end() );
	// }

	/// single backtrack generation
	using gen_type = utils::uint_set::value_type;
	/// set of backtrack generations
	using gen_set = utils::uint_set;

	/// sentinel generation indicating none-such
	static constexpr gen_type no_gen = std::numeric_limits<gen_type>::max();

	/// Gets first index in a generation set; undefined for empty set
	static inline gen_type first( const gen_set& gs ) { return gs.min(); }

	/// Gets last index in a generation set; undefined for empty set
	static inline gen_type last( const gen_set& gs ) { return gs.max(); }

	/// Inserts a value into a generation set
	static inline void set_add( gen_set& a, gen_type g ) { a |= g; }

	/// Takes set union of two generation sets
	static inline void set_union( gen_set& a, const gen_set& b ) { a |= b; }
	
	// Forward declarations of expression node types
	class fail_expr;
	class inf_expr;
	class eps_expr;
	class char_expr;
	class except_expr;
	class range_expr;
	class except_range_expr;
	class any_expr;
	class none_expr;
	class str_expr;
	class not_expr;
	class alt_expr;
	class opt_expr;
	class or_expr;
	class and_expr;
	class seq_expr;
	
	/// Type of expression node
	enum expr_type {
		fail_type,
		inf_type,
		eps_type,
		char_type,
		except_type,
		range_type,
		except_range_type,
		any_type,
		none_type,
		str_type,
		not_type,
		alt_type,
		opt_type,
		or_type,
		and_type,
		seq_type
	}; // enum expr_type
	
	std::ostream& operator<< (std::ostream& out, expr_type t);
	
	/// Abstract base class of all derivative visitors
	class visitor {
	public:
		virtual void visit(fail_expr&)  = 0;
		virtual void visit(inf_expr&)   = 0;
		virtual void visit(eps_expr&)   = 0;
		virtual void visit(char_expr&)  = 0;
		virtual void visit(except_expr&) = 0;
		virtual void visit(range_expr&) = 0;
		virtual void visit(except_range_expr&) = 0;
		virtual void visit(any_expr&)   = 0;
		virtual void visit(none_expr&)  = 0;
		virtual void visit(str_expr&)   = 0;
		virtual void visit(not_expr&)   = 0;
		virtual void visit(alt_expr&)   = 0;
		virtual void visit(opt_expr&)   = 0;
		virtual void visit(or_expr&)    = 0;
		virtual void visit(and_expr&)   = 0;
		virtual void visit(seq_expr&)   = 0;
	}; // class visitor
	
	/// Abstract base class for parsing expressions
	class expr {
	friend class fixer;
	protected:
		expr() = default;
		
	public:
		template<typename T, typename... Args>
		static ptr<expr> make_ptr(Args&&... args) {
			return std::static_pointer_cast<expr>(
					std::make_shared<T>(
						std::forward<Args>(args)...));
		}
		
		/// Derivative of this expression with respect to x
		virtual ptr<expr> d(char x, gen_type i) const = 0;
		
		/// Accept visitor
		virtual void accept(visitor*) = 0;
		
		/// At what backtracking generations does this expression match?
		virtual gen_set match() const = 0;
		
		/// What backtracking generations does this expression expose?
		virtual gen_set back() const = 0;
				
		/// Expression node type
		virtual expr_type type() const = 0;
	}; // class expr
	
	/// List of expression pointers
	using expr_list = std::vector<ptr<expr>>;
	
	/// Abstract base class for memoized parsing expressions
	class memo_expr : public expr {
	friend class fixer;
	protected:
		/// Constructor providing memoization table reference
		memo_expr() : memo_d(), last_index(no_gen) { flags = {false,false}; }
		
		/// Actual derivative calculation
		virtual ptr<expr> deriv(char, gen_type) const = 0;
		
		/// Actual computation of match set
		virtual gen_set match_set() const = 0;
		
		/// Actual computation of backtrack set
		virtual gen_set back_set() const = 0;
				
	public:
		/// Resets the memoziation fields of a memoized expression
		void reset_memo();
		
		virtual void accept(visitor*) = 0;
		
		virtual ptr<expr> d(char x, gen_type i) const;
		virtual gen_set   match()   const;
		virtual gen_set   back()    const;
	
	protected:
		mutable memo_ptr<expr> memo_d; ///< Stored derivative
		mutable gen_set memo_match;    ///< Stored match set
		mutable gen_set memo_back;     ///< Stored backtracking set
		/// Index of last stored derivative [no_gen for none such]
		mutable gen_type last_index;
		mutable struct {
			bool match : 1; ///< Is there a match set stored?
			bool back  : 1; ///< Is there a backtrack set stored?
		} flags;                     ///< Memoization status flags
	}; // class memo_expr
	
	/// A failure parsing expression
	class fail_expr : public expr {
		// implements singleton pattern; access through make()
		fail_expr() = default;
		fail_expr(const fail_expr&) = delete;
		fail_expr(fail_expr&&) = delete;
		
		fail_expr& operator = (const fail_expr&) = delete;
		fail_expr& operator = (fail_expr&&) = delete;
	public:
		~fail_expr() = default;

		static ptr<expr> make();
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char, gen_type) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return fail_type; }
	}; // class fail_expr
	
	/// An infinite loop failure parsing expression
	class inf_expr : public expr {
		// implements singleton pattern; access through make()
		inf_expr() = default;
		inf_expr(const inf_expr&) = delete;
		inf_expr(inf_expr&&) = delete;
		
		inf_expr& operator = (const inf_expr&) = delete;
		inf_expr& operator = (inf_expr&&) = delete;
	public:
		~inf_expr() = default;
		
		static ptr<expr> make();
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char, gen_type) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return inf_type; }
	}; // class inf_expr
	
	/// An empty success parsing expression
	class eps_expr : public expr {
	public:
		eps_expr(gen_type g) : g{g} {}
		
		static ptr<expr> make(gen_type g);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char, gen_type) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return eps_type; }

		gen_type g; ///< Input index where this match occured
	}; // class eps_expr
	
	/// A single-character parsing expression
	class char_expr : public expr {
	public:
		char_expr(char c) : c(c) {}
		
		static ptr<expr> make(char c);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char, gen_type) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return char_type; }
		
		char c; ///< Character represented by the expression
	}; // class char_expr

	/// A single-character-excluded parsing expression
	class except_expr : public expr {
	public:
		except_expr(char c) : c(c) {}
		
		static ptr<expr> make(char c);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char, gen_type) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return except_type; }
		
		char c; ///< Character excluded by the expression
	}; // class except_expr
	
	/// A character range parsing expression
	class range_expr : public expr {
	public:
		range_expr(char b, char e) : b(b), e(e) {}
		
		static ptr<expr> make(char b, char e);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char, gen_type) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return range_type; }
		
		char b;  ///< First character in expression range 
		char e;  ///< Last character in expression range
	}; // class range_expr

	/// A character range-excluded parsing expression
	class except_range_expr : public expr {
	public:
		except_range_expr(char b, char e) : b(b), e(e) {}
		
		static ptr<expr> make(char b, char e);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char, gen_type) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return except_range_type; }
		
		char b;  ///< First character in excluded range 
		char e;  ///< Last character in excluded range
	}; // class except_range_expr
	
	/// A parsing expression which matches any character
	class any_expr : public expr {
	public:
		any_expr() = default;
		
		static ptr<expr> make();
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char, gen_type) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return any_type; }
	}; // class any_expr

	/// A parsing expression which matches end-of-input
	class none_expr : public expr {
	public:
		none_expr() = default;
		
		static ptr<expr> make();
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char, gen_type) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return none_type; }
	}; // class none_expr

	/// A parsing expression representing a character string
	class str_expr : public expr {
		str_expr(ptr<std::string> sp, unsigned long si) : sp(sp), si(si) {}
	public:
		str_expr(const std::string& s) : sp{std::make_shared<std::string>(s)}, si{0} {}
		str_expr(std::string&& s) : sp{std::make_shared<std::string>(s)}, si{0} {}
		
		static ptr<expr> make(const std::string& s);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char, gen_type) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return str_type; }
		
		std::string str() const { return std::string{*sp, si}; }
		unsigned long size() const { return sp->size() - si; }
	private:
		ptr<std::string> sp;  ///< Pointer to interred string
		unsigned long si;     ///< Index into interred string
	}; // class str_expr
	
	/// A parsing expression representing negative lookahead
	class not_expr : public memo_expr {
	public:
		not_expr(ptr<expr> e, gen_type g) : memo_expr(), e(e), g(g) {}
		
		static ptr<expr> make(ptr<expr> e, gen_type g);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char, gen_type) const;
		virtual gen_set   match_set() const;
		virtual gen_set   back_set()  const;
		virtual expr_type type()      const { return not_type; }
		
		virtual gen_set match() const { return not_expr::match_set(); }
		virtual gen_set back()  const { return not_expr::back_set(); }
		
		ptr<expr> e;  ///< Subexpression to negatively match
		gen_type g;   ///< Index at which expression ceased consuming input
	}; // class not_expr
	
	/// A parsing expression representing the alternation of two or more parsing expressions
	class alt_expr : public memo_expr {
	public:
		alt_expr(ptr<expr> a, ptr<expr> b) : memo_expr(), es{a, b}, gl(no_gen) {}
		
		alt_expr(const expr_list& es, gen_type gl) : memo_expr(), es(es), gl(gl) {}
		
		/// Make an expression using the default generation rules
		static ptr<expr> make(ptr<expr> a, ptr<expr> b);
		/// Make an expression using the default generation rules
		static ptr<expr> make(const expr_list& es);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char, gen_type) const;
		virtual gen_set   match_set() const;
		virtual gen_set   back_set()  const;
		virtual expr_type type()      const { return alt_type; }
		
		expr_list  es;  ///< List of subexpressions, ordered by priority
		gen_type   gl;  ///< Index of last epsilon match [no_gen for none such]
	}; // class alt_expr

	/// A parsing expression representing the alternation of a non-matching parsing 
	/// expression and an epsilon expression
	class opt_expr : public memo_expr {
	public:
		opt_expr(ptr<expr> e, gen_type gl) : memo_expr(), e(e), gl(gl) {}

		static ptr<expr> make(ptr<expr> e, gen_type gl);
		void accept(visitor* v) { v->visit(*this); }

		virtual ptr<expr> deriv(char, gen_type) const;
		virtual gen_set   match_set() const;
		virtual gen_set   back_set()  const;
		virtual expr_type type()      const { return opt_type; }

		virtual gen_set match() const { return opt_expr::match_set(); }

		ptr<expr> e;  ///< Subexpression to match
		gen_type gl;  ///< Failure match index for e
	}; // class opt_expr

	/// A parsing expression representing the simultaneous match of any of multiple 
	/// expressions at the same position. Only guaranteed to work for single-character 
	/// subexpressions.
	class or_expr : public memo_expr {
	public:
		or_expr(expr_list&& es) : memo_expr(), es(std::move(es)) {}
		
		/// Make an expression using the default generation rules
		static ptr<expr> make(expr_list&& es);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char, gen_type) const;
		virtual gen_set   match_set() const;
		virtual gen_set   back_set()  const;
		virtual expr_type type()      const { return or_type; }
		
		expr_list es;  ///< List of subexpressions
	}; // class or_expr

	/// A parsing expression representing the simultaneous match of multiple expressions 
	/// at the same position. Only guaranteed to work for single-character subexpressions.
	class and_expr : public memo_expr {
	public:
		and_expr(expr_list&& es) : memo_expr(), es(es) {}
		
		/// Make an expression using the default generation rules
		static ptr<expr> make(expr_list&& es);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char, gen_type) const;
		virtual gen_set   match_set() const;
		virtual gen_set   back_set()  const;
		virtual expr_type type()      const { return and_type; }
		
		expr_list es;  ///< List of subexpressions
	}; // class and_expr
	
	/// A parsing expression representing the concatenation of two parsing expressions
	class seq_expr : public memo_expr {
	public:
		struct look_node {
			look_node(gen_type g, ptr<expr> e, gen_type gl = no_gen) 
				: g(g), e(e), gl(gl) {}
			
			gen_type  g;   ///< Backtrack index this follower corresponds to
			ptr<expr> e;   ///< Follower expression for this lookahead generation
			gen_type  gl;  ///< Index of last match [no_gen for none such]
		}; // struct look_node
		using look_list = std::vector<look_node>;
		
		seq_expr(ptr<expr> a, ast::matcher_ptr b)
			: memo_expr(), a(a), b(b), bs(), gl(no_gen) {}
		
		seq_expr(ptr<expr> a, ast::matcher_ptr b, look_list&& bs, gen_type gl)
			: memo_expr(), a(a), b(b), bs(std::move(bs)), gl(gl) {}
	
		static ptr<expr> make(ptr<expr> a, ast::matcher_ptr b, gen_type gl = 0);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char, gen_type) const;
		virtual gen_set   match_set() const;
		virtual gen_set   back_set()  const;
		virtual expr_type type()      const { return seq_type; }
		
		ptr<expr> a;         ///< First subexpression
		ast::matcher_ptr b;  ///< Un-normalized second subexpression
		look_list bs;        ///< List of following subexpressions for each backtrack index
		gen_type gl;         ///< Last matching generation [no_gen for none such]
	}; // class seq_expr
	
} // namespace derivs

