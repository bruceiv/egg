#pragma once

/*
 * Copyright (c) 2013 Aaron Moss
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
 
#include <string>

#include "ast.hpp"
#include "utils/strings.hpp"

#include "parser.hpp"

namespace egg {

	bool grammar(parser::state&, ast::grammar_ptr&);
	bool out_action(parser::state&, std::string&);
	bool rule(parser::state&, ast::grammar_rule_ptr&);
	bool identifier(parser::state&, std::string&);
	bool type_id(parser::state&, std::string&);
	bool choice(parser::state&, ast::alt_matcher_ptr&);
	bool sequence(parser::state&, ast::seq_matcher_ptr&);
	bool expression(parser::state&, ast::matcher_ptr&);
	bool primary(parser::state&, ast::matcher_ptr&);
	bool action(parser::state&, ast::action_matcher_ptr&);
	bool char_literal(parser::state&, ast::char_matcher_ptr&);
	bool str_literal(parser::state&, ast::str_matcher_ptr&);
	bool char_class(parser::state&, ast::range_matcher_ptr&);
	bool characters(parser::state&, ast::char_range&);
	bool character(parser::state&, char&);
	bool OUT_BEGIN(parser::state&);
	bool OUT_END(parser::state&);
	bool BIND(parser::state&);
	bool EQUAL(parser::state&);
	bool PIPE(parser::state&);
	bool AND(parser::state&);
	bool NOT(parser::state&);
	bool OPT(parser::state&);
	bool STAR(parser::state&);
	bool PLUS(parser::state&);
	bool OPEN(parser::state&);
	bool CLOSE(parser::state&);
	bool ANY(parser::state&);
	bool EMPTY(parser::state&);
	bool BEGIN(parser::state&);
	bool END(parser::state&);
	bool _(parser::state&);
	bool space(parser::state&);
	bool comment(parser::state&);
	bool end_of_line(parser::state&);
	bool end_of_file(parser::state&);
	
	bool grammar(parser::state& ps) {
		ast::grammar_ptr _;
		return egg::grammar(ps, _);
	}
	
	bool out_action(parser::state& ps) {
		std::string _;
		return egg::out_action(ps, _);
	}
	
	bool rule(parser::state& ps) {
		ast::grammar_rule_ptr _;
		return egg::rule(ps, _);
	}
	
	bool identifier(parser::state& ps) {
		std::string _;
		return egg::identifier(ps, _);
	}
	
	bool type_id(parser::state& ps) {
		std::string _;
		return egg::type_id(ps, _);
	}
	
	bool choice(parser::state& ps) {
		ast::alt_matcher_ptr _;
		return egg::choice(ps, _);
	}
	
	bool sequence(parser::state& ps) {
		ast::seq_matcher_ptr _;
		return egg::sequence(ps, _);
	}
	
	bool expression(parser::state& ps) {
		ast::matcher_ptr _;
		return egg::expression(ps, _);
	}
	
	bool primary(parser::state& ps) {
		ast::matcher_ptr _;
		return egg::primary(ps, _);
	}
	
	bool action(parser::state& ps) {
		ast::action_matcher_ptr _;
		return egg::action(ps, _);
	}
	
	bool char_literal(parser::state& ps) {
		ast::char_matcher_ptr _;
		return egg::char_literal(ps, _);
	}
	
	bool str_literal(parser::state& ps) {
		ast::str_matcher_ptr _;
		return egg::str_literal(ps, _);
	}
	
	bool char_class(parser::state& ps) {
		ast::range_matcher_ptr _;
		return egg::char_class(ps, _);
	}
	
	bool characters(parser::state& ps) {
		ast::char_range _;
		return egg::characters(ps, _);
	}
	
	bool character(parser::state& ps) {
		char _;
		return egg::character(ps, _);
	}

	bool grammar(parser::state& ps, ast::grammar_ptr& psVal) {
		ast::grammar_rule_ptr  r;
		std::string  s;
		
		return parser::fail("grammar not yet implemented")(ps);
	}

	bool out_action(parser::state& ps, std::string& psVal) {
		return parser::fail("out_action not yet implemented")(ps);
	}

	bool rule(parser::state& ps, ast::grammar_rule_ptr& psVal) {
		ast::alt_matcher_ptr  m;
		std::string  s;

		return parser::fail("rule not yet implemented")(ps);
	}

	bool identifier(parser::state& ps, std::string& psVal) {
		return parser::fail("identifier not yet implemented")(ps);
	}

	bool type_id(parser::state& ps, std::string& psVal) {
		return parser::fail("type_id not yet implemented")(ps);
	}

	bool choice(parser::state& ps, ast::alt_matcher_ptr& psVal) {
		ast::seq_matcher_ptr  m;

		return parser::fail("choice not yet implemented")(ps);
	}

	bool sequence(parser::state& ps, ast::seq_matcher_ptr& psVal) {
		ast::action_matcher_ptr  a;
		ast::matcher_ptr  e;

		return parser::fail("sequence not yet implemented")(ps);
	}

	bool expression(parser::state& ps, ast::matcher_ptr& psVal) {
		ast::matcher_ptr  m;

		return parser::fail("expression not yet implemented")(ps);
	}

	bool primary(parser::state& ps, ast::matcher_ptr& psVal) {
		ast::alt_matcher_ptr  am;
		ast::seq_matcher_ptr  bm;
		ast::char_matcher_ptr  cm;
		ast::range_matcher_ptr  rm;
		std::string  s;
		ast::str_matcher_ptr  sm;
		std::string  t;

		return parser::fail("primary not yet implemented")(ps);
	}

	bool action(parser::state& ps, ast::action_matcher_ptr& psVal) {
		std::string s;

		return parser::fail("action not yet implemented")(ps);
	}

	bool char_literal(parser::state& ps, ast::char_matcher_ptr& psVal) {
		char  c;

		return parser::fail("char_literal not yet implemented")(ps);
	}

	bool str_literal(parser::state& ps, ast::str_matcher_ptr& psVal) {
		std::string s;

		return parser::fail("str_literal not yet implemented")(ps);
	}

	bool char_class(parser::state& ps, ast::range_matcher_ptr& psVal) {
		ast::char_range  r;

		return parser::fail("char_class not yet implemented")(ps);
	}

	bool characters(parser::state& ps, ast::char_range& psVal) {
		char  c;
		char  f;
		char  t;

		return parser::fail("characters not yet implemented")(ps);
	}

	bool character(parser::state& ps, char& psVal) {
		return parser::fail("character not yet implemented")(ps);
	}

	bool OUT_BEGIN(parser::state& ps) {
		return parser::fail("OUT_BEGIN not yet implemented")(ps);
	}

	bool OUT_END(parser::state& ps) {
		return parser::fail("OUT_END not yet implemented")(ps);
	}

	bool BIND(parser::state& ps) {
		return parser::fail("BIND not yet implemented")(ps);
	}

	bool EQUAL(parser::state& ps) {
		return parser::fail("EQUAL not yet implemented")(ps);
	}

	bool PIPE(parser::state& ps) {
		return parser::fail("PIPE not yet implemented")(ps);
	}

	bool AND(parser::state& ps) {
		return parser::fail("AND not yet implemented")(ps);
	}

	bool NOT(parser::state& ps) {
		return parser::fail("NOT not yet implemented")(ps);
	}

	bool OPT(parser::state& ps) {
		return parser::fail("OPT not yet implemented")(ps);
	}

	bool STAR(parser::state& ps) {
		return parser::fail("STAR not yet implemented")(ps);
	}

	bool PLUS(parser::state& ps) {
		return parser::fail("PLUS not yet implemented")(ps);
	}

	bool OPEN(parser::state& ps) {
		return parser::fail("OPEN not yet implemented")(ps);
	}

	bool CLOSE(parser::state& ps) {
		return parser::fail("CLOSE not yet implemented")(ps);
	}

	bool ANY(parser::state& ps) {
		return parser::fail("ANY not yet implemented")(ps);
	}

	bool EMPTY(parser::state& ps) {
		return parser::fail("EMPTY not yet implemented")(ps);
	}

	bool BEGIN(parser::state& ps) {
		return parser::fail("BEGIN not yet implemented")(ps);
	}

	bool END(parser::state& ps) {
		return parser::fail("END not yet implemented")(ps);
	}

	bool _(parser::state& ps) {
		return parser::fail("_ not yet implemented")(ps);
	}

	bool space(parser::state& ps) {
		return parser::fail("space not yet implemented")(ps);
	}

	bool comment(parser::state& ps) {
		return parser::fail("comment not yet implemented")(ps);
	}

	bool end_of_line(parser::state& ps) {
		return parser::fail("end_of_line not yet implemented")(ps);
	}

	bool end_of_file(parser::state& ps) {
		return parser::fail("end_of_file not yet implemented")(ps);
	}

} /* namespace egg */
