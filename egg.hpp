#pragma once

//--
#include <string>

#include "ast.hpp"
#include "utils/strings.hpp"
//--

#include <string>

#include "parse.hpp"

namespace egg {

	parse::result<ast::grammar_ptr> grammar(parse::state&);
	parse::result<std::string> out_action(parse::state&);
	parse::result<ast::grammar_rule_ptr> rule(parse::state&);
	parse::result<std::string> identifier(parse::state&);
	parse::result<std::string> type_id(parse::state&);
	parse::result<ast::alt_matcher_ptr> choice(parse::state&);
	parse::result<ast::seq_matcher_ptr> sequence(parse::state&);
	parse::result<ast::matcher_ptr> expression(parse::state&);
	parse::result<ast::matcher_ptr> primary(parse::state&);
	parse::result<ast::action_matcher_ptr> action(parse::state&);
	parse::result<ast::char_matcher_ptr> char_literal(parse::state&);
	parse::result<ast::str_matcher_ptr> str_literal(parse::state&);
	parse::result<ast::range_matcher_ptr> char_class(parse::state&);
	parse::result<ast::char_range> characters(parse::state&);
	parse::result<char> character(parse::state&);

	parse::result<parse::value> OUT_BEGIN(parse::state&);
	parse::result<parse::value> OUT_END(parse::state&);
	parse::result<parse::value> BIND(parse::state&);
	parse::result<parse::value> EQUAL(parse::state&);
	parse::result<parse::value> PIPE(parse::state&);
	parse::result<parse::value> AND(parse::state&);
	parse::result<parse::value> NOT(parse::state&);
	parse::result<parse::value> OPT(parse::state&);
	parse::result<parse::value> STAR(parse::state&);
	parse::result<parse::value> PLUS(parse::state&);
	parse::result<parse::value> OPEN(parse::state&);
	parse::result<parse::value> CLOSE(parse::state&);
	parse::result<parse::value> ANY(parse::state&);
	parse::result<parse::value> EMPTY(parse::state&);
	parse::result<parse::value> BEGIN(parse::state&);
	parse::result<parse::value> END(parse::state&);
	
	parse::result<parse::value> _(parse::state&);
	parse::result<parse::value> space(parse::state&);
	parse::result<parse::value> comment(parse::state&);
	parse::result<parse::value> end_of_line(parse::state&);
	parse::result<parse::value> end_of_file(parse::state&);
	

	 parse::result<ast::grammar_ptr> grammar(parse::state& ps) {
		parse::ind psStart = ps.pos;
		ast::grammar_ptr psVal;
		
		std::string s;
		ast::grammar_rule_ptr r;

		{ psVal = ast::make_ptr<ast::grammar>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<ast::grammar_ptr>(); }

		if ( parse::bind(out_action, ps, s) ) { psVal->pre = s; }
		
		if ( parse::bind(rule, ps, r) ) { *psVal += r; } 
		else { ps.pos = psStart; return parse::fail<ast::grammar_ptr>(); }
		while ( parse::bind(rule, ps, r) ) { *psVal += r; }

		if ( parse::bind(out_action, ps, s) ) { psVal->post = s; }
		
		if ( ! end_of_file(ps) ) { ps.pos = psStart; return parse::fail<ast::grammar_ptr>(); }
		
		parse::ind psLen = ps.pos - psStart;
		//{ std::cout << "grammar [" << psStart << "," << psStart+psLen << "]" << std::endl; }
		
		return parse::match(psVal);
	}

	bool out_action_1(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( OUT_END(ps) ) { ps.pos = psStart; return false; }

		if ( ! parse::any(ps) ) { ps.pos = psStart; return false; }

		return true;
	}

	parse::result<std::string> out_action(parse::state& ps) {
		parse::ind psStart = ps.pos;
		std::string psVal;

		if ( ! OUT_BEGIN(ps) ) { ps.pos = psStart; return parse::fail<std::string>(); }

		parse::ind psCatch = ps.pos;

		while ( out_action_1(ps) ) {}

		parse::ind psCatchLen = ps.pos - psCatch;
		std::string psCapture(ps.string(psCatch, psCatchLen));

		if ( ! OUT_END(ps) ) { ps.pos = psStart; return parse::fail<std::string>(); }

		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<std::string>(); }

		{ psVal = psCapture; }
		
		//{ std::cout << "out_action `" << ps.string(psCatch, psCatchLen) << "`" << std::endl; }

		return parse::match(psVal);
	}

	bool rule_1(parse::state& ps, ast::grammar_rule_ptr& psVal) {
		parse::ind psStart = ps.pos;
		std::string s;

		if ( ! BIND(ps) ) { ps.pos = psStart; return false; }

		if ( parse::bind(type_id, ps, s) ) { psVal->type = s; }
		else { ps.pos = psStart; return false; }

		return true;
	}
	
	parse::result<ast::grammar_rule_ptr> rule(parse::state& ps) {
		parse::ind psStart = ps.pos;
		ast::grammar_rule_ptr psVal;
		std::string s;
		ast::alt_matcher_ptr m;
		
		if ( parse::bind(identifier, ps, s) ) { psVal = ast::make_ptr<ast::grammar_rule>(s); } 
		else { ps.pos = psStart; return parse::fail<ast::grammar_rule_ptr>(); }

		rule_1(ps, psVal);
		
		if ( ! EQUAL(ps) ) { ps.pos = psStart; return parse::fail<ast::grammar_rule_ptr>(); }
		
		if ( parse::bind(choice, ps, m) ) { psVal->m = m; }
		else { ps.pos = psStart; return parse::fail<ast::grammar_rule_ptr>(); }
		
		parse::ind psLen = ps.pos - psStart;
		//{ std::cout << "rule [" << psStart << "," << psStart+psLen << "]" << std::endl; }
		
		return parse::match(psVal);
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
	
	parse::result<std::string> identifier(parse::state& ps) {
		parse::ind psStart = ps.pos;
		std::string psVal;
		
		parse::ind psCatch = ps.pos;
		
		if ( ! identifier_1(ps) ) { ps.pos = psStart; return parse::fail<std::string>(); }
		
		while ( identifier_2(ps) ) {}
		
		parse::ind psCatchLen = ps.pos - psCatch;
		std::string psCapture(ps.string(psCatch, psCatchLen));
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<std::string>(); }

		{ psVal = psCapture; }
		
		//{ std::cout << "identifier `" << psCapture << "`" << std::endl; }
		
		return parse::match(psVal);
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

	parse::result<std::string> type_id(parse::state& ps) {
		parse::ind psStart = ps.pos;
		std::string psVal;
		parse::ind psCatch = ps.pos;

		if ( ! identifier(ps) ) { ps.pos = psStart; return parse::fail<std::string>(); }

		while ( type_id_1(ps) ) {}

		type_id_2(ps);

		parse::ind psCatchLen = ps.pos - psCatch;
		std::string psCapture(ps.string(psCatch, psCatchLen));

		{ psVal = psCapture; }

		//{ std::cout << "type_id `" << psCapture << "`" << std::endl; }

		return parse::match(psVal);
	}
	
	bool choice_1(parse::state& ps, ast::alt_matcher_ptr& psVal) {
		parse::ind psStart = ps.pos;
		ast::seq_matcher_ptr m;
		
		if ( ! PIPE(ps) ) { ps.pos = psStart; return false; }
		
		if ( parse::bind(sequence, ps, m) ) { *psVal += m; }
		else { ps.pos = psStart; return false; }
		
		return true;
	}
	
	parse::result<ast::alt_matcher_ptr> choice(parse::state& ps) {
		parse::ind psStart = ps.pos;
		ast::alt_matcher_ptr psVal;
		ast::seq_matcher_ptr m;
		
		if ( parse::bind(sequence, ps, m) ) { psVal = ast::make_ptr<ast::alt_matcher>(); *psVal += m; } 
		else { ps.pos = psStart; return parse::fail<ast::alt_matcher_ptr>(); }
		
		while ( choice_1(ps, psVal) ) {}
		
		parse::ind psLen = ps.pos - psStart;
		//{ std::cout << "choice [" << psStart << "," << psStart+psLen << "]" << std::endl; }
		
		return parse::match(psVal);
	}

	bool sequence_1(parse::state& ps, ast::seq_matcher_ptr& psVal) {
		parse::ind psStart = ps.pos;
		ast::matcher_ptr e;
		ast::action_matcher_ptr a;

		if ( parse::bind(expression, ps, e) ) { *psVal += e; return true; }
		else { ps.pos = psStart; }

		if ( parse::bind(action, ps, a) ) { *psVal += a; return true; }
		else { ps.pos = psStart; return false; }
	}

	parse::result<ast::seq_matcher_ptr> sequence(parse::state& ps) {
		parse::ind psStart = ps.pos;
		ast::seq_matcher_ptr psVal;

		{ psVal = ast::make_ptr<ast::seq_matcher>(); }
		if ( ! sequence_1(ps, psVal) ) { ps.pos = psStart; return parse::fail<ast::seq_matcher_ptr>(); }

		while ( sequence_1(ps, psVal) ) {}

		parse::ind psLen = ps.pos - psStart;
		//{ std::cout << "sequence [" << psStart << "," << psStart+psLen << "]" << std::endl; }

		return parse::match(psVal);
	}
	
	bool expression_1(parse::state& ps, ast::matcher_ptr& psVal, ast::matcher_ptr& m) {
		parse::ind psStart = ps.pos;
		
		if ( OPT(ps) ) { psVal = ast::make_ptr<ast::opt_matcher>(m); return true; }
		else { ps.pos = psStart; }
		
		if ( STAR(ps) ) { psVal = ast::make_ptr<ast::many_matcher>(m); return true; }
		else { ps.pos = psStart; }
		
		if ( PLUS(ps) ) { psVal = ast::make_ptr<ast::some_matcher>(m); return true; }
		else { ps.pos = psStart; return false; }
	}
	
	parse::result<ast::matcher_ptr> expression(parse::state& ps) {
		parse::ind psStart = ps.pos;
		ast::matcher_ptr psVal;
		ast::matcher_ptr m;

		if ( AND(ps) && parse::bind(primary, ps, m) ) { 
			//{ std::cout << "expression [" << psStart << "," << ps.pos << "] `" << strings::escape(ps.string(psStart, ps.pos-psStart)) << "`" << std::endl; }
			{ psVal = ast::make_ptr<ast::look_matcher>(m); }
			return parse::match(psVal);
		} else { ps.pos = psStart; }
		
		if ( NOT(ps) && parse::bind(primary, ps, m) ) { 
			//{ std::cout << "expression [" << psStart << "," << ps.pos << "] `" << strings::escape(ps.string(psStart, ps.pos-psStart)) << "`" << std::endl; }
			{ psVal = ast::make_ptr<ast::not_matcher>(m); }
			return parse::match(psVal);
		} else { ps.pos = psStart; }

		if ( parse::bind(primary, ps, m) ) {
			{ psVal = m; }
			expression_1(ps, psVal, m);
			//{ std::cout << "expression [" << psStart << "," << ps.pos << "] `" << strings::escape(ps.string(psStart, ps.pos-psStart)) << "`" << std::endl; }
			return parse::match(psVal);
		} else { ps.pos = psStart; return parse::fail<ast::matcher_ptr>(); }
	}

	bool primary_1_1_1(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( ! (BIND(ps) && type_id(ps))  ) { ps.pos = psStart; return false; }

		return true;
	}

	bool primary_1_1(parse::state& ps) {
		parse::ind psStart = ps.pos;

		primary_1_1_1(ps);

		if ( ! EQUAL(ps) ) { ps.pos = psStart; return false; }

		return true;
	}

	bool primary_1_2(parse::state& ps, ast::matcher_ptr& psVal) {
		parse::ind psStart = ps.pos;
		std::string s;

		if ( (BIND(ps) && parse::bind(identifier, ps, s)) ) { ast::as_ptr<ast::rule_matcher>(psVal)->var = s; }
		else { ps.pos = psStart; return false; }

		return true;
	}
	
	bool primary_1(parse::state& ps, ast::matcher_ptr& psVal) {
		parse::ind psStart = ps.pos;
		std::string s;
		
		if ( ! parse::bind(identifier, ps, s) ) { ps.pos = psStart; return false; }

		if ( primary_1_1(ps) ) { ps.pos = psStart; return false; }

		{ psVal = ast::make_ptr<ast::rule_matcher>(s); }
		
		primary_1_2(ps, psVal);
		
		return true;
	}
	
	parse::result<ast::matcher_ptr> primary(parse::state& ps) {
		parse::ind psStart = ps.pos;
		ast::matcher_ptr psVal;
		ast::alt_matcher_ptr am;
		ast::char_matcher_ptr cm;
		ast::str_matcher_ptr sm;
		ast::range_matcher_ptr rm;
		ast::seq_matcher_ptr bm;
		
		if ( primary_1(ps, psVal) ) { return parse::match(psVal); }
		else { ps.pos = psStart; }
		
		if ( OPEN(ps) && parse::bind(choice, ps, am) && CLOSE(ps) ) { psVal = am; return parse::match(psVal); }
		else { ps.pos = psStart; }
		
		if ( parse::bind(char_literal, ps, cm) ) { psVal = cm; return parse::match(psVal); }
		else { ps.pos = psStart; }
		
		if ( parse::bind(str_literal, ps, sm) ) { psVal = sm; return parse::match(psVal); }
		else { ps.pos = psStart; }
		
		if ( parse::bind(char_class, ps, rm) ) { psVal = rm; return parse::match(psVal); }
		else { ps.pos = psStart; }
		
		if ( ANY(ps) ) { psVal = ast::make_ptr<ast::any_matcher>(); return parse::match(psVal); }
		else { ps.pos = psStart; }

		if ( EMPTY(ps) ) { psVal = ast::make_ptr<ast::empty_matcher>(); return parse::match(psVal); }
		else { ps.pos = psStart; }
		
		if ( BEGIN(ps) && parse::bind(sequence, ps, bm) && END(ps) ) { psVal = bm; return parse::match(psVal); }
		else { ps.pos = psStart; return parse::fail<ast::matcher_ptr>(); }
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

	parse::result<ast::action_matcher_ptr> action(parse::state& ps) {
		parse::ind psStart = ps.pos;
		ast::action_matcher_ptr psVal;

		if ( ! (parse::matches<'{'>(ps)) ) { ps.pos = psStart; return parse::fail<ast::action_matcher_ptr>(); }

		parse::ind psCatch = ps.pos;

		while ( action_1(ps) ) {}

		parse::ind psCatchLen = ps.pos - psCatch;
		std::string psCapture(ps.string(psCatch, psCatchLen));

		if ( ! (parse::matches<'}'>(ps)) ) { ps.pos = psStart; return parse::fail<ast::action_matcher_ptr>(); }

		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<ast::action_matcher_ptr>(); }

		//{ std::cout << "action `" << psCapture << "`" << std::endl; }
		{ psVal = ast::make_ptr<ast::action_matcher>(psCapture); }

		return parse::match(psVal);
	}
	
	parse::result<ast::char_matcher_ptr> char_literal(parse::state& ps) {
		parse::ind psStart = ps.pos;
		ast::char_matcher_ptr psVal;
		
		if ( ! (parse::matches<'\''>(ps)) ) { ps.pos = psStart; return parse::fail<ast::char_matcher_ptr>(); }
		
		parse::ind psCatch = ps.pos;
		
		if ( ! character(ps) ) { ps.pos = psStart; return parse::fail<ast::char_matcher_ptr>(); }
		
		parse::ind psCatchLen = ps.pos - psCatch;
		std::string psCapture(ps.string(psCatch, psCatchLen));
		
		if ( ! (parse::matches<'\''>(ps)) ) { ps.pos = psStart; return parse::fail<ast::char_matcher_ptr>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<ast::char_matcher_ptr>(); }
		
		//{ std::cout << "char_literal `" << psCapture << "`" << std::endl; }
		{ psVal = ast::make_ptr<ast::char_matcher>(ps[psCatch]); }
		
		return parse::match(psVal);
	}
	
	parse::result<ast::str_matcher_ptr> str_literal(parse::state& ps) {
		parse::ind psStart = ps.pos;
		ast::str_matcher_ptr psVal;
		
		if ( ! (parse::matches<'\"'>(ps)) ) { ps.pos = psStart; return parse::fail<ast::str_matcher_ptr>(); }
		
		parse::ind psCatch = ps.pos;
		
		while ( character(ps) ) {}
		
		parse::ind psCatchLen = ps.pos - psCatch;
		std::string psCapture(ps.string(psCatch, psCatchLen));
		
		if ( ! (parse::matches<'\"'>(ps)) ) { ps.pos = psStart; return parse::fail<ast::str_matcher_ptr>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<ast::str_matcher_ptr>(); }
		
		//{ std::cout << "str_literal `" << psCapture << "`" << std::endl; }
		{ psVal = ast::make_ptr<ast::str_matcher>(psCapture); }
		
		return parse::match(psVal);
	}
	
	bool char_class_1(parse::state& ps, ast::range_matcher_ptr& psVal) {
		parse::ind psStart = ps.pos;
		ast::char_range r;
		
		if ( parse::matches<']'>(ps) ) { ps.pos = psStart; return false; }
		else { ps.pos = psStart; }
		
		if ( parse::bind(characters, ps, r) ) { *psVal += r; }
		else { return false; }
		
		return true;
	}
	
	parse::result<ast::range_matcher_ptr> char_class(parse::state& ps) {
		parse::ind psStart = ps.pos;
		ast::range_matcher_ptr psVal;
		
		if ( ! (parse::matches<'['>(ps)) ) { ps.pos = psStart; return parse::fail<ast::range_matcher_ptr>(); }

		{ psVal = ast::make_ptr<ast::range_matcher>(); }
		
		parse::ind psCatch = ps.pos;
		
		while ( char_class_1(ps, psVal) ) {}
		
		parse::ind psCatchLen = ps.pos - psCatch;
		std::string psCapture(ps.string(psCatch, psCatchLen));
		
		if ( ! (parse::matches<']'>(ps)) ) { ps.pos = psStart; return parse::fail<ast::range_matcher_ptr>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<ast::range_matcher_ptr>(); }
		
		//{ std::cout << "char_class `" << psCapture << "`" << std::endl; }
		
		return parse::match(psVal);
	}
	
	parse::result<ast::char_range> characters(parse::state& ps) {
		parse::ind psStart = ps.pos;
		ast::char_range psVal;
		char f, t, c;
		
		if ( parse::bind(character, ps, f) && parse::matches<'-'>(ps) && parse::bind(character, ps, t) ) { psVal = ast::char_range(f, t); return parse::match(psVal); }
		else { ps.pos = psStart; }
		
		if ( parse::bind(character, ps, c) ) { psVal = ast::char_range(c); return parse::match(psVal); }
		else { ps.pos = psStart; return parse::fail<ast::char_range>(); }
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
	
	parse::result<char> character(parse::state& ps) {
		parse::ind psStart = ps.pos;
		char psVal;
		
		if ( character_1(ps) ) { psVal = strings::unescaped_char(ps[psStart+1]); return parse::match(psVal); }
		else { ps.pos = psStart; }
		
		if ( character_2(ps) ) { psVal = ps[psStart]; return parse::match(psVal); }
		else { ps.pos = psStart; return parse::fail<char>(); }
	}

	parse::result<parse::value> OUT_BEGIN(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( ! ('{' == ps[ps.pos++] && '%' == ps[ps.pos++]) ) { ps.pos = psStart; return parse::fail<parse::value>(); }

		return parse::match(parse::val);
	}

	parse::result<parse::value> OUT_END(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( ! ('%' == ps[ps.pos++] && '}' == ps[ps.pos++]) ) { ps.pos = psStart; return parse::fail<parse::value>(); }

		return parse::match(parse::val);
	}

	parse::result<parse::value> BIND(parse::state& ps) {
		parse::ind psStart = ps.pos;

		if ( ! parse::matches<':'>(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }

		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }

		return parse::match(parse::val);
	}
	
	parse::result<parse::value> EQUAL(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'='>(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		return parse::match(parse::val);
	}
	
	parse::result<parse::value> PIPE(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'|'>(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		return parse::match(parse::val);
	}
	
	parse::result<parse::value> AND(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'&'>(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		return parse::match(parse::val);
	}
	
	parse::result<parse::value> NOT(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'!'>(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		return parse::match(parse::val);
	}
	
	parse::result<parse::value> OPT(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'?'>(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		return parse::match(parse::val);
	}
	
	parse::result<parse::value> STAR(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'*'>(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		return parse::match(parse::val);
	}
	
	parse::result<parse::value> PLUS(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'+'>(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		return parse::match(parse::val);
	}
	
	parse::result<parse::value> OPEN(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'('>(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		return parse::match(parse::val);
	}
	
	parse::result<parse::value> CLOSE(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<')'>(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		return parse::match(parse::val);
	}
	
	parse::result<parse::value> ANY(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'.'>(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		return parse::match(parse::val);
	}

	parse::result<parse::value> EMPTY(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<';'>(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		return parse::match(parse::val);
	}
	
	parse::result<parse::value> BEGIN(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'<'>(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		return parse::match(parse::val);
	}
	
	parse::result<parse::value> END(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'>'>(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		if ( ! _(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		return parse::match(parse::val);
	}
	
	bool __1(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( space(ps) ) { return true; }
		else { ps.pos = psStart; }
		
		if ( comment(ps) ) { return true; }
		else { ps.pos = psStart; return false; }
	}
	
	parse::result<parse::value> _(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		while ( __1(ps) ) {}
		
		return parse::match(parse::val);
	}
	
	parse::result<parse::value> space(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( parse::matches<' '>(ps) ) { return parse::match(parse::val); }
		else { ps.pos = psStart; }
		
		if ( parse::matches<'\t'>(ps) ) { return parse::match(parse::val); }
		else { ps.pos = psStart; }
		
		if ( end_of_line(ps) ) { return parse::match(parse::val); }
		else { ps.pos = psStart; return parse::fail<parse::value>(); }
	}
	
	bool comment_1(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( end_of_line(ps) ) { ps.pos = psStart; return false; }
		
		if ( ! parse::any(ps) ) { ps.pos = psStart; return false; }
		
		return true;
	}
	
	parse::result<parse::value> comment(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( ! parse::matches<'#'>(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		while ( comment_1(ps) ) {}
		
		if ( ! end_of_line(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		
		parse::ind psLen = ps.pos - psStart;
		//{ std::cout << "comment [" << psStart << "," << psStart+psLen << "] `" << strings::escape(ps.string(psStart, psLen)) << "`" << std::endl; }
		
		return parse::match(parse::val);
	}
	
	parse::result<parse::value> end_of_line(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( [&ps]() {
			parse::ind psStart = ps.pos;
			
			if ( ! ('\r' == ps[ps.pos++] && '\n' == ps[ps.pos++]) ) { ps.pos = psStart; return false; }
			
			return true;
		}() ) { return parse::match(parse::val); }
		else { ps.pos = psStart; }
		
		if ( parse::matches<'\n'>(ps) ) { return parse::match(parse::val); }
		else { ps.pos = psStart; }
		
		if ( parse::matches<'\r'>(ps) ) { return parse::match(parse::val); }
		else { ps.pos = psStart; return parse::fail<parse::value>(); }
	}
	
	parse::result<parse::value> end_of_file(parse::state& ps) {
		parse::ind psStart = ps.pos;
		
		if ( parse::any(ps) ) { ps.pos = psStart; return parse::fail<parse::value>(); }
		else { ps.pos = psStart; }
		
		//{ std::cout << "end_of_file [" << psStart << "]" << std::endl; }
		
		return parse::match(parse::val);
	}

} /* namespace egg */

