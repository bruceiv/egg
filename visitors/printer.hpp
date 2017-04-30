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

#include <iostream>
#include <string>

#include "../ast.hpp"
#include "../utils/strings.hpp"

namespace visitor {

	/** Pretty-printer for Egg matcher ASTs. */
	class printer : ast::visitor {
	public:
		printer(std::ostream& out = std::cout, int tabs = 0) 
			: out(out), tabs(tabs) {}

		void visit(ast::char_matcher& m) {
			out << "\'" << strings::escape(m.c) << "\'";
		}
		
		void visit(ast::str_matcher& m) {
			out << "\"" << strings::escape(m.s) << "\"";
		}

		void visit(ast::range_matcher& m) {
			out << "[";

			for (auto iter = m.rs.begin(); iter != m.rs.end(); ++iter) {
				ast::char_range& r = *iter;
				out << strings::escape(r.from);
				if ( r.from != r.to ) {
					out << "-" << strings::escape(r.to);
				}
			}
			
			out << "]";
			
			if ( ! m.var.empty() ) {
				out << " : " << m.var;
			}
		}
		
		void visit(ast::rule_matcher& m) {
			out << m.rule;
			if ( ! m.var.empty() ) {
				out << " : " << m.var;
			}
		}

		void visit(ast::any_matcher& m) {
			out << ".";
			
			if ( ! m.var.empty() ) {
				out << " : " << m.var;
			}
		}

		void visit(ast::empty_matcher& m) {
			out << ";";
		}
		
		void visit(ast::none_matcher& m) {
			out << "$";
		}

		void visit(ast::action_matcher& m) {
			out << "{" << strings::single_line(m.a) << "}";
		}

		void visit(ast::opt_matcher& m) {
			m.m->accept(this);
			out << "?";
		}

		void visit(ast::many_matcher& m) {
			m.m->accept(this);
			out << "*";
		}

		void visit(ast::some_matcher& m) {
			m.m->accept(this);
			out << "+";
		}

		void visit(ast::seq_matcher& m) {
			if ( m.ms.size() != 1 ) { out << "( "; }
			if ( ! m.ms.empty() ) {
				std::string indent((4 * ++tabs), ' ');
				
				auto iter = m.ms.begin();
				(*iter)->accept(this);
				while ( ++iter != m.ms.end() ) {
					out << std::endl << indent;
					(*iter)->accept(this);
				}
				
				--tabs;
			}
			if ( m.ms.size() != 1 ) { out << " )"; }
		}

		void visit(ast::alt_matcher& m) {
			if ( m.ms.size() != 1 ) { out << "( "; }
			if ( ! m.ms.empty() ) {
				std::string indent((4 * ++tabs), ' ');

				auto iter = m.ms.begin();
				(*iter)->accept(this);
				while ( ++iter != m.ms.end() ) {
					out << "\n" << indent << "| ";
					(*iter)->accept(this);
				}
				
				--tabs;
			}
			if ( m.ms.size() != 1 ) { out << " )"; }
		}
		
		void visit(ast::ualt_matcher& m) {
			if ( m.ms.size() != 1 ) { out << "( "; }
			if ( ! m.ms.empty() ) {
				std::string indent((4 * ++tabs), ' ');

				auto iter = m.ms.begin();
				(*iter)->accept(this);
				while ( ++iter != m.ms.end() ) {
					out << "\n" << indent << "^| ";
					(*iter)->accept(this);
				}
				
				--tabs;
			}
			if ( m.ms.size() != 1 ) { out << " )"; }
		}

		void visit(ast::look_matcher& m) {
			out << "&";
			m.m->accept(this);
		}

		void visit(ast::not_matcher& m) {
			out << "!";
			m.m->accept(this);
		}
		
		void visit(ast::capt_matcher& m) {
			out << "< ";
			m.m->accept(this);
			out << " > : " << m.var;
		}
		
		void visit(ast::named_matcher& m) {
			m.m->accept(this);
			out << "@`" << strings::unescape_error(m.error) << "`";
		}
		
		void visit(ast::fail_matcher& m) {
			out << "~`" << strings::unescape_error(m.error) << "`";
		}

		void print(ast::grammar_rule& r) {
			out << r.name;
			if ( ! r.type.empty() ) {
				out << " : " << r.type;
			}
			out << " = ";
			r.m->accept(this);
			out << std::endl;
		}

		void print(ast::grammar& g) {
			if ( ! g.pre.empty() ) {
				out << "{%" << g.pre << "%}" << std::endl;
			}

			out << std::endl;
			for (auto iter = g.rs.begin(); iter != g.rs.end(); ++iter) {
				ast::grammar_rule_ptr& r = *iter;
				print(*r);
			}
			out << std::endl;

			if ( ! g.post.empty() ) {
				out << "{%" << g.post << "%}" <<std::endl << std::endl;
			}
		}
		
	private:
		std::ostream& out;	///< output stream
		int tabs;			///< current number of tab stops
	};
} /* namespace visitor */

