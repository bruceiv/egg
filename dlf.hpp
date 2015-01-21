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

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <ostream>
#include <unordered_set>
#include <utility>

//#include "utils/flagset.hpp"
#include "utils/flagvector.hpp"

#include "utils/hash_bag.hpp"

#include "utils/plalloc.hpp"

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
	/// Restriction index
	using cutind = flags::index;

	/// Set of restriction indices
//	using cutset = flags::set;
	using cutset = flags::vector;
	
	/// Shorthand for shared_ptr
	template <typename T>
	using ptr = std::shared_ptr<T>;

	/// Use weak_ptr as well
	using std::weak_ptr;

	/// Abbreviates std::make_shared
	template<typename T, typename... Args>
	inline ptr<T> make_ptr(Args&&... args) {
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	/// Abreviates std::static_pointer_cast
	template<typename T, typename U>
	inline ptr<T> as_ptr(const ptr<U>& p) { return std::static_pointer_cast<T>(p); }

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
	class cut_node;
	class alt_node;

	/// Forward declaration of arc type
	class arc;

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
		cut_type   = 0x9,
		alt_type   = 0xA,
	};

	/// Tags `x` with the given node type; useful for hashing
	inline constexpr std::size_t tag_with(node_type ty, std::size_t x = 0x0) {
		return (x << 4) | static_cast<std::size_t>(ty);
	}

	/// Prints node type
	std::ostream& operator<< (std::ostream& out, node_type ty) {
		switch ( ty ) {
		case match_type: out << "MATCH"; break;
		case fail_type:  out << "FAIL";  break;
		case inf_type:   out << "INF";   break;
		case end_type:   out << "END";   break;
		case char_type:  out << "CHAR";  break;
		case range_type: out << "RANGE"; break;
		case any_type:   out << "ANY";   break;
		case str_type:   out << "STR";   break;
		case rule_type:  out << "RULE";  break;
		case cut_type:   out << "CUT";   break;
		case alt_type:   out << "ALT";   break;
		}
		return out;
	}

	/// Abstract base class of all node visitors
	class visitor {
	public:
		virtual void visit(const match_node&) = 0;
		virtual void visit(const fail_node&)  = 0;
		virtual void visit(const inf_node&)   = 0;
		virtual void visit(const end_node&)   = 0;
		virtual void visit(const char_node&)  = 0;
		virtual void visit(const range_node&) = 0;
		virtual void visit(const any_node&)   = 0;
		virtual void visit(const str_node&)   = 0;
		virtual void visit(const rule_node&)  = 0;
		virtual void visit(const cut_node&)   = 0;
		virtual void visit(const alt_node&)   = 0;
	}; // visitor

	/// Abstract base class for expression nodes
	class node {
	protected:
		node() = default;

	public:
		virtual ~node() = default;

		/// Abreviates std::make_shared for expression nodes
		template<typename T, typename... Args>
		static inline ptr<node> make(Args&&... args) {
			return as_ptr<node>(make_ptr<T>(std::forward<Args>(args)...));
		}

		/// Accept visitor
		virtual void accept(visitor*) const = 0;

		/// Check if this node has a single successor arc it can return
		virtual bool has_succ() const = 0;

		/// Returns the single successor node (or nullptr if none such)
		virtual arc get_succ() const = 0;

		/// Returns a clone of this node, with the provided successor
		virtual ptr<node> clone_with_succ(arc&& succ) const = 0;

		/// Expression node type
		virtual node_type type() const = 0;

		/// Polymorphic hash function; doesn't account for successor nodes
		virtual std::size_t hash() const = 0;

		/// Polymorphic equality function; doesn't account for successor nodes
		virtual bool equiv(ptr<node> o) const = 0;
	};  // node

	/// Interface for arc destruction listeners
	class cut_listener {
	protected:
		cut_listener() = default;
	public:
		virtual ~cut_listener() = default;

		/// State that these cuts can newly be acquired
		virtual void acquire_cut(cutind cut) = 0;

		/// State that these cuts can no longer be acquired
		virtual void release_cut(cutind cut) = 0;
	}; // cut listener

	/// Directed arc linking two nodes
	struct arc {
		arc(ptr<node> succ, cutset&& blocking = cutset{})
		: succ{succ}, blocking{std::move(blocking)} {}

		ptr<node> succ;   ///< Successor pointer
		cutset blocking;  ///< Restrictions blocking this arc
	};

	/// Terminal node representing a match
	class match_node : public node {
		// disallow copying and moving; access through make()
		match_node() = default;
		match_node(weak_ptr<node> self) : self{self} {}
		match_node(const match_node&) = delete;
		match_node& operator= (const match_node&) = delete;
		match_node(match_node&&) = delete;
		match_node& operator= (match_node&&) = delete;
	public:
		virtual ~match_node() = default;

		static  ptr<node>   make() {
			ptr<match_node> mn{new match_node};
			mn->self = weak_ptr<node>{mn};
			return as_ptr<node>(mn);
		}
		/// Return clone of this node
		ptr<node>   clone() const { return self.lock(); }

		virtual void        accept(visitor* v) const { v->visit(*this); }
		virtual bool        has_succ() const { return false; }
		virtual arc         get_succ() const { return arc{ptr<node>{}}; }
		virtual ptr<node>   clone_with_succ(arc&& succ) const { return clone(); }
		virtual node_type   type() const { return match_type; }
		virtual std::size_t hash() const { return tag_with(match_type); }
		virtual bool        equiv(ptr<node> o) const { return o->type() == match_type; }
	private:
		weak_ptr<node> self;  ///< Self pointer to not duplicate node
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

		static  ptr<node>   make() {
			static ptr<node> singleton = as_ptr<node>(ptr<fail_node>{new fail_node});
			return singleton;
		}

		virtual void        accept(visitor* v) const { v->visit(*this); }
		virtual bool        has_succ() const { return false; }
		virtual arc         get_succ() const { return arc{ptr<node>{}}; }
		virtual ptr<node>   clone_with_succ(arc&& succ) const { return fail_node::make(); }
		virtual node_type   type() const { return fail_type; }
		virtual std::size_t hash() const { return tag_with(fail_type); }
		virtual bool        equiv(ptr<node> o) const { return o->type() == fail_type; }
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

		static  ptr<node>   make() {
			static ptr<node> singleton = as_ptr<node>(ptr<inf_node>{new inf_node});
			return singleton;
		}
		virtual void        accept(visitor* v) const { v->visit(*this); }
		virtual bool        has_succ() const { return false; }
		virtual arc         get_succ() const { return arc{ptr<node>{}}; }
		virtual ptr<node>   clone_with_succ(arc&& succ) const { return inf_node::make(); }
		virtual node_type   type() const { return inf_type; }
		virtual std::size_t hash() const { return tag_with(inf_type); }
		virtual bool        equiv(ptr<node> o) const { return o->type() == inf_type; }
	}; // inf_node

	/// Placeholder node for the end of a subexpression
	class end_node : public node {
	public:
		end_node() = default;
		virtual ~end_node() = default;

		static  ptr<node>   make() { return node::make<end_node>(); }

		virtual void        accept(visitor* v) const { v->visit(*this); }
		virtual bool        has_succ() const { return false; }
		virtual arc         get_succ() const { return arc{ptr<node>{}}; }
		virtual ptr<node>   clone_with_succ(arc&& succ) const { return end_node::make(); }
		virtual node_type   type() const { return end_type; }
		virtual std::size_t hash() const { return tag_with(end_type); }
		virtual bool        equiv(ptr<node> o) const { return o->type() == end_type; }
	}; // end_node

	/// Node representing a character literal
	class char_node : public node {
	public:
		char_node(arc&& out, char c) : out{std::move(out)}, c{c} {}
		virtual ~char_node() = default;

		static  ptr<node>   make(arc&& out, char c) { return node::make<char_node>(std::move(out), c); }

		virtual void        accept(visitor* v) const { v->visit(*this); }
		virtual bool        has_succ() const { return true; }
		virtual arc         get_succ() const { return out; }
		virtual ptr<node>   clone_with_succ(arc&& succ) const {
			return node::make<char_node>(std::move(succ), c);
		}
		virtual node_type   type() const { return char_type; }
		virtual std::size_t hash() const { return tag_with(char_type, c); }
		virtual bool        equiv(ptr<node> o) const {
			return o->type() == char_type && as_ptr<char_node>(o)->c == c;
		}

		arc out;  ///< Successor node
		char c;   ///< Character represented by the expression
	}; // char_node

	/// Node representing a character range literal
	class range_node : public node {
	public:
		range_node(arc&& out, char b, char e) : out{std::move(out)}, b{b}, e{e} {}
		virtual ~range_node() = default;

		static  ptr<node>   make(arc&& out, char b, char e) {
			if ( b > e ) return fail_node::make();
			else if ( b == e ) return char_node::make(std::move(out), b);
			else return node::make<range_node>(std::move(out), b, e);
		}

		virtual void        accept(visitor* v) const { v->visit(*this); }
		virtual bool        has_succ() const { return true; }
		virtual arc         get_succ() const { return out; }
		virtual ptr<node>   clone_with_succ(arc&& succ) const {
			return node::make<range_node>(std::move(succ), b, e);
		}
		virtual node_type   type() const { return range_type; }
		virtual std::size_t hash() const { return tag_with(range_type, (b << 8) | e); }
		virtual bool        equiv(ptr<node> o) const {
			if ( o->type() != range_type ) return false;
			ptr<range_node> oc = as_ptr<range_node>(o);
			return oc->b == b && oc->e == e;
		}

		arc out;  ///< Successor node
		char b;   ///< First character in expression range
		char e;   ///< Last character in expression range
	}; // range_node

	/// Node representing an "any character" literal
	class any_node : public node {
	public:
		any_node(arc&& out) : out{std::move(out)} {}
		virtual ~any_node() = default;

		static  ptr<node>   make(arc&& out) { return node::make<any_node>(std::move(out)); }

		virtual void        accept(visitor* v) const { v->visit(*this); }
		virtual bool        has_succ() const { return true; }
		virtual arc         get_succ() const { return out; }
		virtual ptr<node>   clone_with_succ(arc&& succ) const {
			return node::make<any_node>(std::move(succ));
		}
		virtual node_type   type() const { return any_type; }
		virtual std::size_t hash() const { return tag_with(any_type); }
		virtual bool        equiv(ptr<node> o) const { return o->type() == any_type; }

		arc out;  ///< Successor node
	}; // any_node

	/// Node representing a string literal
	class str_node : public node {
	public:
		str_node(arc&& out, const std::string& s)
		: out{std::move(out)}, sp{make_ptr<std::string>(s)}, i{0} {}
		str_node(arc&& out, std::string&& s)
		: out{std::move(out)}, sp{make_ptr<std::string>(std::move(s))}, i{0} {}
		str_node(arc&& out, ptr<std::string> sp, std::string::size_type i)
		: out{std::move(out)}, sp{sp}, i{i} {}

		static  ptr<node>   make(arc&& out, const std::string& s) {
			switch ( s.size() ) {
			case 0:  return out.succ;
			case 1:  return char_node::make(std::move(out), s[0]);
			default: return node::make<str_node>(std::move(out), s);
			}
		}

		virtual void        accept(visitor* v) const { v->visit(*this); }
		virtual bool        has_succ() const { return true; }
		virtual arc         get_succ() const { return out; }
		virtual ptr<node>   clone_with_succ(arc&& succ) const {
			return node::make<str_node>(std::move(succ), sp, i);
		}
		virtual node_type   type() const { return str_type; }
		virtual std::size_t hash() const { return tag_with(str_type, std::size_t(sp.get()) ^ i); }
		virtual bool        equiv(ptr<node> o) const {
			if ( o->type() != str_type ) return false;
			ptr<str_node> oc = as_ptr<str_node>(o);
			return oc->sp == sp && oc->i == i;
		}

		std::string str() const { return std::string{*sp, i}; }
		std::string::size_type size() const { return sp->size() - i; }

		arc out;                   ///< Successor node
		ptr<std::string> sp;       ///< Pointer to the interred string
		std::string::size_type i;  ///< Index into the interred string
	}; // str_node

	/// Nonterminal substitution
	struct nonterminal {
		nonterminal(const std::string& name) : name{name}, sub{fail_node::make()} {}
		nonterminal(const std::string& name, ptr<node> sub) : name{name}, sub{sub} {}

		const std::string name;  ///< Name of the non-terminal
		ptr<node> sub;           ///< First subexpression in the non-terminal
	};  // nonterminal

	/// Node representing a non-terminal
	class rule_node : public node {
	public:
		rule_node(arc&& out, ptr<nonterminal> r) : out{std::move(out)}, r{r} {}
		virtual ~rule_node() = default;

		static  ptr<node>   make(arc&& out, ptr<nonterminal> r) {
			return node::make<rule_node>(std::move(out), r);
		}

		virtual void        accept(visitor* v) const { v->visit(*this); }
		virtual bool        has_succ() const { return true; }
		virtual arc         get_succ() const { return out; }
		virtual ptr<node>   clone_with_succ(arc&& succ) const {
			return node::make<rule_node>(std::move(succ), r);
		}
		virtual node_type   type() const { return rule_type; }
		virtual std::size_t hash() const { return tag_with(rule_type, std::size_t(r.get())); }
		virtual bool        equiv(ptr<node> o) const {
			return o->type() == rule_type && as_ptr<rule_node>(o)->r == r;
		}

		arc out;             ///< Successor node
		ptr<nonterminal> r;  ///< Pointer to shared rule definition
	}; // rule_node

	/// Node representing a cut in the graph
	class cut_node : public node {
	public:
		cut_node(arc&& out, cutind cut) : out{std::move(out)}, cut{cut}, listener{nullptr} {}
		cut_node(arc&& out, cutind cut, cut_listener& _listener)
		: out{std::move(out)}, cut{cut}, listener{&_listener} { _listener.acquire_cut(cut); }
		cut_node(arc&& out, cutind cut, cut_listener* listener)
		: out{std::move(out)}, cut{cut}, listener{listener} {
			if ( listener ) { listener->acquire_cut(cut); }
		}
		cut_node(const cut_node& o) : out{o.out}, cut{o.cut}, listener{o.listener} {
			if ( listener ) { listener->acquire_cut(cut); }
		}
		cut_node(cut_node&& o) : out{std::move(o.out)}, cut{o.cut}, listener{o.listener} {
			o.listener = nullptr;  // release responsibility moved to this node
		}

		cut_node& operator= (const cut_node& o) {
			// Acquire new cut THEN release current one
			if ( o.listener ) { o.listener->acquire_cut(o.cut); }
			if ( listener ) { listener->release_cut(cut); }

			out = o.out;
			cut = o.cut;
			listener = o.listener;

			return *this;
		}
		cut_node& operator= (cut_node&& o) {
			// Rely on o to have acquired new cut, just release old one
			if ( listener ) { listener->release_cut(cut); }

			out = std::move(o.out);
			cut = o.cut;
			listener = o.listener;

			// make sure o doesn't release the cut
			o.listener = nullptr;

			return *this;
		}

		virtual ~cut_node() { if ( listener ) listener->release_cut(cut); }

		static  ptr<node>   make(arc&& out, cutind cut) {
			return node::make<cut_node>(std::move(out), cut);
		}
		static  ptr<node>   make(arc&& out, cutind cut, cut_listener& listener) {
			return node::make<cut_node>(std::move(out), cut, listener);
		}

		virtual void        accept(visitor* v) const { v->visit(*this); }
		virtual bool        has_succ() const { return true; }
		virtual arc         get_succ() const { return out; }
		virtual ptr<node>   clone_with_succ(arc&& succ) const {
			ptr<node> r = node::make<cut_node>(std::move(succ), cut);
			// silently transfer listenership to new node WARNING this is horrific
			as_ptr<cut_node>(r)->listener = listener;
			const_cast<cut_node*>(this)->listener = nullptr;
			return r;
		}
		virtual node_type   type() const { return cut_type; }
		virtual std::size_t hash() const { return tag_with(cut_type, cut); }
		virtual bool        equiv(ptr<node> o) const {
			return o->type() == cut_type && as_ptr<cut_node>(o)->cut == cut;
		}

		arc out;                 ///< Successor node
		cutind cut;              ///< Index to block
		cut_listener* listener;  ///< Listener to alert of changes in the cut state
	};  // cut_node

	/// Set of arcs which pushes alternation through its successor nodes.
	class arc_set {
	public:
		/// Hashes arcs by their pointed-to nodes
		struct succ_hash {
			std::size_t operator() (const arc& a) const { return a.succ->hash(); }
		};

		/// Puts arcs into equivalence classes by the structure of their pointed-to nodes
		struct succ_equiv {
			bool operator() (const arc& a, const arc& b) const { return a.succ->equiv(b.succ); }
		};

		/// Underlying set type
//		using impl_set = std::unordered_set<arc, succ_hash, succ_equiv, plalloc<arc>>;
		using impl_set = hash_bag<arc, succ_hash, succ_equiv>;

		// STL defines
		using value_type = impl_set::value_type;
		using size_type = impl_set::size_type;
		using difference_type = impl_set::difference_type;
		using reference = impl_set::reference;
		using const_reference = impl_set::const_reference;
		using pointer = impl_set::pointer;
		using const_pointer = impl_set::const_pointer;
		using iterator = impl_set::iterator;
		using const_iterator = impl_set::const_iterator;

	private:
		/// Emplace a new arc into the set, maintaining the invariants that no fail nodes 
		/// are allowed, all alt nodes are flattened, equal nodes are blocked by the 
		/// intersection of their blocking sets, and equivalent nodes have the alternation 
		/// factored past them
		void emplace_invar(arc&& a);

	public:
		arc_set() = default;
		template<typename It> arc_set(It first, It last) : s(first, last) {}
		arc_set(const arc_set&) = default;
		arc_set(arc_set&&) = default;
		arc_set(std::initializer_list<arc> s) : s{s} {}
		arc_set& operator= (const arc_set&) = default;
		arc_set& operator= (arc_set&&) = default;
		arc_set& operator= (std::initializer_list<arc> sn) { s = sn; return *this; }
		void swap(arc_set& o) { s.swap(o.s); }
		~arc_set() = default;

		iterator begin() { return s.begin(); }
		const_iterator begin() const { return s.cbegin(); }
		const_iterator cbegin() const { return s.cbegin(); }
		iterator end() { return s.end(); }
		const_iterator end() const { return s.cend(); }
		const_iterator cend() const { return s.cend(); }

		bool empty() const { return s.empty(); }
		size_type size() const { return s.size(); }

		void clear() { s.clear(); }
		void insert(const arc& a) { emplace_invar(arc{a}); }
		void insert(arc&& a) { emplace_invar(std::move(a)); }
		template<typename It> void insert(It first, It last) {
			while ( first != last ) { insert(*first); ++first; }
		}
		void insert(std::initializer_list<arc> sn) { for (const arc& a : sn) insert(a); }
		void emplace(arc&& a) { emplace_invar(std::move(a)); }
		iterator erase(iterator pos) { return s.erase(pos); }
		size_type erase(const arc& a) { return s.erase(a); }

		size_type count(const arc& a) const { return s.count(a); }
		iterator find(const arc& a) { return s.find(a); }
		const_iterator find(const arc& a) const { return s.find(a); }

	private:
		impl_set s;
	}; // arc_set

	/// Node containing a number of subexpressions to parse concurrently
	class alt_node : public node {
	public:
		alt_node() = default;
		alt_node(const arc_set& out) : out{out} {}
		alt_node(arc_set&& out) : out{out} {}
		alt_node(const alt_node&) = default;
		alt_node(alt_node&&) = default;
		alt_node& operator= (const alt_node&) = default;
		alt_node& operator= (alt_node&&) = default;
		virtual ~alt_node() = default;

		static  ptr<node>   make(arc_set&& as) {
			return as.empty() ? fail_node::make() : node::make<alt_node>(std::move(as));
		}
		static  ptr<node>   make(arc&& a, arc&& b) {
			arc_set as;
			as.emplace(std::move(a));
			as.emplace(std::move(b));
			return make(std::move(as));
		}
		static  ptr<node>   make(arc&& a, arc&& b, arc&& c) {
			arc_set as;
			as.emplace(std::move(a));
			as.emplace(std::move(b));
			as.emplace(std::move(c));
			return make(std::move(as));
		}

		virtual void        accept(visitor* v) const { v->visit(*this); }
		virtual bool        has_succ() const { return false; }
		virtual arc         get_succ() const { return arc{ptr<node>{}}; }
		virtual ptr<node>   clone_with_succ(arc&& succ) const { return fail_node::make(); }
		virtual node_type   type() const { return alt_type; }
		virtual std::size_t hash() const { return tag_with(alt_type); }
		virtual bool        equiv(ptr<node> o) const { return o->type() == alt_type; }

		arc_set out;
	}; // alt_node
	
	void arc_set::emplace_invar(arc&& a) {
		node_type ty = a.succ->type();

		// skip added fail nodes
		if ( ty == fail_type ) return;

		// flatten added alt nodes
		if ( ty == alt_type ) {
			alt_node& an = *as_ptr<alt_node>(a.succ);

			for (arc aa : an.out) {
				aa.blocking |= a.blocking;
				emplace_invar(std::move(aa));
			}

			return;
		}

		auto ai = s.find(a);
		// add nodes that don't yet exist
		if ( ai == s.end() ) {
			s.emplace(std::move(a));
			return;
		}

		// merge nodes that are already present
		arc& e = const_cast<arc&>(*ai);
		if ( e.succ == a.succ || ! e.succ->has_succ() ) {
			/// same node or unfollowed, merge blocking sets
			e.blocking &= a.blocking;
		} else {
			/// distinct node, merge blocking sets and alternate successors
			arc ee = e.succ->get_succ(), aa = a.succ->get_succ();
			ee.blocking |= (e.blocking - a.blocking);
			aa.blocking |= (a.blocking - e.blocking);
			e.blocking &= a.blocking;
			e.succ = e.succ->clone_with_succ(alt_node::make(std::move(ee), std::move(aa)));
		}
	}

	/// Default visitor that just visits all the nodes; override individual methods to add
	/// functionality. Stores visited nodes so that they're not re-visited.
	class iterator : public visitor {
	protected:
		virtual void visit(const arc& a) { visit(a.succ); }
		virtual void visit(const ptr<node> np) {
			// check memo-table and visit if not found
			if ( visited.count(np) == 0 ) {
				visited.emplace(np);
				np->accept(this);
			}
		}

	public:
		// Unfollowed nodes are no-ops
		virtual void visit(const match_node&)   {}
		virtual void visit(const fail_node&)    {}
		virtual void visit(const inf_node&)     {}
		virtual void visit(const end_node&)     {}
		// Remaining nodes get their successors visited
		virtual void visit(const char_node& n)  { visit(n.out); }
		virtual void visit(const range_node& n) { visit(n.out); }
		virtual void visit(const any_node& n)   { visit(n.out); }
		virtual void visit(const str_node& n)   { visit(n.out); }
		virtual void visit(const rule_node& n)  { visit(n.out); }
		virtual void visit(const cut_node& n)   { visit(n.out); }
		virtual void visit(const alt_node& n)   { for (auto& out : n.out) visit(out); }
	protected:
		std::unordered_set<ptr<node>> visited;  ///< Nodes already seen
	};  // iterator
}
