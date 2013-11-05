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
	
	bool grammar(parser::state& ps, ast::grammar_ptr& psVal) {
		ast::grammar_rule_ptr  r;
		std::string  s;
		
		return 
		parser::named("grammar",
			parser::sequence({
				[&](parser::state& ps) { psVal = ast::make_ptr<ast::grammar>(); return true; },
				_,
				parser::option(
					parser::sequence({
						parser::bind(s, out_action),
						[&](parser::state& ps) { psVal->pre = s; return true; }})),
				parser::some(
					parser::sequence({
						parser::bind(r, rule),
						[&](parser::state& ps) { *psVal += r; return true; }})),
				parser::option(
					parser::sequence({
						parser::bind(s, out_action),
						[&](parser::state& ps) { psVal->post = s; return true; }})),
				end_of_file}))(ps);
	}

	bool out_action(parser::state& ps, std::string& psVal) {
		return 
		parser::named("out_action",
			parser::sequence({
				OUT_BEGIN,
				parser::capture(psVal, 
					parser::many(
						parser::sequence({
							parser::look_not(OUT_END),
							parser::any()}))),
				OUT_END,
				_}))(ps);
	}

	bool rule(parser::state& ps, ast::grammar_rule_ptr& psVal) {
		ast::alt_matcher_ptr  m;
		std::string  s;

		return 
		parser::named("rule", 
			parser::sequence({
				parser::bind(s, identifier),
				[&](parser::state& ps) { psVal = ast::make_ptr<ast::grammar_rule>(s); return true; },
				parser::option(
					parser::sequence({
						BIND,
						parser::bind(s, type_id),
						[&](parser::state& ps) { psVal->type = s; return true; }})),
				EQUAL,
				parser::bind(m, choice),
				[&](parser::state& ps) { psVal->m = m; return true; }}))(ps);
	}

	bool identifier(parser::state& ps, std::string& psVal) {
		return parser::named("identifier", 
			parser::sequence({
				parser::capture(psVal,
					parser::sequence({
						parser::choice({
							parser::between('A', 'Z'),
							parser::between('a', 'z'),
							parser::literal('_')}),
						parser::many(
							parser::choice({
								parser::between('A', 'Z'),
								parser::between('a', 'z'),
								parser::literal('_'),
								parser::between('0', '9')}))})),
				_}))(ps);
	}

	bool type_id(parser::state& ps, std::string& psVal) {
		return parser::named("type_id", 
			parser::capture(psVal, 
				parser::sequence({
					parser::unbind(identifier),
					parser::many(
						parser::sequence({
							parser::literal("::"),
							_,
							parser::unbind(type_id)})),
					parser::option(
						parser::sequence({
							parser::literal('<'),
							_,
							parser::unbind(type_id),
							parser::many(
								parser::sequence({
									parser::literal(','),
									_,
									parser::unbind(type_id)})),
							parser::literal('>'),
							_}))})))(ps);
	}

	bool choice(parser::state& ps, ast::alt_matcher_ptr& psVal) {
		ast::seq_matcher_ptr  m;

		return parser::named("choice",
			parser::sequence({
				parser::bind(m, sequence),
				[&](parser::state& ps) { psVal = ast::make_ptr<ast::alt_matcher>(); *psVal += m; return true; },
				parser::many(
					parser::sequence({
						PIPE,
						parser::bind(m, sequence),
						[&](parser::state& ps) { *psVal += m; return true; }}))}))(ps);
	}

	bool sequence(parser::state& ps, ast::seq_matcher_ptr& psVal) {
		ast::action_matcher_ptr  a;
		ast::matcher_ptr  e;

		return parser::named("sequence", 
			parser::sequence({
				[&](parser::state& ps) { psVal = ast::make_ptr<ast::seq_matcher>(); return true; },
				parser::some(
					parser::choice({
						parser::sequence({
							parser::bind(e, expression),
							[&](parser::state& ps) { *psVal += e; return true; }}),
						parser::sequence({
							parser::bind(a, action),
							[&](parser::state& ps) { *psVal += a; return true; }})}))}))(ps);
	}

	bool expression(parser::state& ps, ast::matcher_ptr& psVal) {
		ast::matcher_ptr  m;

		return parser::named("expression",
			parser::choice({
				parser::sequence({
					AND,
					parser::bind(m, primary),
					[&](parser::state& ps) { psVal = ast::make_ptr<ast::look_matcher>(m); return true; }}),
				parser::sequence({
					NOT,
					parser::bind(m, primary),
					[&](parser::state& ps) { psVal = ast::make_ptr<ast::not_matcher>(m); return true; }}),
				parser::sequence({
					parser::bind(m, primary),
					[&](parser::state& ps) { psVal = m; return true; },
					parser::option(
						parser::choice({
							parser::sequence({
								OPT,
								[&](parser::state& ps) { psVal = ast::make_ptr<ast::opt_matcher>(m); return true; }}),
							parser::sequence({
								STAR,
								[&](parser::state& ps) { psVal = ast::make_ptr<ast::many_matcher>(m); return true; }}),
							parser::sequence({
								PLUS,
								[&](parser::state& ps) { psVal = ast::make_ptr<ast::some_matcher>(m); return true; }})}))})}))(ps);
	}

	bool primary(parser::state& ps, ast::matcher_ptr& psVal) {
		ast::alt_matcher_ptr  am;
		ast::seq_matcher_ptr  bm;
		ast::char_matcher_ptr  cm;
		ast::range_matcher_ptr  rm;
		std::string  s;
		ast::str_matcher_ptr  sm;
		std::string  t;

		return parser::named("primary", 
			parser::choice({
				parser::sequence({
					parser::bind(s, identifier),
					parser::look_not(
						parser::sequence({
							parser::option(
								parser::sequence({
									BIND,
									parser::unbind(type_id)})),
							EQUAL})),
					[&](parser::state& ps) { psVal = ast::make_ptr<ast::rule_matcher>(s); return true; },
					parser::option(
						parser::sequence({
							BIND,
							parser::bind(s, identifier),
							[&](parser::state& ps) { ast::as_ptr<ast::rule_matcher>(psVal)->var = s; return true; }}))}),
				parser::sequence({
					OPEN,
					parser::bind(am, choice),
					CLOSE,
					[&](parser::state& ps) { psVal = am; return true; }}),
				parser::sequence({
					parser::bind(cm, char_literal),
					[&](parser::state& ps) { psVal = cm; return true; }}),
				parser::sequence({
					parser::bind(sm, str_literal),
					[&](parser::state& ps) { psVal = sm; return true; }}),
				parser::sequence({
					parser::bind(rm, char_class),
					[&](parser::state& ps) { psVal = rm; return true; },
					parser::option(
						parser::sequence({
							BIND,
							parser::bind(t, identifier),
							[&](parser::state& ps) { ast::as_ptr<ast::range_matcher>(psVal)->var = t; return true; }}))}),
				parser::sequence({
					ANY,
					[&](parser::state& ps) { psVal = ast::make_ptr<ast::any_matcher>(); return true; },
					parser::option(
						parser::sequence({
							BIND,
							parser::bind(t, identifier), 
							[&](parser::state& ps) { ast::as_ptr<ast::any_matcher>(psVal)->var = t; return true; }}))}),
				parser::sequence({
					EMPTY,
					[&](parser::state& ps) { psVal = ast::make_ptr<ast::empty_matcher>(); return true; }}),
				parser::sequence({
					BEGIN,
					parser::bind(bm, sequence),
					END,
					BIND,
					parser::bind(t, identifier),
					[&](parser::state& ps) { psVal = ast::make_ptr<ast::capt_matcher>(bm, t); return true; }})}))(ps);
	}

	bool action(parser::state& ps, ast::action_matcher_ptr& psVal) {
		std::string s;
		
		return parser::named("action",
			parser::sequence({
				parser::look_not(OUT_BEGIN),
				parser::literal('{'),
				parser::capture(s,
					parser::many(
						parser::choice({
							parser::unbind(action),
							parser::sequence({
								parser::look_not(parser::literal('}')),
								parser::any()})}))),
				parser::literal('}'),
				_,
				[&](parser::state& ps) { psVal = ast::make_ptr<ast::action_matcher>(s); return true; }}))(ps);
	}

	bool char_literal(parser::state& ps, ast::char_matcher_ptr& psVal) {
		char  c;

		return parser::named("char_literal",
			parser::sequence({
				parser::literal('\''),
				parser::bind(c, character),
				parser::literal('\''),
				_,
				[&](parser::state& ps) { psVal = ast::make_ptr<ast::char_matcher>(c); return true; }}))(ps);
	}

	bool str_literal(parser::state& ps, ast::str_matcher_ptr& psVal) {
		std::string s;

		return parser::named("str_literal", 
			parser::sequence({
				parser::literal('\"'),
				parser::capture(s, 
					parser::many(parser::unbind(character))),
				parser::literal('\"'),
				_,
				[&](parser::state& ps) { psVal = ast::make_ptr<ast::str_matcher>(strings::unescape(s)); return true; }}))(ps);
	}

	bool char_class(parser::state& ps, ast::range_matcher_ptr& psVal) {
		ast::char_range  r;

		return parser::named("char_class",
			parser::sequence({
				parser::literal('['),
				[&](parser::state& ps) { psVal = ast::make_ptr<ast::range_matcher>(); return true; },
				parser::many(
					parser::sequence({
						parser::look_not(parser::literal(']')),
						parser::bind(r, characters),
						[&](parser::state& ps) { *psVal += r; return true; }})),
				parser::literal(']'),
				_}))(ps);
	}

	bool characters(parser::state& ps, ast::char_range& psVal) {
		char  c;
		char  f;
		char  t;

		return parser::choice({
			parser::sequence({
				parser::bind(f, character),
				parser::literal('-'),
				parser::bind(t, character),
				[&](parser::state& ps) { psVal = ast::char_range(f, t); return true; }}),
			parser::sequence({
				parser::bind(c, character),
				[&](parser::state& ps) { psVal = ast::char_range(c); return true; }})})(ps);
	}

	bool character(parser::state& ps, char& psVal) {
		char c;
		
		return 
			parser::choice({
				parser::sequence({
					parser::literal('\\'),
					parser::choice({
						parser::literal('n', c),
						parser::literal('r', c),
						parser::literal('t', c),
						parser::literal('\'', c),
						parser::literal('\"', c),
						parser::literal('\\', c)}),
					[&](parser::state& ps) { psVal = strings::unescaped_char(c); return true; }}),
				parser::sequence({
					parser::look_not(
						parser::choice({
							parser::literal('\''),
							parser::literal('\"'),
							parser::literal('\\')})),
					parser::any(psVal)})})(ps);
	}

	bool OUT_BEGIN(parser::state& ps) {
		return parser::literal("{%")(ps);
	}

	bool OUT_END(parser::state& ps) {
		return parser::literal("%}")(ps);
	}

	bool BIND(parser::state& ps) {
		return 
			parser::sequence({
				parser::literal(':'),
				_})(ps);
	}

	bool EQUAL(parser::state& ps) {
		return 
			parser::sequence({
				parser::literal('='),
				_})(ps);
	}

	bool PIPE(parser::state& ps) {
		return 
			parser::sequence({
				parser::literal('|'),
				_})(ps);
	}

	bool AND(parser::state& ps) {
		return 
			parser::sequence({
				parser::literal('&'),
				_})(ps);
	}

	bool NOT(parser::state& ps) {
		return 
			parser::sequence({
				parser::literal('!'),
				_})(ps);
	}

	bool OPT(parser::state& ps) {
		return 
			parser::sequence({
				parser::literal('?'),
				_})(ps);
	}

	bool STAR(parser::state& ps) {
		return 
			parser::sequence({
				parser::literal('*'),
				_})(ps);
	}

	bool PLUS(parser::state& ps) {
		return 
			parser::sequence({
				parser::literal('+'),
				_})(ps);
	}

	bool OPEN(parser::state& ps) {
		return 
			parser::sequence({
				parser::literal('('),
				_})(ps);
	}

	bool CLOSE(parser::state& ps) {
		return 
			parser::sequence({
				parser::literal(')'),
				_})(ps);
	}

	bool ANY(parser::state& ps) {
		return 
			parser::sequence({
				parser::literal('.'),
				_})(ps);
	}

	bool EMPTY(parser::state& ps) {
		return 
			parser::sequence({
				parser::literal(';'),
				_})(ps);
	}

	bool BEGIN(parser::state& ps) {
		return 
			parser::sequence({
				parser::literal('<'),
				_})(ps);
	}

	bool END(parser::state& ps) {
		return 
			parser::sequence({
				parser::literal('>'),
				_})(ps);
	}

	bool _(parser::state& ps) {
		return 
		parser::many(
			parser::choice({
				space,
				comment}))(ps);
	}

	bool space(parser::state& ps) {
		return 
		parser::choice({
			parser::literal(' '),
			parser::literal('\t'),
			end_of_line})(ps);
	}

	bool comment(parser::state& ps) {
		return 
		parser::sequence({
			parser::literal('#'),
			parser::many(
				parser::sequence({
					parser::look_not(end_of_line),
					parser::any()})),
			end_of_line})(ps);
	}

	bool end_of_line(parser::state& ps) {
		return 
		parser::choice({
			parser::literal("\r\n"),
			parser::literal('\n'),
			parser::literal('\r')})(ps);
	}

	bool end_of_file(parser::state& ps) {
		return parser::look_not(parser::any())(ps);
	}

} /* namespace egg */
