#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "fixer.hpp"

namespace derivs {
	matcher_mode fixer::operator() (const ast::matcher_ptr& e) {
		if ( ! e ) return {};

		auto it = cache.find( e.get() );
		if ( it != cache.end() ) return it->second;

		return fix_match( e );
	}
	
	matcher_mode fixer::fix_match(const ast::matcher_ptr& e) {
		// fast path for expressions that have fundamental match set
		switch ( e->type() ) {
		case ast::char_type: case ast::str_type: case ast::range_type: 
		case ast::any_type: case ast::empty_type: case ast::none_type: 
		case ast::action_type: case ast::look_type: case ast::not_type: 
		case ast::fail_type:
			e->accept( this );
			return cache.emplace( e.get(), mode ).second;
		default: break;
		}

		// fixed point calculation for recursive expressions
		bool changed = false;
		fixer::emap visited;
		running.insert(e.get());

		// recalculate expressions until they don't change; 
		// by Kleene's thm, a fixed point
		matcher_mode mode{};
		while (true) {
			mode = iter_match(e, visited, changed);
			if ( ! changed ) break;
			changed = false;
			visited.clear();
		}

		running.clear();
		cache.emplace( e.get(), mode );
		if ( e->type() == ast::rule_type ) {
			by_name.emplace(
				std::static_pointer_cast<ast::rule_matcher>(e)->rule, mode );
		}

		return mode;
	}
	
	matcher_mode fixer::iter_match(const ast::matcher_ptr& e, 
			fixer::emap& visited, bool& changed) {
		// check cache
		auto cit = cache.find( e.get() );
		if ( cit != cache.end() ) return cit->second;
		
		// run this expression, if not in progress
		if ( ! running.count( e.get() ) ) return fix_match( e );
		
		// return already calculated value if visited this iteration
		auto vit = visited.find( e.get() );
		if ( vit != visited.end() ) return vit->second;

		// otherwise store current value
		bool iter = this->iter;
		this->iter = false;
		e->accept( this );
		visited.emplace( e.get(), mode );
		this->iter = iter;

		// iterate fixed point for this expression
		matcher_mode old_mode = mode;
		matcher_mode new_mode = calc_match( e, visited, changed );
		if ( new_mode != old_mode ) { changed = true; }

		return new_mode;
	}
	
	matcher_mode fixer::calc_match(const ast::matcher_ptr& e, 
			fixer::emap& visited, bool& changed) {
		this->visited = &visited;
		this->changed = &changed;
		
		bool iter = this->iter;
		this->iter = true;
		e->accept( this );
		this->iter = iter;

		return mode;
	}

	void fixer::iter_or_visit( ast::matcher_ptr& m ) {
		if ( iter ) {
			mode = iter_match( m, *visited, *changed );
		} else {
			m->accept( this );
		}
	}
	
	void fixer::iter_or_visit( ast::matcher_ptr& m, 
			fixer::emap* visited, bool* changed ) {
		if ( iter ) {
			mode = iter_match( m, *visited, *changed );
		} else {
			m->accept( this );
		}
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
			mode = it->second;
			return;
		}

		// return bottom if not iterating
		if ( ! iter ) {
			mode = matcher_mode{};
			return;
		}

		// otherwise turn off iteration and do one iteration
		iter = false;
		mode = iter_match( g->names[ m.rule ]->m, *visited, *changed );
		iter = true;
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
		iter_or_visit( m.m );
		mode.nbl = true;
	}

	void fixer::visit(ast::many_matcher& m) {
		iter_or_visit( m.m );
		mode.nbl = true;
	}

	void fixer::visit(ast::some_matcher& m) {
		iter_or_visit( m.m );
	}

	void fixer::visit(ast::seq_matcher& m) {
		// empty matcher if empty sequence
		if ( m.ms.empty() ) {
			mode = { true };
			return;
		}

		// backup visited, changed in case changed by subexpressions
		emap* visited = this->visited;
		bool* changed = this->changed;

		// calculate mode for matcher
		auto it = m.ms.begin();
		(*it)->accept( this );
		matcher_mode seq_mode = mode;
		while ( ++it != m.ms.end() ) {
			iter_or_visit( *it, visited, changed );
			seq_mode &= mode;
		}
		mode = seq_mode;
	}

	void fixer::visit(ast::alt_matcher& m) {
		// empty matcher if empty sequence
		if ( m.ms.empty() ) {
			mode = { true };
			return;
		}

		// backup visited, changed in case changed by subexpressions
		emap* visited = this->visited;
		bool* changed = this->changed;

		// calculate mode for matcher
		auto it = m.ms.begin();
		(*it)->accept( this );
		matcher_mode alt_mode = mode;
		while ( ++it != m.ms.end() ) {
			iter_or_visit( *it, visited, changed );
			alt_mode |= mode;
		}
		mode = alt_mode;
	}

	void fixer::visit(ast::until_matcher& m) {
		if ( iter ) {
			// backup visited, changed in case changed by subexpressions
			emap* visited = this->visited;
			bool* changed = this->changed;

			// ensure repeated subexpression fixed
			iter_match( m.r, *visited, *changed );

			mode = iter_match( m.t, *visited, *changed );
		} else {
			// until matcher has equivalent mode to terminator
			m.t->accept( this );
		}
	}

	void fixer::visit(ast::look_matcher& m) {
		// ensure subexpression fixed
		if ( iter ) {
			iter_match( m.m, *visited, *changed );
		}

		mode = matcher_mode{ false, true };
	}

	void fixer::visit(ast::not_matcher& m) {
		// ensure subexpression fixed
		if ( iter ) {
			iter_match( m.m, *visited, *changed );
		}

		mode = matcher_mode{ false, true };
	}

	void fixer::visit(ast::capt_matcher& m) {
		iter_or_visit( m.m );
	}

	void fixer::visit(ast::named_matcher& m) {
		iter_or_visit( m.m );
	}

	void fixer::visit(ast::fail_matcher&) {
		mode = matcher_mode{};
	}
}
