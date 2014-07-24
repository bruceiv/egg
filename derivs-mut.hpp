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
#include <cstdlib> // for std::size_t
#include <forward_list>
#include <string>
#include <typeinfo>
#include <unordered_set>
#include <utility>
#include <vector>

#include "utils/uint_pfn.hpp"

#include <iostream>

/**
 * Implements derivative parsing for parsing expression grammars, according to the algorithm 
 * described by Aaron Moss in 2014 (http://arxiv.org/abs/1405.4841).
 * 
 * The basic idea of this derivative parsing algorithm is to repeatedly take the "derivative" of a 
 * parsing expression with respect to the next character in the input sequence, where the 
 * derivative is a parsing expression which matches the suffixes of all strings in the language of 
 * the original expression which start with the given prefix. This implementation diverges from the 
 * functional presentation of the paper for performance reasons.
 */
namespace derivs {
	
	class expr;
	/// Set of expressions
	using expr_set = std::unordered_set<expr*>;
	/// map of backtrack generations
	using gen_map  = utils::uint_pfn;
	/// set of backtrack generations
	using gen_set  = gen_map::set_type;
	/// single backtrack generation
	using gen_type = gen_map::value_type;
	/// Index in string
	using ind      = std::size_t;
	
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
	
	// Forward declarations of node types
	class node;
	class fail_node;
	class inf_node;
	class eps_node;
	class look_node;
	class char_node;
	class range_node;
	class any_node;
	class str_node;
	class shared_node;
	class rule_node;
	class not_node;
	class map_node;
	class alt_node;
	class seq_node;
	
	gen_map new_back_map(const expr& e, gen_type gm, bool& did_inc, ind i = 0);
	gen_map new_back_map(const expr& e, gen_type& gm, ind i = 0);
	gen_map default_back_map(const expr& e, bool& did_inc, ind i = 0);
	void update_back_map(gen_map& eg, gen_type ebm, const expr& de, gen_type& gm, ind i);
	void update_back_map(gen_map& eg, gen_type ebm, const expr& de, 
	                     gen_type gm, bool& did_inc, ind i);
	
	/// Abstract base class of all derivative visitors
	class visitor {
	public:
		virtual void visit(fail_node&)   = 0;
		virtual void visit(inf_node&)    = 0;
		virtual void visit(eps_node&)    = 0;
		virtual void visit(look_node&)   = 0;
		virtual void visit(char_node&)   = 0;
		virtual void visit(range_node&)  = 0;
		virtual void visit(any_node&)    = 0;
		virtual void visit(str_node&)    = 0;
		virtual void visit(shared_node&) = 0;
		virtual void visit(rule_node&)   = 0;
		virtual void visit(not_node&)    = 0;
		virtual void visit(map_node&)    = 0;
		virtual void visit(alt_node&)    = 0;
		virtual void visit(seq_node&)    = 0;
	}; // class visitor
	
	/// Abstract base class for expression nodes
	class node {
	public:
		virtual ~node() = default;
		
		/// Performs a deep clone of this node
		virtual expr clone(ind) const = 0;
		
		/// Normalizes this node; mutates self
		virtual void normalize(expr& self, expr_set& normed) = 0;
		
		/// Accept visitor
		virtual void accept(visitor*) = 0;
		
		/// Takes the derivative of this expression with respect to x at i; mutating self.
		virtual void  d(expr& self, char x, ind i) = 0;
		
		/// At what backtracking generations does this expression match?
		virtual gen_set match(ind) const = 0;
		
		/// What backtracking generations does this expression expose?
		virtual gen_set back(ind) const = 0;
				
		/// Expression node type
		/// Shared_nodes return their subexpression, thus NOT safe for typecast.
		virtual expr_type type() const = 0;
		
	}; // class node
	
	/// A derivative expression
	class expr {
		expr(node* n) : n(n) {}
		
	public:
		expr() : n(nullptr) {}
		expr(expr&& t) : n(t.n) { t.n = nullptr; /* prevent temporary from deleting node */ }
		
		expr& operator= (expr&& t) {
			node* tmp = n;  // backup n so that we can defer destruction
			n = t.n;        // set new node
			t.n = nullptr;  // prevent t from deleting our node
			delete tmp;     // delete old node (if recursive should have broken cycle with previous line)
			return *this;
		}
		
		/// Destroys expression and contained node
		~expr() { delete n; }
		
		/// Makes an expression for a given node.
		template<typename T, typename... Args>
		static expr make(Args&&... args) {
			return expr{new T(std::forward<Args>(args)...)};
		}
		
		/// Performs a deep clone of this expression
		inline expr clone(ind i = 0) const { return n->clone(i); }
		
		/// Swaps the node for a new one
		template<typename T, typename... Args>
		expr& remake(Args&&... args) {
			node* tmp = n;                          // backup n so that we can defer destruction
			n = new T(std::forward<Args>(args)...); // set new node
			delete tmp;                             // delete old node
			return *this;
		}
		
		/// Resets the expression to contain a new value (default nothing)
		inline void reset(node* m = nullptr) {
			if ( n == m ) return;
			node* tmp = n;
			n = m;
			delete tmp;
		}
		
		/// Normalizes an existing node.
		/// Should only be called as part of the construction process.
		inline void normalize(expr_set& normed) { n->normalize(*this, normed); }
		
		/// Accept visitor
		inline void accept(visitor* v) { n->accept(v); };
				
		/// Takes the derivative of this expression with respect to x (at i)
		inline void  d(char x, ind i) { return n->d(*this, x, i); }
		
		/// At what backtracking generations does this expression match?
		inline gen_set match(ind i = 0) const { return n->match(i); }
		
		/// What backtracking generations does this expression expose?
		inline gen_set back(ind i = 0) const { return n->back(i); }
		
		/// Expression node type. 
		/// Shared_nodes return their subexpression, thus NOT safe for typecast.
		inline expr_type type() const { return n->type(); }
		
		/// Returns the contained node.
		inline node* const get() { return n; }
		
	private:
		node* n;  ///< Contained node; owned by and destructed by expr
	}; // class expr
	
	/// Caches results of possibly expensive match() and back() computations
	class node_cache {
	public:
		node_cache() { flags = {false, false}; }
		node_cache(const node_cache& o) : flags(o.flags) {
			if ( flags.match ) match = o.match;
			if ( flags.back ) back = o.back;
		}
		
		inline void set_match(const gen_set& m) const {
			flags.match = true;
			match = m;
		}
		
		inline void set_back(const gen_set& b) const {
			flags.back = true;
			back = b;
		}
		
		inline void invalidate() const { flags = {false, false}; }
		
		mutable gen_set match;  ///< Stored match set
		mutable gen_set back;   ///< Stored backtracking set
		mutable struct {
			bool match : 1;  ///< Is there a match set stored?
			bool back  : 1;  ///< Is there a backtrack set stored?
		} flags;        ///< Memoization status flags
	}; // class node_cache
	
	/// A failure node
	class fail_node : public node {
	public:
		fail_node() = default;
		
		static expr make();
		virtual expr clone(ind) const;
		virtual void normalize(expr&, expr_set&);
		virtual void accept(visitor* v) { v->visit(*this); }
		
		virtual void      d(expr&, char, ind);
		virtual gen_set   match(ind) const;
		virtual gen_set   back(ind)  const;
		virtual expr_type type()     const { return fail_type; }
	}; // class fail_node
	
	/// An infinite loop failure node
	class inf_node : public node {
	public:
		inf_node() = default;
		
		static expr make();
		virtual expr clone(ind) const;
		virtual void normalize(expr&, expr_set&);
		virtual void accept(visitor* v) { v->visit(*this); }
		
		virtual void      d(expr&, char, ind);
		virtual gen_set   match(ind) const;
		virtual gen_set   back(ind)  const;
		virtual expr_type type()     const { return inf_type; }
	}; // class inf_node
	
	/// An empty success node
	class eps_node : public node {
	public:
		eps_node() = default;
		
		static expr make();
		virtual expr clone(ind) const;
		virtual void normalize(expr&, expr_set&);
		virtual void accept(visitor* v) { v->visit(*this); }
		
		virtual void      d(expr&, char, ind);
		virtual gen_set   match(ind) const;
		virtual gen_set   back(ind)  const;
		virtual expr_type type()     const { return eps_type; }
	}; // class eps_node
	
	/// An lookahead success node
	class look_node : public node {
	public:
		look_node(gen_type g = 1) : b{g} {}
		
		static expr make(gen_type g = 1);
		virtual expr clone(ind) const;
		virtual void normalize(expr&, expr_set&);
		virtual void accept(visitor* v) { v->visit(*this); }
		
		virtual void      d(expr&, char, ind);
		virtual gen_set   match(ind) const;
		virtual gen_set   back(ind)  const;
		virtual expr_type type()     const { return look_type; }
		
		gen_type b;  ///< Generation of this success match
	}; // class look_node
	
	/// A single-character node
	class char_node : public node {
	public:
		char_node(char c) : c(c) {}
		
		static expr make(char c);
		virtual expr clone(ind) const;
		virtual void normalize(expr&, expr_set&);
		virtual void accept(visitor* v) { v->visit(*this); }
		
		virtual void      d(expr&, char, ind);
		virtual gen_set   match(ind) const;
		virtual gen_set   back(ind)  const;
		virtual expr_type type()     const { return char_type; }
		
		char c; ///< Character represented
	}; // class char_node
	
	/// A character range node
	class range_node : public node {
	public:
		range_node(char b, char e) : b(b), e(e) {}
		
		static expr make(char b, char e);
		virtual expr clone(ind) const;
		virtual void normalize(expr&, expr_set&);
		virtual void accept(visitor* v) { v->visit(*this); }
		
		virtual void      d(expr&, char, ind);
		virtual gen_set   match(ind) const;
		virtual gen_set   back(ind)  const;
		virtual expr_type type()     const { return range_type; }
		
		char b;  ///< First character in range 
		char e;  ///< Last character in range
	}; // class range_node
	
	/// A node which matches any character
	class any_node : public node {
	public:
		any_node() = default;
		
		static expr make();
		virtual expr clone(ind) const;
		virtual void normalize(expr&, expr_set&);
		virtual void accept(visitor* v) { v->visit(*this); }
		
		virtual void      d(expr&, char, ind);
		virtual gen_set   match(ind) const;
		virtual gen_set   back(ind)  const;
		virtual expr_type type()     const { return any_type; }
	}; // class any_node
	
	/// A node which matches a string
	class str_node : public node {
	public:
		str_node(const std::string& s) : s(s.rbegin(), s.rend()) {}
		str_node(const str_node& o) = default;
		
		static expr make(const std::string& s);
		virtual expr clone(ind) const;
		virtual void normalize(expr&, expr_set&);
		virtual void accept(visitor* v) { v->visit(*this); }
		
		virtual void      d(expr&, char, ind);
		virtual gen_set   match(ind) const;
		virtual gen_set   back(ind)  const;
		virtual expr_type type()     const { return str_type; }
		
		inline std::string str() const { return std::string{s.rbegin(), s.rend()}; }
		inline unsigned long size() const { return s.size(); }
		
	private:
		std::vector<char> s;  ///< stores the string in reverse order for efficient popping
	}; // class str_node
	
	/// Shares an expression so that it can be referenced in multiple places.
	class shared_node : public node {
	public:
		struct impl {
			impl(expr&& e, ind crnt = 0, node_cache crnt_cache = node_cache{}, 
			     node_cache prev_cache = node_cache{}) 
				: e(std::move(e)), crnt(crnt), refs(1), 
				  crnt_cache(crnt_cache), prev_cache(prev_cache), dirty(false) {}
			
			expr e;                 ///< Shared expression
			ind crnt;               ///< Index of the current state of the expression
			ind refs;               ///< Ref-count
			node_cache crnt_cache;  ///< Cache of current values of back and match
			node_cache prev_cache;  ///< Cache of previous values of back and match
			bool dirty;             ///< One bit of shared state for clients
		}; // struct impl
		impl* shared;  ///< Pointer to this node's shared block
			
		shared_node(expr&& e, ind crnt = 0, node_cache crnt_cache = node_cache{},
		            node_cache prev_cache = node_cache{}) 
			: shared(new impl(std::move(e), crnt, crnt_cache, prev_cache)) {}
		
		shared_node(const shared_node& o) : shared(o.shared) { ++(shared->refs); }
		
		shared_node& operator= (const shared_node& o) {
			if ( shared == o.shared ) return *this;
			if ( --(shared->refs) == 0 ) { delete shared; }
			shared = o.shared;
			++(shared->refs);
			return *this;
		}
		
		~shared_node() { if ( --(shared->refs) == 0 ) { delete shared; } }
		
		static expr make(expr&& e, ind crnt = 0);
		virtual expr clone(ind) const;
		/// Special overload that doesn't need a containing expression
		void normalize(expr_set&);
		virtual void normalize(expr&, expr_set& normed) { normalize(normed); }
		virtual void accept(visitor* v) { v->visit(*this); }
		
		virtual void      d(expr&, char, ind);
		virtual gen_set   match(ind) const;
		virtual gen_set   back(ind)  const;
		/// NOTE: Assumes this will never be called for the previous value after a derivative taken
		virtual expr_type type()     const { return shared->e.type(); }
		
		/// Returns a view of the contained pointer
		inline expr* const get() const { return &(shared->e); }
	}; // class shared_node
	
	/// A non-terminal node
	class rule_node : public node {
	public:
		rule_node(expr&& e) : r(std::move(e)) {}
		rule_node(const rule_node& n) = default;
		rule_node(const shared_node& r) : r(r) {}
		
		rule_node& operator= (const rule_node& n) = default;
		
		virtual expr clone(ind) const;
		virtual void normalize(expr&, expr_set&);
		virtual void accept(visitor* v) { v->visit(*this); }
		
		virtual void      d(expr&, char, ind);
		virtual gen_set   match(ind) const;
		virtual gen_set   back(ind)  const;
		virtual expr_type type()     const { return rule_type; }
		
		shared_node r;     ///< Rule expansion
		node_cache cache;  ///< Cache for back and match
	}; // class rule_node
	
	/// A negative lookahead node
	class not_node : public node {
	public:
		not_node(expr&& e) : e(std::move(e)) {}
		
		static expr make(expr&& e, ind i = 0);
		virtual expr clone(ind) const;
		virtual void normalize(expr&, expr_set&);
		virtual void accept(visitor* v) { v->visit(*this); }
		
		virtual void      d(expr&, char, ind);
		virtual gen_set   match(ind) const;
		virtual gen_set   back(ind)  const;
		virtual expr_type type()     const { return not_type; }
		
		expr e;  ///< Subexpression to negatively match
	}; // class not_node

	/// Maintains generation mapping from collapsed alternation expression	
	class map_node : public node {
	public:
		map_node(expr&& e, const gen_map& eg, gen_type gm, 
		         const node_cache& cache = node_cache{}) 
			: e(std::move(e)), eg(eg), gm(gm), cache(cache) {}
		
		static expr make(expr&& e, const gen_map& eg, gen_type gm, ind i);
		virtual expr clone(ind) const;
		virtual void normalize(expr&, expr_set&);
		virtual void accept(visitor* v) { v->visit(*this); }
		
		virtual void      d(expr&, char, ind);
		virtual gen_set   match(ind) const;
		virtual gen_set   back(ind)  const;
		virtual expr_type type()     const { return map_type; }
		
		expr       e;      ///< Subexpression to map
		gen_map    eg;     ///< Generation flags for subexpression
		gen_type   gm;     ///< Maximum generation from source expresssion
		node_cache cache;  ///< Caches match and back 
	}; // class map_node
	
	/// An ordered choice between two expressions
	class alt_node : public node {
	public:
		alt_node(expr&& a, expr&& b, 
		         const gen_map& ag = gen_map{0}, const gen_map& bg = gen_map{0}, 
		         gen_type gm = 0, const node_cache& cache = node_cache{}) 
			: a(std::move(a)), b(std::move(b)), ag(ag), bg(bg), 
			  gm(gm), cache(cache) {}
		
		static expr make(expr&& a, expr&& b);
		static expr make(expr&& a, expr&& b, const gen_map& ag, const gen_map& bg, 
		                 gen_type gm, ind i = 0);
		virtual expr clone(ind) const;
		virtual void normalize(expr&, expr_set&);
		virtual void accept(visitor* v) { v->visit(*this); }
		
		virtual void      d(expr&, char, ind);
		virtual gen_set   match(ind) const;
		virtual gen_set   back(ind)  const;
		virtual expr_type type()     const { return alt_type; }
		
		expr       a;      ///< First alternative
		expr       b;      ///< Second alternative
		gen_map    ag;     ///< Generation flags for first alternative
		gen_map    bg;     ///< Generation flags for second alternative
		gen_type   gm;     ///< Maximum generation from source expresssion
		node_cache cache;  ///< Caches match and back
	}; // class alt_node
	
	/// A sequence of two expressions
	class seq_node : public node {
	public:
		struct look_back {
			look_back(gen_type g, expr&& e, gen_map eg, gen_type gl = 0) 
				: g(g), e(std::move(e)), eg(eg), gl(gl) {}
			
			gen_type  g;   ///< Backtrack generation this follower corresponds to
			expr      e;   ///< Follower expression for this lookahead generation
			gen_map   eg;  ///< Map of generations from this node to the containing node
			gen_type  gl;  ///< Generation of last match
		}; // struct look_list
		using look_list = std::forward_list<look_back>;
		
		seq_node(expr&& a, expr&& b)
			: a(std::move(a)), b(std::move(b)), bs(), 
			  c(fail_node::make()), cg(gen_map{0}), 
			  gm(0), cache(node_cache{}) {}
		
		seq_node(expr&& a, expr&& b, look_list&& bs, expr&& c, const gen_map& cg, 
		         gen_type gm, const node_cache& cache = node_cache{})
			: a(std::move(a)), b(std::move(b)), bs(std::move(bs)), 
			  c(std::move(c)), cg(cg), gm(gm), cache(cache) {}
		
		static expr make(expr&& a, expr&& b);
		virtual expr clone(ind) const;
		virtual void normalize(expr&, expr_set&);
		virtual void accept(visitor* v) { v->visit(*this); }
		
		virtual void      d(expr&, char, ind);
		virtual gen_set   match(ind) const;
		virtual gen_set   back(ind)  const;
		virtual expr_type type()     const { return seq_type; }
		
		expr       a;      ///< First subexpression
		expr       b;      ///< Gen-zero follower
		look_list  bs;     ///< List of following subexpressions for each backtrack generation
		expr       c;      ///< Matching backtrack value
		gen_map    cg;     ///< Backtrack map for c
		gen_type   gm;     ///< Maximum backtrack generation
		node_cache cache;  ///< Caches match and back
	}; // class seq_node
	
} // namespace derivs
