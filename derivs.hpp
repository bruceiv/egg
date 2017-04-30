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

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "utils/uint_pfn.hpp"

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
	
	/// map of backtrack generations
	using gen_map  = utils::uint_pfn;
	/// set of backtrack generations
	using gen_set  = gen_map::set_type;
	/// single backtrack generation
	using gen_type = gen_map::value_type;
	
	// Forward declarations of expression node types
	class fail_expr;
	class inf_expr;
	class eps_expr;
	class look_expr;
	class char_expr;
	class range_expr;
	class any_expr;
	class none_expr;
	class str_expr;
	class rule_expr;
	class not_expr;
	class map_expr;
	class alt_expr;
	class ualt_expr;
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
		none_type,
		str_type,
		rule_type,
		not_type,
		map_type,
		alt_type,
		ualt_type,
		seq_type
	}; // enum expr_type
	
	std::ostream& operator<< (std::ostream& out, expr_type t);
	
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
		virtual void visit(none_expr&)  = 0;
		virtual void visit(str_expr&)   = 0;
		virtual void visit(rule_expr&)  = 0;
		virtual void visit(not_expr&)   = 0;
		virtual void visit(map_expr&)   = 0;
		virtual void visit(alt_expr&)   = 0;
		virtual void visit(ualt_expr&)  = 0;
		virtual void visit(seq_expr&)   = 0;
	}; // class visitor
	
	/// Abstract base class for parsing expressions
	class expr {
	friend class fixer;
	protected:
		expr() = default;
		
		/// Creates a new backtrack map to map an expression into a generation space
		static gen_map new_back_map(ptr<expr> e, gen_type gm, bool& did_inc);
		static inline gen_map new_back_map(ptr<expr> e, gen_type gm) {
			bool did_inc = true;
			return new_back_map(e, gm, did_inc);
		}
		
		/// Gets the default backtracking map for an expression 
		///   (zero_map if no lookahead gens, zero_one_map otherwise)
		static gen_map default_back_map(ptr<expr> e, bool& did_inc);
		static inline gen_map default_back_map(ptr<expr> e) {
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
		static gen_map update_back_map(ptr<expr> e, ptr<expr> de, gen_map eg, 
		                               gen_type gm, bool& did_inc);
		static inline gen_map update_back_map(ptr<expr> e, ptr<expr> de, gen_map eg, gen_type gm) {
			bool did_inc = true;
			return update_back_map(e, de, eg, gm, did_inc);
		}
		
	public:
		template<typename T, typename... Args>
		static ptr<expr> make_ptr(Args&&... args) {
			return std::static_pointer_cast<expr>(
					std::make_shared<T>(
						std::forward<Args>(args)...));
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
	
	/// List of expression pointers
	using expr_list = std::vector<ptr<expr>>;
	
	/// Abstract base class for memoized parsing expressions
	class memo_expr : public expr {
	friend class fixer;
	protected:
		/// Constructor providing memoization table reference
		memo_expr() : memo_d(), last_char(0x7F) { flags = {false,false}; }
		
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
		mutable memo_ptr<expr> memo_d; ///< Stored derivative
		mutable gen_set memo_match;    ///< Stored match set
		mutable gen_set memo_back;     ///< Stored backtracking set
		/// Character stored derivative was taken w.r.t. [0x7F for none such]
		mutable char last_char;
		mutable struct {
			bool match : 1; ///< Is there a match set stored?
			bool back  : 1; ///< Is there a backtrack set stored?
		} flags;                     ///< Memoization status flags
	}; // class memo_expr
	
	/// Calculates least fixed point of match sets for compound expressions and stores them in the 
	/// memo table. Approach based on Kleene's fixed point theorem, as implemented in the 
	/// derivative parser of Might et al. for CFGs.
	class fixer : public visitor {
	public:
		/// Calculates the least fixed point of x->match() and memoizes it
		void operator() (ptr<expr> x);
		
		// Implements visitor
		void visit(fail_expr&);
		void visit(inf_expr&);
		void visit(eps_expr&);
		void visit(look_expr&);
		void visit(char_expr&);
		void visit(range_expr&);
		void visit(any_expr&);
		void visit(none_expr&);
		void visit(str_expr&);
		void visit(rule_expr&);
		void visit(not_expr&);
		void visit(map_expr&);
		void visit(alt_expr&);
		void visit(ualt_expr&);
		void visit(seq_expr&);
	
	private:
		/// Performs fixed point computation to calculate match set of x
		/// Based on Kleene's thm, iterates upward from a bottom set of {}
		gen_set fix_match(ptr<expr> x);
		
		/// Recursively calculates next iteration of match set
		/// @param x        The expression to match
		/// @param changed  Did some subexpression change its value for match?
		/// @param visited  Which subexpressions have been visited?
		/// @return The match set
		gen_set iter_match(ptr<expr> x, bool& changed, 
		                   std::unordered_set<ptr<expr>>& visited);
		
		/// Wraps visitor pattern for actual calculation of next match set
		/// @param x        The expression to match
		/// @param changed  Did some subexpression change its value for match?
		/// @param visited  Which subexpressions have been visited?
		/// @return The match set
		gen_set calc_match(ptr<expr> x, bool& changed, 
		                   std::unordered_set<ptr<expr>>& visited);
		
		std::unordered_set<ptr<expr>>  running;  ///< Set of expressions currently being fixed
		std::unordered_set<ptr<expr>>  fixed;    ///< Set of expressions already fixed
		std::unordered_set<ptr<expr>>* visited;  ///< Set of expressions visited in current fix
		bool*                          changed;  ///< Has anything in the current fix changed?
		gen_set                        match;    ///< Match returned by current fix
	}; // class fixer
	
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
		
		virtual ptr<expr> d(char) const;
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
		
		virtual ptr<expr> d(char) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return inf_type; }
	}; // class inf_expr
	
	/// An empty success parsing expression
	class eps_expr : public expr {
		// implements singleton pattern; access through make()
		eps_expr() = default;
		eps_expr(const eps_expr&) = delete;
		eps_expr(eps_expr&&) = delete;
		
		eps_expr& operator = (const eps_expr&) = delete;
		eps_expr& operator = (eps_expr&&) = delete;
	public:
		~eps_expr() = default;
		
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

	/// A parsing expression which matches end-of-input
	class none_expr : public expr {
	public:
		none_expr() = default;
		
		static ptr<expr> make();
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return none_type; }
	}; // class none_expr

	/// A parsing expression representing a character string
	class str_expr : public expr {
		str_expr(ptr<std::string> sp, unsigned long i) : sp(sp), i(i) {}
	public:
		str_expr(const std::string& s) : sp{std::make_shared<std::string>(s)}, i{0} {}
		str_expr(std::string&& s) : sp{std::make_shared<std::string>(s)}, i{0} {}
		
		static ptr<expr> make(const std::string& s);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> d(char) const;
		virtual gen_set   match() const;
		virtual gen_set   back()  const;
		virtual expr_type type()  const { return str_type; }
		
		std::string str() const { return std::string{*sp, i}; }
		unsigned long size() const { return sp->size() - i; }
	private:
		ptr<std::string> sp;  ///< Pointer to interred string
		unsigned long i;      ///< Index into interred string
	}; // class str_expr
	
	/// A parsing expression representing a non-terminal
	class rule_expr : public memo_expr {
	public:
		rule_expr(ptr<expr> r = nullptr) : memo_expr(), r(r) {}
		
		static ptr<expr> make(ptr<expr> r = nullptr);
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
		not_expr(ptr<expr> e) : memo_expr(), e(e) { flags.match = flags.back = true; }
		
		static ptr<expr> make(ptr<expr> e);
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
		map_expr(ptr<expr> e, gen_type gm, gen_map eg) : memo_expr(), e(e), gm(gm), eg(eg) {}
		
		static ptr<expr> make(ptr<expr> e, gen_type mg, gen_map eg);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char) const;
		virtual gen_set   match_set() const;
		virtual gen_set   back_set()  const;
		virtual expr_type type()      const { return map_type; }
		
		ptr<expr> e;   ///< Subexpression
		gen_type  gm;  ///< Maximum generation from source expresssion
		gen_map   eg;  ///< Generation flags for subexpression
	}; // class map_expr
	
	/// A parsing expression representing the alternation of two parsing expressions
	class alt_expr : public memo_expr {
	public:
		struct alt_node {
			alt_node(ptr<expr> e, gen_map eg) : e(e), eg(eg) {}

			ptr<expr> e;   ///< Subexpression
			gen_map   eg;  ///< Generation flags for subexpression
		};  // struct alt_node
		using alt_list = std::vector<alt_node>;
		
		alt_expr(ptr<expr> a, ptr<expr> b, 
				gen_map ag = gen_map{0}, gen_map bg = gen_map{0}, gen_type gm = 0)
			: memo_expr(), es{alt_node{a, ag}, alt_node{b, bg}}, gm(gm) {}
		
		alt_expr(const alt_list& es, gen_type gm) : memo_expr(), es(es), gm(gm) {}
		
		/// Make an expression using the default generation rules
		static ptr<expr> make(ptr<expr> a, ptr<expr> b);
		/// Make an expression with the given generation maps
		static ptr<expr> make(ptr<expr> a, ptr<expr> b, 
		                      gen_map ag, gen_map bg, gen_type gm);
		/// Make an expression using the default generation rules
		static ptr<expr> make(const expr_list& es);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char) const;
		virtual gen_set   match_set() const;
		virtual gen_set   back_set()  const;
		virtual expr_type type()      const { return alt_type; }
		
		alt_list  es;  ///< List of subexpressions, ordered by priority
		gen_type  gm;  ///< Maximum generation
	}; // class alt_expr

	/// A parsing expression representing the unordered alternation of two parsing expressions
	/// An optimization which accepts the first match; correctness is guaranteed by grammar writer.
	class ualt_expr : public memo_expr {
	public:
		struct alt_node {
			alt_node(ptr<expr> e, gen_map eg) : e(e), eg(eg) {}

			ptr<expr> e;   ///< Subexpression
			gen_map   eg;  ///< Generation flags for subexpression
		};  // struct alt_node
		using alt_list = std::vector<alt_node>;
		
		ualt_expr(ptr<expr> a, ptr<expr> b, 
				gen_map ag = gen_map{0}, gen_map bg = gen_map{0}, gen_type gm = 0)
			: memo_expr(), es{alt_node{a, ag}, alt_node{b, bg}}, gm(gm) {}
		
		ualt_expr(const alt_list& es, gen_type gm) : memo_expr(), es(es), gm(gm) {}
		
		/// Make an expression using the default generation rules
		static ptr<expr> make(ptr<expr> a, ptr<expr> b);
		/// Make an expression with the given generation maps
		static ptr<expr> make(ptr<expr> a, ptr<expr> b, 
		                      gen_map ag, gen_map bg, gen_type gm);
		/// Make an expression using the default generation rules
		static ptr<expr> make(const expr_list& es);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char) const;
		virtual gen_set   match_set() const;
		virtual gen_set   back_set()  const;
		virtual expr_type type()      const { return alt_type; }
		
		alt_list  es;  ///< List of subexpressions
		gen_type  gm;  ///< Maximum generation
	}; // class ualt_expr
	
	/// A parsing expression representing the concatenation of two parsing expressions
	class seq_expr : public memo_expr {
	public:
		struct look_node {
			look_node(gen_type g, gen_map eg, ptr<expr> e, gen_type gl = 0) 
				: g(g), eg(eg), e(e), gl(gl) {}
			
			gen_type  g;   ///< Backtrack generation this follower corresponds to
			gen_map   eg;  ///< Map of generations from this node to the containing node
			ptr<expr> e;   ///< Follower expression for this lookahead generation
			gen_type  gl;  ///< Generation of last match
		}; // struct look_node
		using look_list = std::vector<look_node>;
		
		seq_expr(ptr<expr> a, ptr<expr> b)
			: memo_expr(), a(a), b(b), bs(), c(fail_expr::make()), cg(gen_map{0}), gm(0) {}
		
		seq_expr(ptr<expr> a, ptr<expr> b, look_list bs, ptr<expr> c, gen_map cg, gen_type gm)
			: memo_expr(), a(a), b(b), bs(bs), c(c), cg(cg), gm(gm) {}
	
		static ptr<expr> make(ptr<expr> a, ptr<expr> b);
		void accept(visitor* v) { v->visit(*this); }
		
		virtual ptr<expr> deriv(char) const;
		virtual gen_set   match_set() const;
		virtual gen_set   back_set()  const;
		virtual expr_type type()      const { return seq_type; }
		
		ptr<expr> a;   ///< First subexpression
		ptr<expr> b;   ///< Gen-zero follower
		look_list bs;  ///< List of following subexpressions for each backtrack generation
		ptr<expr> c;   ///< Matching backtrack value
		gen_map   cg;  ///< Backtrack map for c
		gen_type  gm;  ///< Maximum backtrack generation
	}; // class seq_expr
	
} // namespace derivs

