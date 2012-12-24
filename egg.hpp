#pragma once

//--
#include <iostream>
#include <sstream>
#include <string>

namespace strings {
	
	std::string escapeWhitespace(const std::string& s) {
		std::stringstream ss;
		for (std::string::const_iterator iter = s.begin(); iter != s.end(); ++iter) {
			char c = *iter;
			switch ( c ) {
			case '\n': ss << "\\n"; break;
			case '\r': ss << "\\r"; break;
			case '\t': ss << "\\t"; break;
			case '\\': ss << "\\\\"; break;
			default: ss << c; break;
			}
		}
		return ss.str();
	}
} /* namespace strings */
//--

#include <string>

#include "parse.hpp"

namespace egg {

	bool grammar(parse::state& ps);
	bool out_action(parse::state& ps);
	bool rule(parse::state& ps);
	bool identifier(parse::state& ps);
	bool type_id(parse::state& ps);
	bool choice(parse::state& ps);
	bool sequence(parse::state& ps);
	bool expression(parse::state& ps);
	bool primary(parse::state& ps);
	bool action(parse::state& ps);
	bool char_literal(parse::state& ps);
	bool str_literal(parse::state& ps);
	bool char_class(parse::state& ps);
	bool char_range(parse::state& ps);
	bool character(parse::state& ps);

	bool OUT_BEGIN(parse::state& ps);
	bool OUT_END(parse::state& ps);
	bool BIND(parse::state& ps);
	bool EQUAL(parse::state& ps);
	bool PIPE(parse::state& ps);
	bool AND(parse::state& ps);
	bool NOT(parse::state& ps);
	bool OPT(parse::state& ps);
	bool STAR(parse::state& ps);
	bool PLUS(parse::state& ps);
	bool OPEN(parse::state& ps);
	bool CLOSE(parse::state& ps);
	bool ANY(parse::state& ps);
	bool EMPTY(parse::state& ps);
	bool BEGIN(parse::state& ps);
	bool END(parse::state& ps);
	
	bool _(parse::state& ps);
	bool space(parse::state& ps);
	bool comment(parse::state& ps);
	bool end_of_line(parse::state& ps);
	bool end_of_file(parse::state& ps);
	
	/** Parsing root. Call to parse an Egg grammar.
	 *  @param ps		The parsing state to start from
	 *  @return Did the input contained in the passed state contain an Egg 
	 *  		grammar?
	 */
	bool parse(parse::state& ps) {
		return grammar(ps);
	}
	
	/** Parsing root. Call to parse an Egg grammar.
	 *  @param in		The input stream to read from
	 *  @return Did the input stream contain an Egg grammar?
	 */
	bool parse(std::istream& in) {
		parse::state ps(in);
		return parse(ps);
	}
	
	bool grammar(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }

		out_action(ps);
		
		if ( ! rule(ps) ) { ps.pos = psStart; return false; }
		while ( rule(ps) ) {}

		out_action(ps);
		
		if ( ! end_of_file(ps) ) { ps.pos = psStart; return false; }
		
		parse::ind psLen = ps.pos - psStart;
		{ std::cout << "grammar [" << psStart << "," << psStart+psLen << "]" << std::endl; }
		
		return true;
	}

	bool out_action_1(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( OUT_END(ps) ) { ps.pos = psStart; return false; }

		if ( ! parse::any(ps) ) { ps.pos = psStart; return false; }

		return true;
	}

	bool out_action(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( ! OUT_BEGIN(ps) ) { ps.pos = psStart; return false; }

		parse::ind psCatch = ps.pos;

		while ( out_action_1(ps) ) {}

		parse::ind psCatchLen = ps.pos - psCatch;

		if ( ! OUT_END(ps) ) { ps.pos = psStart; return false; }

		if ( ! _(ps) ) { ps.pos = psStart; return false; }

		{ std::cout << "out_action `" << ps.string(psCatch, psCatchLen) << "`" << std::endl; }

		return true;
	}

	bool rule_1(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( ! BIND(ps) ) { ps.pos = psStart; return false; }

		if ( ! type_id(ps) ) { ps.pos = psStart; return false; }

		return true;
	}
	
	bool rule(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! identifier(ps) ) { ps.pos = psStart; return false; }

		rule_1(ps);
		
		if ( ! EQUAL(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! choice(ps) ) { ps.pos = psStart; return false; }
		
		parse::ind psLen = ps.pos - psStart;
		{ std::cout << "rule [" << psStart << "," << psStart+psLen << "]" << std::endl; }
		
		return true;
	}
	
	bool identifier_1(parse::state& ps) {
		if ( ! (parse::in_range<'A', 'Z'>(ps) 
			|| parse::in_range<'a', 'z'>(ps)
			|| parse::matches<'_'>(ps)) ) { return false; }
		
		return true;
	}
	
	bool identifier_2(parse::state& ps) {
		if ( ! (parse::in_range<'A', 'Z'>(ps) 
			|| parse::in_range<'a', 'z'>(ps)
			|| parse::matches<'_'>(ps)
			|| parse::in_range<'0', '9'>(ps)) ) { return false; }
		
		return true;
	}
	
	bool identifier(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		parse::ind psCatch = ps.pos;
		
		if ( ! identifier_1(ps) ) { ps.pos = psStart; return false; }
		
		while ( identifier_2(ps) ) {}
		
		parse::ind psCatchLen = ps.pos - psCatch;
		std::string psCapture(ps.string(psCatch, psCatchLen));
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		{ std::cout << "identifier `" << psCapture << "`" << std::endl; }
		
		return true;
	}

	bool type_id_1(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( ! (':' == ps[ps.pos++] && ':' == ps[ps.pos++]) ) { ps.pos = psStart; return false; }

		if ( ! _(ps) ) { ps.pos = psStart; return false; }

		if ( ! type_id(ps) ) { ps.pos = psStart; return false; }

		return true;
	}

	bool type_id_2_1(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( ! parse::matches<','>(ps) ) { ps.pos = psStart; return false; }

		if ( ! _(ps) ) { ps.pos = psStart; return false; }

		if ( ! type_id(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}

	bool type_id_2(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( ! parse::matches<'<'>(ps) ) { ps.pos = psStart; return false; }

		if ( ! _(ps) ) { ps.pos = psStart; return false; }

		if ( ! type_id(ps) ) { ps.pos = psStart; return false; }

		while ( type_id_2_1(ps) ) {}

		if ( ! parse::matches<'>'>(ps) ) { ps.pos = psStart; return false; }

		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}

	bool type_id(parse::state& ps) {
		parse::ind psStart = ps.pos;
		parse::ind psCatch = ps.pos;

		if ( ! identifier(ps) ) { ps.pos = psStart; return false; }

		while ( type_id_1(ps) ) {}

		type_id_2(ps);

		parse::ind psCatchLen = ps.pos - psCatch;
		std::string psCapture(ps.string(psCatch, psCatchLen));

		{ std::cout << "type_id `" << psCapture << "`" << std::endl; }

		return true;
	}
	
	bool choice_1(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! PIPE(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! sequence(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool choice(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! sequence(ps) ) { ps.pos = psStart; return false; }
		
		while ( choice_1(ps) ) {}
		
		parse::ind psLen = ps.pos - psStart;
		{ std::cout << "choice [" << psStart << "," << psStart+psLen << "]" << std::endl; }
		
		return true;
	}

	bool sequence_1(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( expression(ps) ) { return true; }
		else { ps.pos = psStart; }

		if ( action(ps) ) { return true; }
		else { ps.pos = psStart; return false; }
	}

	bool sequence(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! sequence_1(ps) ) { ps.pos = psStart; return false; }

		while ( sequence_1(ps) ) {}

		parse::ind psLen = ps.pos - psStart;
		{ std::cout << "sequence [" << psStart << "," << psStart+psLen << "]" << std::endl; }

		return true;
	}
	
	bool expression_1(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( AND(ps) ) { return true; }
		else { ps.pos = psStart; }
		
		if ( NOT(ps) ) { return true; }
		else { ps.pos = psStart; return false; }
	}
	
	bool expression_2(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( OPT(ps) ) { return true; }
		else { ps.pos = psStart; }
		
		if ( STAR(ps) ) { return true; }
		else { ps.pos = psStart; }
		
		if ( PLUS(ps) ) { return true; }
		else { ps.pos = psStart; return false; }
	}
	
	bool expression(parse::state& ps) {
		parse::ind psStart = ps.pos;

		expression_1(ps);

		if ( ! primary(ps) ) { ps.pos = psStart; return false; }

		expression_2(ps);

		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		parse::ind psLen = ps.pos - psStart;
		{ std::cout << "expression [" << psStart << "," << psStart+psLen << "] `" << strings::escapeWhitespace(ps.string(psStart, psLen)) << "`" << std::endl; }
		
		return true;
	}

	bool primary_1_1(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( ! BIND(ps) ) { ps.pos = psStart; return false; }

		if ( ! identifier(ps) ) { ps.pos = psStart; return false; }

		return true;
	}
	
	bool primary_1(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! identifier(ps) ) { ps.pos = psStart; return false; }

		primary_1_1(ps);
		
		if ( EQUAL(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool primary(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( primary_1(ps) ) { return true; }
		else { ps.pos = psStart; }
		
		if ( OPEN(ps) && choice(ps) && CLOSE(ps) ) { return true; }
		else { ps.pos = psStart; }
		
		if ( char_literal(ps) ) { return true; }
		else { ps.pos = psStart; }
		
		if ( str_literal(ps) ) { return true; }
		else { ps.pos = psStart; }
		
		if ( char_class(ps) ) { return true; }
		else { ps.pos = psStart; }
		
		if ( ANY(ps) ) { return true; }
		else { ps.pos = psStart; }

		if ( EMPTY(ps) ) { return true; }
		else { ps.pos = psStart; }
		
		if ( BEGIN(ps) && sequence(ps) && END(ps) ) { return true; }
		else { ps.pos = psStart; return false; }
	}

	bool action_1_1(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( (parse::matches<'}'>(ps)) ) { ps.pos = psStart; return false; }

		if ( ! (parse::any(ps)) ) { ps.pos = psStart; return false; }

		return true;
	}

	bool action_1(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( action(ps) ) { return true; }
		else { ps.pos = psStart; }

		if ( action_1_1(ps) ) { return true; }
		else { ps.pos = psStart; return false; }
	}

	bool action(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( ! (parse::matches<'{'>(ps)) ) { ps.pos = psStart; return false; }

		parse::ind psCatch = ps.pos;

		while ( action_1(ps) ) {}

		parse::ind psCatchLen = ps.pos - psCatch;
		std::string psCapture(ps.string(psCatch, psCatchLen));

		if ( ! (parse::matches<'}'>(ps)) ) { ps.pos = psStart; return false; }

		if ( ! _(ps) ) { ps.pos = psStart; return false; }

		{ std::cout << "action `" << psCapture << "`" << std::endl; }

		return true;
	}
	
	bool char_literal(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! (parse::matches<'\''>(ps)) ) { ps.pos = psStart; return false; }
		
		parse::ind psCatch = ps.pos;
		
		if ( ! character(ps) ) { ps.pos = psStart; return false; }
		
		parse::ind psCatchLen = ps.pos - psCatch;
		std::string psCapture(ps.string(psCatch, psCatchLen));
		
		if ( ! (parse::matches<'\''>(ps)) ) { ps.pos = psStart; return false; }
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		{ std::cout << "char_literal `" << psCapture << "`" << std::endl; }
		
		return true;
	}
	
	bool str_literal(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! (parse::matches<'\"'>(ps)) ) { ps.pos = psStart; return false; }
		
		parse::ind psCatch = ps.pos;
		
		while ( character(ps) ) {}
		
		parse::ind psCatchLen = ps.pos - psCatch;
		std::string psCapture(ps.string(psCatch, psCatchLen));
		
		if ( ! (parse::matches<'\"'>(ps)) ) { ps.pos = psStart; return false; }
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		{ std::cout << "str_literal `" << psCapture << "`" << std::endl; }
		
		return true;
	}
	
	bool char_class_1(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( parse::matches<']'>(ps) ) { ps.pos = psStart; return false; }
		else { ps.pos = psStart; }
		
		if ( ! char_range(ps) ) { return false; }
		
		return true;
	}
	
	bool char_class(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! (parse::matches<'['>(ps)) ) { ps.pos = psStart; return false; }
		
		parse::ind psCatch = ps.pos;
		
		while ( char_class_1(ps) ) {}
		
		parse::ind psCatchLen = ps.pos - psCatch;
		std::string psCapture(ps.string(psCatch, psCatchLen));
		
		if ( ! (parse::matches<']'>(ps)) ) { ps.pos = psStart; return false; }
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		{ std::cout << "char_class `" << psCapture << "`" << std::endl; }
		
		return true;
	}
	
	bool char_range(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( character(ps) && parse::matches<'-'>(ps) && character(ps) ) { return true; }
		else { ps.pos = psStart; }
		
		if ( character(ps) ) { return true; }
		else { ps.pos = psStart; return false; }
	}
	
	bool character_1_1(parse::state& ps) {
		if ( ! (parse::matches<'n'>(ps)
			|| parse::matches<'r'>(ps)
			|| parse::matches<'t'>(ps)
			|| parse::matches<'\''>(ps)
			|| parse::matches<'\"'>(ps)
			|| parse::matches<'\\'>(ps)) ) { return false; }
		
		return true;
	}
	
	bool character_1(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! (parse::matches<'\\'>(ps)) ) { ps.pos = psStart; return false; }
		
		if ( ! character_1_1(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool character_2_1(parse::state& ps) {
		if ( ! (parse::matches<'\''>(ps)
			|| parse::matches<'\"'>(ps)
			|| parse::matches<'\\'>(ps)) ) { return false; }
		
		return true;
	}
	
	bool character_2(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( character_2_1(ps) ) { ps.pos = psStart; return false; }
		else { ps.pos = psStart; }
		
		if ( ! parse::any(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool character(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( character_1(ps) ) { return true; }
		else { ps.pos = psStart; }
		
		if ( character_2(ps) ) { return true; }
		else { ps.pos = psStart; return false; }
	}

	bool OUT_BEGIN(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( ! ('{' == ps[ps.pos++] && '%' == ps[ps.pos++]) ) { ps.pos = psStart; return false; }

		return true;
	}

	bool OUT_END(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( ! ('%' == ps[ps.pos++] && '}' == ps[ps.pos++]) ) { ps.pos = psStart; return false; }

		return true;
	}

	bool BIND(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( ! parse::matches<':'>(ps) ) { ps.pos = psStart; return false; }

		if ( ! _(ps) ) { ps.pos = psStart; return false; }

		return true;
	}
	
	bool EQUAL(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'='>(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool PIPE(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'|'>(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool AND(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'&'>(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool NOT(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'!'>(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool OPT(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'?'>(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool STAR(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'*'>(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool PLUS(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'+'>(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool OPEN(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'('>(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool CLOSE(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<')'>(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool ANY(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'.'>(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}

	bool EMPTY(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<';'>(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool BEGIN(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'<'>(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool END(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'>'>(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! _(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool __1(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( space(ps) ) { return true; }
		else { ps.pos = psStart; }
		
		if ( comment(ps) ) { return true; }
		else { ps.pos = psStart; return false; }
	}
	
	bool _(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		while ( __1(ps) ) {}
		
		return true;
	}
	
	bool space(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( parse::matches<' '>(ps) ) { return true; }
		else { ps.pos = psStart; }
		
		if ( parse::matches<'\t'>(ps) ) { return true; }
		else { ps.pos = psStart; }
		
		if ( end_of_line(ps) ) { return true; }
		else { ps.pos = psStart; return false; }
	}
	
	bool comment_1(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( end_of_line(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! parse::any(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	bool comment(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'#'>(ps) ) { ps.pos = psStart; return false; }
		
		while ( comment_1(ps) ) {}
		
		if ( ! end_of_line(ps) ) { ps.pos = psStart; return false; }
		
		parse::ind psLen = ps.pos - psStart;
		{ std::cout << "comment [" << psStart << "," << psStart+psLen << "] `" << strings::escapeWhitespace(ps.string(psStart, psLen)) << "`" << std::endl; }
		
		return true;
	}
	
	bool end_of_line(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( [&ps]() {
			parse::ind psStart = ps.pos;
			
			if ( ! ('\r' == ps[ps.pos++] && '\n' == ps[ps.pos++]) ) { ps.pos = psStart; return false; }
			
			return true;
		}() ) { return true; }
		else { ps.pos = psStart; }
		
		if ( parse::matches<'\n'>(ps) ) { return true; }
		else { ps.pos = psStart; }
		
		if ( parse::matches<'\r'>(ps) ) { return true; }
		else { ps.pos = psStart; return false; }
	}
	
	bool end_of_file(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( parse::any(ps) ) { ps.pos = psStart; return false; }
		else { ps.pos = psStart; }
		
		{ std::cout << "end_of_file [" << psStart << "]" << std::endl; }
		
		return true;
	}

} /* namespace egg */

