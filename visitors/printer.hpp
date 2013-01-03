#pragma once

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
		}
		
		void visit(ast::rule_matcher& m) {
			out << m.rule;
			if ( ! m.var.empty() ) {
				out << " : " << m.var;
			}
		}

		void visit(ast::any_matcher& m) {
			out << ".";
		}

		void visit(ast::empty_matcher& m) {
			out << ";";
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
			if ( m.ms.size() != 1 ) out << " )";
		}

		void visit(ast::alt_matcher& m) {
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
			out << " >";
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
		std::ostream& out;	/**< output stream */
		int tabs;			/**< current number of tab stops */
	};
} /* namespace visitor */

