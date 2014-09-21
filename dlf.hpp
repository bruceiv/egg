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
#include <unordered_map>
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
			blocker(bool released, flags::vector& blocking) : released{released}, blocking{blocking} {}
			
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
	};  // class restriction_mgr
	
	/// Determines whether a node is prevented from matching
	class restriction_ck {
	friend restriction_mgr;
	public:
		restriction_ck(restriction_mgr& mgr, flags::vector&& restricted = flags::vector{}) 
			: mgr{mgr}, restricted{restricted}, update{mgr.update}, 
			  state{restricted.empty() ? allowed : restricted} {}
		
		/// Check if a restriction is enforced
		restriction check();
		
		/// Add a new set of restrictions
		void join(const restriction_ck& o);
		
		flags::vector restricted;  ///< set of restrictions on matches
	private:
		restriction_mgr& mgr;      ///< restriction manager
		unsigned long update;      ///< last update seen
		restriction state;         ///< saved restriction state
	};  // class restriction_ck
	
	/// Forward declarations of expression node types
	class match_node;
	class fail_node;
	class inf_node;
	class char_node;
	class range_node;
	class any_node;
	class str_node;
	class rule_node;
	class alt_node;
	class cut_node;
	class end_node;
	
	/// Type of expression node
	enum node_type {
		match_type,
		fail_type,
		inf_type,
		char_type,
		range_type,
		any_type,
		str_type,
		rule_type,
		alt_type,
		cut_type,
		end_type
	};
	
	std::ostream& operator<< (std::ostream& out, expr_type t);
	
	/// Abstract base class of all expression visitors
	class visitor {
	public:
		virtual void visit(match_node&) = 0;
		virtual void visit(fail_node&)  = 0;
		virtual void visit(inf_node&)   = 0;
		virtual void visit(char_node&)  = 0;
		virtual void visit(range_node&) = 0;
		virtual void visit(any_node&)   = 0;
		virtual void visit(str_node&)   = 0;
		virtual void visit(rule_node&)  = 0;
		virtual void visit(alt_node&)   = 0;
		virtual void visit(cut_node&)   = 0;
		virtual void visit(end_node&)   = 0;
	}; // class visitor
	
	/// Abstract base class for expression nodes
	class node {
	protected:
		node() = default;
	
	public:
		virtual ~node() = default;
		
		/// Abreviates std::make_shared for expression nodes
		template<typename T, typename... Args>
		static ptr<node> make_ptr(Args&&... args) {
			return std::static_pointer_cast<expr>(
					std::make_shared<T>(
						std::forward<Args>(args)...));
		}
		
		/// Clone this node
		virtual ptr<node> clone() const = 0;
		
		/// Accept visitor
		virtual void accept(visitor*) = 0;
		
		/// Derivative of this expression with respect to x
		virtual ptr<node> d(char x) const = 0;
		
		/// Expression node type
		virtual node_type type() const = 0;
	};  // class node
	
	/// Terminal node representing a match
	class match_node : public node {
	public:
		match_node() : cuts() {}
		match_node(bitvec&& cuts) : cuts(std::move(cuts)) {}
		virtual ~match_node() = default;
		
		/// Returns match node
		static  ptr<node> make();
		virtual ptr<node> clone() const;
		virtual void      accept(visitor* v) { v->visit(*this); }
		virtual ptr<node> d(char) const;
		virtual node_type type()  const      { return match_type; }
		
		bitvec cuts;  ///< Cut generations that can block this node
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
		
		/// Returns fail node
		static  ptr<node> make();
		virtual ptr<node> clone() const;
		virtual void      accept(visitor* v) { v->visit(*this); }
		virtual ptr<node> d(char) const;
		virtual node_type type()  const      { return fail_type; }
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
		
		/// Returns infinite loop node
		static  ptr<node> make();
		virtual ptr<node> clone() const;
		virtual void      accept(visitor* v) { v->visit(*this); }
		virtual ptr<node> d(char) const;
		virtual node_type type()  const      { return inf_type; }
	}; // inf_node
	
	/// Node representing a character literal
	class char_node : public node {
		
	}; // char_node
	
	/// Node representing a character range literal
	class range_node : public node {
		
	}; // range_node
	
	/// Node representing an "any character" literal
	class any_node : public node {
		
	}; // any_node
	
	/// Node representing a string literal
	class str_node : public node {
		
	}; // str_node
	
	/// Node representing a non-terminal
	class rule_node : public node {
		
	}; // rule_node
	
	/// Node containing a number of subexpressions to parse concurrently
	class alt_node : public node {
		
	}; // alt_node
	
	/// Node which inserts values into the restriction set
	class cut_node : public node {
		
	}; // cut_node
	
}
