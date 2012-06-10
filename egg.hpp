#ifndef _EGG_EGG_HPP_
#define _EGG_EGG_HPP_

//--
#include <iostream>
//--

#include "parse.hpp"

namespace egg {

	/** Parsing root. Call to parse an Egg grammar.
	 *  @param in		The input stream to read from
	 *  @return Did the input stream contain an Egg grammar?
	 */
	bool parse(std::istream& in) {
		return parse(parse::state(in));
	}
	
	/** Parsing root. Call to parse an Egg grammar.
	 *  @param st		The parsing state to start from
	 *  @return Did the input contained in the passed state contain an Egg 
	 *  		grammar?
	 */
	bool parse(parse::state& st) {
		return grammar(st);
	}
	
	bool grammar(parse::state& st) {
		parse::ind startPos = st.pos;
		
		if ( ! _(st) ) {
			st.pos = startPos;
			return false;
		}
		
		if ( ! nonterminal(st) ) {
			st.pos = startPos;
			return false;
		}
		
		while ( nonterminal(st) ) {}
		
		if ( ! EOF(st) ) {
			st.pos = startPos;
			return false;
		}
		
		parse::ind endPos = st.pos;
		
		{ std::cout << "grammar [" << startPos << "," << endPos << "]" << std::endl; }
	}

} /* namespace egg */

#endif /* _EGG_EGG_HPP_ */

