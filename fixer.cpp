#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "fixer.hpp"
#include "utils/scope_sub.hpp"

namespace derivs {
	using ast::matcher_mode;

	matcher_mode fixer::operator() (const ast::matcher_ptr& e) {
		if ( ! e ) return {};

		auto it = cache.find( e.get() );
		if ( it != cache.end() ) return it->second;

		return fix_match( e );
	}
	
	matcher_mode fixer::fix_match(const ast::matcher_ptr& e) {
		auto et = e->type();
		// fast path for expressions that can't have recursive match set
		switch ( et ) {
		case ast::char_type: case ast::str_type: case ast::range_type: 
		case ast::any_type: case ast::empty_type: case ast::none_type: 
		case ast::action_type: case ast::look_type: case ast::not_type: 
		case ast::fail_type:
			mode = matcher_mode{ e->nbl(), e->look() };
			cache.emplace( e.get(), mode );
			return mode;
		default: break;
		}

		// fixed point calculation for recursive expressions
		bool changed = false;
		fixer::emap visited;
		fixer::rmap rules_visited;
		running.insert(e.get());

		// recalculate expressions until they don't change; 
		// by Kleene's thm, a fixed point
		matcher_mode mode{};
		while (true) {
			mode = iter_match(e, rules_visited, visited, changed);
			if ( ! changed ) break;
			changed = false;
			rules_visited.clear();
			visited.clear();
		}

		running.clear();
		cache.emplace( e.get(), mode );
		if ( et == ast::rule_type ) {
			by_name.emplace(
				std::static_pointer_cast<ast::rule_matcher>(e)->rule, mode );
		}

		return mode;
	}
	
	matcher_mode fixer::iter_match(const ast::matcher_ptr& e, 
			fixer::rmap& rules_visited, fixer::emap& visited, bool& changed) {
		// check cache
		auto cit = cache.find( e.get() );
		if ( cit != cache.end() ) return cit->second;
		
		// run this expression, if not in progress
		if ( ! running.count( e.get() ) ) return fix_match( e );
		
		// return already calculated value if visited this iteration, 
		// otherwise store current value
		matcher_mode old_mode{ e->nbl(), e->look() };
		if ( e->type() == ast::rule_type ) {
			const std::string& key = 
				std::static_pointer_cast<ast::rule_matcher>(e)->rule;
			
			auto vit = rules_visited.find( key );
			if ( vit != rules_visited.end() ) return vit->second;

			rules_visited.emplace( key, old_mode );
		} else {
			auto vit = visited.find( e.get() );

			if ( vit != visited.end() ) return vit->second;

			visited.emplace( e.get(), old_mode );
		}
		
		// iterate fixed point for this expression
		matcher_mode new_mode = 
			calc_match( e, rules_visited, visited, changed );
		if ( new_mode != old_mode ) { changed = true; }

		return new_mode;
	}
	
	matcher_mode fixer::calc_match(const ast::matcher_ptr& e, 
			fixer::rmap& rules_visited, fixer::emap& visited, bool& changed) {
		auto s0 = sub_in_scope(this->rules_visited, &rules_visited);
		auto s1 = sub_in_scope(this->visited, &visited);
		auto s2 = sub_in_scope(this->changed, &changed);
		
		e->accept( this );
		
		return mode;
	}

	ast::matcher_mode fixer::iter( ast::matcher_ptr& m ) {
		mode = iter_match( m, *rules_visited, *visited, *changed );
	}
	
	ast::matcher_mode fixer::iter( ast::matcher_ptr& m, 
			fixer::rmap* rules_visited, fixer::emap* visited, bool* changed ) {
		mode = iter_match( m, *rules_visited, *visited, *changed );
	}

	void fixer::visit(ast::char_matcher&) { mode = matcher_mode{}; }

	void fixer::visit(ast::str_matcher& m) {
		mode = matcher_mode{ m.s.empty() };
	}

	void fixer::visit(ast::range_matcher&) { mode = matcher_mode{}; }

	void fixer::visit(ast::rule_matcher& m) {
		// check for existing fixed point
		auto it = by_name.find( m.rule );
		if ( it != by_name.end() ) {
			m.mm = mode = it->second;
			return;
		}

		// return current setting if already running this rule
		auto rit = rules_visited->find( m.rule );
		if ( rit != rules_visited->end() ) {
			mode = rit->second;
			return;
		}
		
		// otherwise mark rule as visited and do one iteration
		rmap* rv = rules_visited;
		rv->emplace( m.rule, matcher_mode{} );
		(*rv)[ m.rule ] = m.mm = mode = iter( g->names[ m.rule ]->m );
	}

	void fixer::visit(ast::any_matcher&) {
		mode = matcher_mode{};
	}

	void fixer::visit(ast::empty_matcher&) {
		mode = matcher_mode{ true };
	}

	void fixer::visit(ast::none_matcher&) {
		mode = matcher_mode{};
	}

	void fixer::visit(ast::action_matcher&) {
		mode = matcher_mode{ true };
	}

	void fixer::visit(ast::opt_matcher& m) {
		iter( m.m );
		mode.nbl = true;
	}

	void fixer::visit(ast::many_matcher& m) {
		iter( m.m );
		mode.nbl = true;
	}

	void fixer::visit(ast::some_matcher& m) {
		iter( m.m );
	}

	void fixer::visit(ast::seq_matcher& m) {
		// empty matcher if empty sequence
		if ( m.ms.empty() ) {
			m.mm = mode = { true };
			return;
		}

		// backup visited, changed in case changed by subexpressions
		rmap* rules_visited = this->rules_visited;
		emap* visited = this->visited;
		bool* changed = this->changed;

		// calculate mode for matcher
		auto it = m.ms.begin();
		matcher_mode seq_mode = iter( *it );;
		while ( ++it != m.ms.end() ) {
			seq_mode &= iter( *it, rules_visited, visited, changed );
		}
		m.mm = mode = seq_mode;
	}

	void fixer::visit(ast::alt_matcher& m) {
		// empty matcher if empty sequence
		if ( m.ms.empty() ) {
			m.mm = mode = { true };
			return;
		}

		// backup visited, changed in case changed by subexpressions
		rmap* rules_visited = this->rules_visited;
		emap* visited = this->visited;
		bool* changed = this->changed;

		// calculate mode for matcher
		auto it = m.ms.begin();
		matcher_mode alt_mode = iter( *it );;
		while ( ++it != m.ms.end() ) {
			alt_mode |= iter( *it, rules_visited, visited, changed );
		}
		m.mm = mode = alt_mode;
	}

	void fixer::visit(ast::until_matcher& m) {
		// backup visited, changed in case changed by subexpressions
		rmap* rules_visited = this->rules_visited;
		emap* visited = this->visited;
		bool* changed = this->changed;

		// ensure repeated subexpression fixed
		iter( m.r );

		// take mode of terminator
		mode = iter( m.t, rules_visited, visited, changed );
	}

	void fixer::visit(ast::look_matcher& m) {
		// ensure subexpression fixed
		iter( m.m );
		
		mode = matcher_mode{ false, true };
	}

	void fixer::visit(ast::not_matcher& m) {
		// ensure subexpression fixed
		iter( m.m );
		
		mode = matcher_mode{ false, true };
	}

	void fixer::visit(ast::capt_matcher& m) {
		iter( m.m );
	}

	void fixer::visit(ast::named_matcher& m) {
		iter( m.m );
	}

	void fixer::visit(ast::fail_matcher&) {
		mode = matcher_mode{};
	}

	void fixer::fix_all() {
		for (const auto& rule : g->rs) {
			if ( by_name.count( rule->name ) ) continue;
			matcher_mode rm = (*this)( rule->m );
			by_name.emplace( rule->name, rm );
		}
	}
}
