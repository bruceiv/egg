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

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "utils/flagvector.hpp"

/**
 * Implements dynamic left-factoring parsing for parsing expression grammars, according to the 
 * algorithm described by Aaron Moss in 2014 (http://arxiv.org/abs/1405.4841).
 * 
 * The basic idea of this parsing algorithm is to repeatedly take the "derivative" of a parsing 
 * expression with respect to the next character in the input sequence, where the derivative 
 * is a parsing expression which matches the suffixes of all strings in the language of the 
 * original expression which start with the given prefix.
 */
namespace dlf {
	/// Shorthand for shared_ptr
	template <typename T>
	using ptr = std::shared_ptr<T>;
	
	/// Abbreviates std::make_shared
	template<typename T, typename... Args>
	ptr<T> make_ptr(Args&&... args) { 
		return std::make_shared<T>(std::forward<Args>(args)...);
	}
	
	/// Abreviates std::static_pointer_cast
	template<typename T, typename U>
	ptr<T> as_ptr(const ptr<U>& p) { return std::static_pointer_cast<T>(p); }
	
	/// Different restriction states
	enum restriction {
		unknown,   ///< Unresolved restrictions
		allowed,   ///< No restrictions
		forbidden  ///< Enforced restrictions
	};
	
	/// Manages restrictions on expression matches
	class restriction_mgr {
	friend class restriction_ck;
		struct blocker {
			blocker() = default;
			blocker(flags::vector& blocking) : released{false}, blocking{blocking} {}
			blocker(bool released, flags::vector& blocking) 
				: released{released}, blocking{blocking} {}
			
			bool released;
			flags::vector blocking;
		};
		
		/// Check for newly enforced rules after unenforceable has been changed; returns if there 
		/// are any newly enforced rules
		bool check_enforced();
		/// Check for newly unenforceable rules after enforced has changed; returns if there are 
		/// any newly unenforceable rules
		bool check_unenforceable();
	public:
		restriction_mgr() : enforced{}, unenforceable{}, pending{}, update{0}, next{0} {}
		
		/// Reserve n consecutive restrictions; returns the first index
		flags::index reserve(flags::index n);
		
		/// Enforce a restriction, unless one of the restrictions is fired
		void enforce_unless(flags::index i, const flags::vector& blocking);
		
		/// A restriction will not be enforced any more
		void release(flags::index i);
		
		flags::vector enforced;       ///< set of enforced restrictions
		flags::vector unenforceable;  ///< set of unenforceable restrictions
	private:
		/// restrictions that we haven't decided about enforcing
		std::unordered_map<flags::index, 
		                   blocker> pending;
		unsigned long update;     ///< index of last update
		flags::index next;        ///< next available restriction
	};  // restriction_mgr
	
	/// Determines whether a node is prevented from matching
	class restriction_ck {
	friend restriction_mgr;
	public:
		restriction_ck(restriction_mgr& mgr, flags::vector&& restricted = flags::vector{}) 
			: mgr{mgr}, restricted{restricted}, update{mgr.update},  
			  state{restricted.empty() ? allowed : unknown} {}
		
		/// Check if a restriction is enforced
		restriction check();
		
		/// Add a new set of restrictions
		void join(const restriction_ck& o);
		
		/// Intersect a new set of restrictions
		void refine(const restriction_ck& o);
		
		flags::vector restricted;  ///< set of restrictions on matches
	private:
		restriction_mgr& mgr;      ///< restriction manager
		unsigned long update;      ///< last update seen
		restriction state;         ///< saved restriction state
	};  // restriction_ck
	
	/// Forward declarations of expression node types
	class match_node;
	class fail_node;
	class inf_node;
	class end_node;
	class char_node;
	class range_node;
	class any_node;
	class str_node;
	class rule_node;
	class alt_node;
	class cut_node;
	
	/// Type of expression node
	enum node_type : std::size_t {
		match_type = 0x0,
		fail_type  = 0x1,
		inf_type   = 0x2,
		end_type   = 0x3,
		char_type  = 0x4,
		range_type = 0x5,
		any_type   = 0x6,
		str_type   = 0x7,
		rule_type  = 0x8,
		alt_type   = 0x9,
		cut_type   = 0xA
	};
	
	/// Tags `x` with the given node type; useful for hashing
	inline constexpr std::size_t tag_with(node_type ty, std::size_t x = 0x0) {
		return (x << 4) | static_cast<std::size_t>(ty);
	}
	
	std::ostream& operator<< (std::ostream& out, node_type t);
	
	/// Abstract base class of all expression visitors
	class visitor {
	public:
		virtual void visit(match_node&) = 0;
		virtual void visit(fail_node&)  = 0;
		virtual void visit(inf_node&)   = 0;
		virtual void visit(end_node&)   = 0;
		virtual void visit(char_node&)  = 0;
		virtual void visit(range_node&) = 0;
		virtual void visit(any_node&)   = 0;
		virtual void visit(str_node&)   = 0;
		virtual void visit(rule_node&)  = 0;
		virtual void visit(alt_node&)   = 0;
		virtual void visit(cut_node&)   = 0;
	}; // visitor
	
	struct arc;
	
	/// Abstract base class for expression nodes
	class node {
	protected:
		node() = default;
		
	public:
		virtual ~node() = default;
		
		/// Abreviates std::make_shared for expression nodes
		template<typename T, typename... Args>
		static ptr<node> make(Args&&... args) {
			return as_ptr<node>(make_ptr<T>(std::forward<Args>(args)...));
		}
		
		/// Accept visitor
		virtual void accept(visitor*) = 0;
		
		/// Derivative of this expression (pointed to by arc `in`) with respect to x.
		/// Returns true for unrestricted match
		virtual bool d(char x, arc& in) = 0;
		
		/// Expression node type
		virtual node_type type() const = 0;
		
		/// Polymorphic hash function; doesn't account for successor nodes
		virtual std::size_t hash() const = 0;
		
		/// Polymorphic equality function; doesn't account for successor nodes
		virtual bool equiv(ptr<node> o) const = 0;
	};  // node
	
	/// Directed arc linking two nodes
	struct arc {
		arc() = default;
		arc(ptr<node> succ, restriction_ck blocking) : succ{succ}, blocking{blocking} {}
		
		/// Returns true and repoints this arc to a fail_node if one of the blocking restrictions 
		/// is enforced
		bool blocked();
		
		/// Joins to an outgoing arc. Returns true if now an unrestricted match (as in node::d()).
		bool join(arc& out);
		
		/// Joins to a fail_node. Returns false.
		bool fail();
			
		ptr<node> succ;           ///< sucessor pointer
		restriction_ck blocking;  ///< restrictions blocking this arc
	};  // arc
	
	/// Visitor with function-like interface for counting restrictions in an expression
	class count_restrict : public visitor {
		/// Common code for visitor pattern
		void visit(ptr<node> np);
	public:
		count_restrict(ptr<node> np) : nRestrict{0}, visited{} { visit(np); }
		
		operator flags::index () { return nRestrict; }
		
		virtual void visit(match_node&);
		virtual void visit(fail_node&);
		virtual void visit(inf_node&);
		virtual void visit(end_node&);
		virtual void visit(char_node& n);
		virtual void visit(range_node& n);
		virtual void visit(any_node& n);
		virtual void visit(str_node& n);
		virtual void visit(rule_node& n);
		virtual void visit(alt_node& n);
		virtual void visit(cut_node& n);
	private:
		flags::index nRestrict;                 ///< restriction count
		std::unordered_set<ptr<node>> visited;  ///< Nodes already seen
	}; // count_restrict
	
	/// Nonterminal substitution
	struct nonterminal {
		nonterminal(const std::string& name, ptr<node> begin)
		: name{name}, begin{begin}, nRestrict{count_restrict(begin)}, inDeriv{false} {}
		
		const std::string name;        ///< Name of the non-terminal
		const ptr<node> begin;         ///< First subexpression in the non-terminal
		const flags::index nRestrict;  ///< Count of restrictions in this non-terminal
		bool inDeriv;                  ///< Flag for detecting infinite loops
	};  // nonterminal
	
	/// Visitor with function-like interface for cloning a contained expression
	class clone : public visitor {
		/// Common code for visiting an arc
		arc visit(arc& a);
		
		/// Functional interface to visitor pattern
		ptr<node> visit(ptr<node> np);
	public:
		clone(nonterminal& nt, ptr<node> out, restriction_mgr& mgr) 
		: rVal{}, out{out}, mgr{mgr}, visited{} {
			nShift = mgr.reserve(nt.nRestrict);
			visit(nt.begin);
		}
		
		operator ptr<node> () { return rVal; }
		
		virtual void visit(match_node&);
		virtual void visit(fail_node&);
		virtual void visit(inf_node&);
		virtual void visit(end_node&);
		virtual void visit(char_node& n);
		virtual void visit(range_node& n);
		virtual void visit(any_node& n);
		virtual void visit(str_node& n);
		virtual void visit(rule_node& n);
		virtual void visit(alt_node& n);
		virtual void visit(cut_node& n);
		
	private:
		ptr<node> pVal;        ///< Parameter of last visit
		ptr<node> rVal;        ///< Return value of last visit
		ptr<node> out;         ///< Replacement for endNode
		restriction_mgr& mgr;  ///< Restriction manager
		flags::index nShift;   ///< Amount to shift restrictions by
		
		/// Memoizes visited nodes (to ensure singleton nodes remain singletons).
		std::unordered_map<ptr<node>, ptr<node>> visited;
	}; // clone_visitor
	
	/// Terminal node representing a match
	class match_node : public node {
		// disallow copying
		match_node(const match_node&) = delete;
		match_node(match_node&&) = delete;
		match_node& operator= (const match_node&) = delete;
		match_node& operator= (match_node&&) = delete;
	public:
		match_node(bool& reacheable) : reachable{reachable} {}
		virtual ~match_node() { reachable = false; };
		
		static  ptr<node>   make(bool& reachable);
		virtual void        accept(visitor* v) { v->visit(*this); }
		virtual bool        d(char, arc&);
		virtual node_type   type() const { return match_type; }
		virtual std::size_t hash() const;
		virtual bool        equiv(ptr<node>) const;
	
	private:
		bool& reachable;  ///< Reachability flag, sets false in destructor
	}; // match_node
	
	/// Terminal node representing a failure
	class fail_node : public node {
		// Implements singleton pattern; access through make()
		fail_node() = default;
		fail_node(const fail_node&) = delete;
		fail_node(fail_node&&) = delete;
		fail_node& operator= (const fail_node&) = delete;
		fail_node& operator= (fail_node&&) = delete;	
	public:
		virtual ~fail_node() = default;
		
		static  ptr<node>   make();
		virtual void        accept(visitor* v) { v->visit(*this); }
		virtual bool        d(char, arc&);
		virtual node_type   type() const { return fail_type; }
		virtual std::size_t hash() const;
		virtual bool        equiv(ptr<node>) const;
	}; // fail_node
	
	/// Terminal node representing an infinite loop
	class inf_node : public node {
		// Implements singleton pattern; access through make()
		inf_node() = default;
		inf_node(const inf_node&) = delete;
		inf_node(inf_node&&) = delete;
		inf_node& operator= (const inf_node&) = delete;
		inf_node& operator= (inf_node&&) = delete;	
	public:
		virtual ~inf_node() = default;
		
		static  ptr<node>   make();
		virtual void        accept(visitor* v) { v->visit(*this); }
		virtual bool        d(char, arc&);
		virtual node_type   type() const { return inf_type; }
		virtual std::size_t hash() const;
		virtual bool        equiv(ptr<node>) const;
	}; // inf_node
	
	/// Placeholder node for the end of a subexpression
	class end_node : public node {
		end_node() = default;
		virtual ~end_node() = default;
		
		static  ptr<node>   make();
		virtual void        accept(visitor* v) { v->visit(*this); }
		virtual bool        d(char, arc&);
		virtual node_type   type() const { return end_type; }
		virtual std::size_t hash() const;
		virtual bool        equiv(ptr<node>) const;
	}; // end_node
	
	/// Node representing a character literal
	class char_node : public node {
		char_node(const arc& out, char c) : out{out}, c{c} {}
		virtual ~char_node() = default;
		
		static  ptr<node>   make(const arc& out, char c);
		virtual void        accept(visitor* v) { v->visit(*this); }
		virtual bool        d(char, arc&);
		virtual node_type   type() const { return char_type; }
		virtual std::size_t hash() const;
		virtual bool        equiv(ptr<node>) const;
		
		arc out;  ///< Successor node
		char c;   ///< Character represented by the expression
	}; // char_node
	
	/// Node representing a character range literal
	class range_node : public node {
		range_node(const arc& out, char b, char e) : out{out}, b{b}, e{e} {}
		virtual ~range_node() = default;
		
		static  ptr<node>   make(const arc& out, char b, char c);
		virtual void        accept(visitor* v) { v->visit(*this); }
		virtual bool        d(char, arc&);
		virtual node_type   type() const { return range_type; }
		virtual std::size_t hash() const;
		virtual bool        equiv(ptr<node>) const;
		
		arc out;  ///< Successor node
		char b;   ///< First character in expression range 
		char e;   ///< Last character in expression range
	}; // range_node
	
	/// Node representing an "any character" literal
	class any_node : public node {
		any_node(const arc& out) : out{out} {}
		virtual ~any_node() = default;
		
		static  ptr<node>   make(const arc& out);
		virtual void        accept(visitor* v) { v->visit(*this); }
		virtual bool        d(char, arc&);
		virtual node_type   type() const { return any_type; }
		virtual std::size_t hash() const;
		virtual bool        equiv(ptr<node>) const;
		
		arc out;  ///< Successor node
	}; // any_node
	
	/// Node representing a string literal
	class str_node : public node {
		str_node(const arc& out, ptr<std::string> sp, std::string::size_type i) 
			: out{out}, sp{sp}, i{i} {}
	public:
		str_node(const arc& out, const std::string& s) 
			: out{out}, sp{make_ptr<std::string>(s)}, i{0} {}
		str_node(const arc& out, std::string&& s) 
			: out{out}, sp{make_ptr<std::string>(s)}, i{0} {}
		str_node(const arc& out, const str_node& o) : out{out}, sp{o.sp}, i{o.i} {}
		
		static  ptr<node>   make(const arc& out, const std::string& s);
		virtual void        accept(visitor* v) { v->visit(*this); }
		virtual bool        d(char, arc&);
		virtual node_type   type() const { return str_type; }
		virtual std::size_t hash() const;
		virtual bool        equiv(ptr<node>) const;
		
		std::string str() const { return std::string{*sp, i}; }
		std::string::size_type size() const { return sp->size() - i; }
		
		arc out;  ///< Successor node
	private:
		ptr<std::string> sp;       ///< Pointer to the interred string
		std::string::size_type i;  ///< Index into the interred string
	}; // str_node
	
	/// Node representing a non-terminal
	class rule_node : public node {
	public:	
		rule_node(const arc& out, ptr<nonterminal> r, restriction_mgr& mgr) 
			: out{out}, r{r}, mgr{mgr} {}
		virtual ~rule_node() = default;
		
		static  ptr<node>   make(const arc& out, ptr<nonterminal> r);
		virtual void        accept(visitor* v) { v->visit(*this); }
		virtual bool        d(char, arc&);
		virtual node_type   type() const { return rule_type; }
		virtual std::size_t hash() const;
		virtual bool        equiv(ptr<node>) const;
		
		arc out;               ///< Successor node
		ptr<nonterminal> r;    ///< Pointer to shared rule definition
		restriction_mgr& mgr;  ///< Restriction manager
	}; // rule_node
	
	/// Node containing a number of subexpressions to parse concurrently
	class alt_node : public node {
		/// Hashes arcs by their pointed-to nodes
		struct succ_hash {
			std::size_t operator() (const arc& a) const { return a.succ->hash(); }
		};
		
		/// Puts arcs into equivalence classes by the structure of their nodes
		struct succ_equiv {
			bool operator() (const arc& a, const arc& b) const { return a.succ->equiv(b.succ); }
		};
		
		/// Set of outgoing arcs
		using arc_set = std::unordered_set<arc, succ_hash, succ_equiv>;
		
		/// Merges `add` into existing `ex`, pushing the alternation before the successors
		template<typename NT>
		void merge(arc& ex, arc& add) {
			arc& ex_out  = as_ptr<NT>(ex.succ)->out;          // Get successor from common type
			arc& add_out = as_ptr<NT>(add.succ)->out;         // Get successor from added node
			ex_out.blocking.join(ex.blocking);                // add existing blockers to successor
			add_out.blocking.join(add.blocking);              // add new blockers to new successor
			ex.blocking.refine(add.blocking);                 // intersect blockers
			ex_out = arc{alt_node::make({ex_out, add_out})};  // join new node
		}
		
		/// Merges an arc `a` into the set (flattening alternations and merging equivalent nodes).
		/// Returns a pair containing the iterator to the merged item and a boolean value that will 
		/// be true if the merged item is an unrestricted match
		std::pair<arc_set::iterator, bool> merge(arc& a);
	public:
		alt_node() = default;
		virtual ~alt_node() = default;
		
		static  ptr<node>   make(std::initializer_list<arc> out);
		virtual void        accept(visitor* v) { v->visit(*this); }
		virtual bool        d(char, arc&);
		virtual node_type   type() const { return alt_type; }
		virtual std::size_t hash() const;
		virtual bool        equiv(ptr<node>) const;
		
		/// Set of outward arcs, with a structural equivalence relation on their pointed-to nodes
		arc_set out;
	}; // alt_node
	
	/// Node which inserts values into the restriction set
	class cut_node : public node {
		// disallow copying
		cut_node(const cut_node&) = delete;
		cut_node(cut_node&&) = delete;
		cut_node& operator= (const cut_node&) = delete;
		cut_node& operator= (cut_node&&) = delete;
	public:
		cut_node(const arc& out, flags::index i, restriction_mgr& mgr) 
			: out{out}, i{i}, mgr{mgr} {}
		/// Releases restriction upon no longer reachable
		virtual ~cut_node() { mgr.release(i); };
		
		static  ptr<node>   make(const arc& out, flags::index i, restriction_mgr& mgr);
		virtual void        accept(visitor* v) { v->visit(*this); }
		virtual bool        d(char, arc&);
		virtual node_type   type() const { return cut_type; }
		virtual std::size_t hash() const;
		virtual bool        equiv(ptr<node>) const;
		
		arc out;               ///< Successor node
		flags::index i;        ///< Restriction to fire
		restriction_mgr& mgr;  ///< Restriction manager
	}; // cut_node
}
