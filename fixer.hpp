#include <unordered_map>
#include <unordered_set>

#include "ast.hpp"

namespace derivs {
	/// Represents nullability/lookahead status of an AST node
	struct matcher_mode {
		bool nbl  : 1;  ///< Does this expression match on any string?
		bool look : 1;  ///< Does this expression match on some empty strings?

		constexpr matcher_mode( bool nbl = false, bool look = false ) 
			: nbl(nbl), look(look) {}

		constexpr bool operator== (matcher_mode o) {
			return nbl == o.nbl && look == o.look;
		}

		constexpr bool operator!= (matcher_mode o) {
			return nbl != o.nbl || look != o.look;
		}

		matcher_mode& operator&= (matcher_mode o) {
			nbl &= o.nbl;
			look &= o.look;
			return *this;
		}

		matcher_mode& operator|= (matcher_mode o) {
			nbl |= o.nbl;
			look |= o.look;
			return *this;
		}
	};

	/// Implements fixed-point nullability/lookahead calculation on 
	/// AST nodes.
	class fixer : public ast::visitor {
	public:
		/// Calculates the least fixed point of back and match, 
		/// caches it, and returns it.
		matcher_mode operator() (const ast::matcher_ptr& e);

		virtual void visit(ast::str_matcher&);
		virtual void visit(ast::char_matcher&);
		virtual void visit(ast::range_matcher&);
		virtual void visit(ast::rule_matcher&);
		virtual void visit(ast::any_matcher&);
		virtual void visit(ast::empty_matcher&);
		virtual void visit(ast::none_matcher&);
		virtual void visit(ast::action_matcher&);
		virtual void visit(ast::opt_matcher&);
		virtual void visit(ast::many_matcher&);
		virtual void visit(ast::some_matcher&);
		virtual void visit(ast::seq_matcher&);
		virtual void visit(ast::alt_matcher&);
		virtual void visit(ast::until_matcher&);
		virtual void visit(ast::look_matcher&);
		virtual void visit(ast::not_matcher&);
		virtual void visit(ast::capt_matcher&);
		virtual void visit(ast::named_matcher&);
		virtual void visit(ast::fail_matcher&);

	private:
		/// Map from rule names to modes
		using rmap = std::unordered_map<std::string, matcher_mode>;
		/// Map from matchers to modes
		using emap = std::unordered_map<const ast::matcher*, matcher_mode>;
		/// Set of matchers
		using eset = std::unordered_set<const ast::matcher*>;

		/// Performs fixed point computation to calculate the back and
		/// match sets of e.
		/// Based on Kleene's theorem; iterates upward from a bottom 
		/// set of {false, false}
		matcher_mode fix_match(const ast::matcher_ptr& m);

		/// Recursively calculates next iteration of match set
		matcher_mode iter_match(const ast::matcher_ptr& m, emap& visited, 
				bool& changed);

		/// Wraps visitor pattern for actual calculation of next 
		/// match set
		matcher_mode calc_match(const ast::matcher_ptr& m, emap& visited,
				bool& changed);
		
		/// Iterates the fixed point or just visits the matcher, depending on 
		/// the internal iter flag
		void iter_or_visit( ast::matcher_ptr& m );

		/// Iterates the fixed point or just visits the matcher, depending on 
		/// the internal iter flag
		void iter_or_visit( ast::matcher_ptr& m, emap* visited, bool* changed );
		
		/// Cache for rules
		rmap by_name;
		/// Cache for matcher parameters
		emap cache;
		/// Set of expressions currently being fixed
		eset running;
		/// Set of expressions visited in the current iteration
		emap* visited;
		/// Has anything in the current iteration changed?
		bool* changed;
		/// Match returned by the current iteration
		matcher_mode mode;
		/// Should the current match iterate the fixed-point?
		bool iter;
		/// Grammar being fixed
		ast::grammar_ptr g;
	};
}
