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
#include <map>
#include <string>
#include <unordered_map>
#include <utility>

#include "../ast.hpp"
#include "../utils/strings.hpp"

namespace visitor {

	/** Gets a list of variables declared in a grammar rule. */
	class variable_list : ast::visitor {
	public:
		variable_list() {}

		variable_list(const ast::grammar& g) {
			for (auto it = g.rs.begin(); it != g.rs.end(); ++it) {
				ast::grammar_rule& r = **it;
				types.insert(std::make_pair(r.name, r.type));
			}
		}

		void visit(ast::char_matcher& m) {}
		
		void visit(ast::str_matcher& m) {}

		void visit(ast::range_matcher& m) {
			if ( ! m.var.empty() ) {
				vars.insert(std::make_pair(m.var, "char"));
			}
		}

		void visit(ast::rule_matcher& m) {
			if ( ! m.var.empty() ) {
				vars.insert(std::make_pair(m.var, types[m.rule]));
			}
		}

		void visit(ast::any_matcher& m) {
			if ( ! m.var.empty() ) {
				vars.insert(std::make_pair(m.var, "char"));
			}
		}
		
		void visit(ast::empty_matcher& m) {}
		
		void visit(ast::action_matcher& m) {}
		
		void visit(ast::opt_matcher& m) {
			m.m->accept(this);
		}

		void visit(ast::many_matcher& m) {
			m.m->accept(this);
		}

		void visit(ast::some_matcher& m) {
			m.m->accept(this);
		}

		void visit(ast::seq_matcher& m) {
			for (auto it = m.ms.begin(); it != m.ms.end(); ++it) {
				(*it)->accept(this);
			}
		}
		
		void visit(ast::alt_matcher& m) {
			for (auto it = m.ms.begin(); it != m.ms.end(); ++it) {
				(*it)->accept(this);
			}
		}

		void visit(ast::look_matcher& m) {
			m.m->accept(this);
		}

		void visit(ast::not_matcher& m) {
			m.m->accept(this);
		}

		void visit(ast::capt_matcher& m) {
			vars.insert(std::make_pair(m.var, "std::string"));
			m.m->accept(this);
		}

		std::map<std::string, std::string> list(ast::matcher_ptr& m) {
			vars.clear();
			m->accept(this);
			return vars;
		}
		
		std::map<std::string, std::string> list(ast::grammar_rule& r) {
			vars.clear();
			r.m->accept(this);
			return vars;
		}
		
		bool is_typed(const std::string& rule) const {
			return types.count(rule) ? !types.at(rule).empty() : false;
		}
		
	private:
		/** map of grammar rule names to types */
		std::unordered_map<std::string, std::string> types;
		/** map of variable names to types */
		std::map<std::string, std::string> vars;
	}; /* class variable_list */
	
	/** Code generator for Egg matcher ASTs */
	class compiler : ast::visitor {
	public:
		compiler(std::string name, std::ostream& out = std::cout) 
			: name(name), out(out), tabs(2) {}

		void visit(ast::char_matcher& m) {
			out << "parser::literal(\'" << strings::escape(m.c) << "\')";
		}

		void visit(ast::str_matcher& m) {
			out << "parser::literal(\"" << strings::escape(m.s) << "\")";
		}
		
		void visit(ast::char_range& r, const std::string& var) {
			if ( r.single() ) {
				out << "parser::literal(\'" << strings::escape(r.to) << "\'";
			} else {
				out << "parser::between(\'" << strings::escape(r.from) 
				    << "\', \'" << strings::escape(r.to) << "\'";
			}
			
			if ( ! var.empty() ) {
				out << ", " << var;
			}
			
			out << ")";
		}
		
		void visit(ast::range_matcher& m) {
			if ( m.rs.empty() ) {
				out << "parser::empty()";
			}
			
			if ( m.rs.size() == 1 ) {
				visit(m.rs.front(), m.var);
				return;
			}
			
			std::string indent(++tabs, '\t');

			//chain matcher ranges
			out << std::endl 
				<< indent << "parser::choice({\n"
				<< indent << "\t"
				;
				
			indent += '\t';
			++tabs;
			
			auto it = m.rs.begin();
			visit(*it, m.var);
			
			while ( ++it != m.rs.end() ) {
				out << "," << std::endl
					<< indent 
					;
				visit(*it, m.var);
			}
			
			out << "})";
			
			tabs -= 2;
		}

		void visit(ast::rule_matcher& m) {
			if ( vars.is_typed(m.rule) ) {  // syntactic rule
				if ( m.var.empty() ) {  // unbound
					out << "parser::unbind(" << m.rule << ")";
				} else {  // bound
					out << "parser::bind(" << m.var << ", " << m.rule << ")";
				}
			} else {  // lexical rule
				out << m.rule;
			}
		}

		void visit(ast::any_matcher& m) {
			out << "parser::any(" << m.var << ")";
		}

		void visit(ast::empty_matcher& m) {
			out << "parser::empty()";
		}

		void visit(ast::action_matcher& m) {
			//runs action code with all variables bound, then returns true
			out << "[&](parser::state& ps) {" << m.a << " return true; }";
		}

		void visit(ast::opt_matcher& m) {
			out << "parser::option(";
			m.m->accept(this);
			out << ")";
		}

		void visit(ast::many_matcher& m) {
			out << "parser::many(";
			m.m->accept(this);
			out << ")";
		}

		void visit(ast::some_matcher& m) {
			out << "parser::some(";
			m.m->accept(this);
			out << ")";
		}
		
		void visit(ast::seq_matcher& m) {
			// empty sequence bad form, but always matches
			if ( m.ms.empty() ) {
				out << "parser::empty()";
				return;
			}
			
			// singleton sequence also bad form, equivalent to the single matcher
			if ( m.ms.size() == 1 ) {
				m.ms.front()->accept(this);
				return;
			}

			std::string indent(++tabs, '\t');
			
			out << std::endl
				<< indent << "parser::sequence({\n"
				<< indent << "\t"
				;
			
			indent += '\t';
			++tabs;

			auto it = m.ms.begin();
			(*it)->accept(this);
			while ( ++it != m.ms.end() ) {
				out << "," << std::endl
					<< indent 
					;
				(*it)->accept(this);
			}

			out << "})";

			tabs -= 2;
		}
		
		void visit(ast::alt_matcher& m) {
			// empty alternation bad form, but always matches
			if ( m.ms.empty() ) {
				out << "parser::empty()";
				return;
			}
			
			// singleton alternation also bad form, equivalent to the single matcher
			if ( m.ms.size() == 1 ) {
				m.ms.front()->accept(this);
				return;
			}

			std::string indent(++tabs, '\t');
			
			out << std::endl
				<< indent << "parser::choice({\n"
				<< indent << "\t"
				;
			
			indent += '\t';
			++tabs;

			auto it = m.ms.begin();
			(*it)->accept(this);
			while ( ++it != m.ms.end() ) {
				out << "," << std::endl
					<< indent 
					;
				(*it)->accept(this);
			}

			out << "})";

			tabs -= 2;
		}

		void visit(ast::look_matcher& m) {
			out << "parser::look(";
			m.m->accept(this);
			out << ")";
		}

		void visit(ast::not_matcher& m) {
			out << "parser::look_not(";
			m.m->accept(this);
			out << ")";
		}

		void visit(ast::capt_matcher& m) {
			out << "parser::capture(" << m.var << ", ";
			m.m->accept(this);
			out << ")";
		}

		void compile(ast::grammar_rule& r) {
			bool typed = ! r.type.empty();
			bool has_error = ! r.error.empty();

			//print prototype
			out << "\tbool " << r.name << "(parser::state& ps";
			if ( typed ) out << ", " << r.type << "& psVal";
			out << ") {" << std::endl;
			
			//setup bound variables
			std::map<std::string, std::string> vs = vars.list(r);
			//skip parser variables
			vs.erase("ps");
			vs.erase("psVal");
			for (auto it = vs.begin(); it != vs.end(); ++it) {
				// add variable binding
				out << "\t\t" << it->second << " " << it->first << ";" << std::endl;
			}
			if ( ! vs.empty() ) out << std::endl;

			//apply matcher
			out << "\t\treturn ";
			if ( has_error ) {
				out << "parser::named(\"" << strings::escape(r.error) << "\", ";
			}
			r.m->accept(this);
			if ( has_error) { out << ")"; }
			out << "(ps);";

			//close out method
			out << "\n"
				<< "\t}" << std::endl
				<< std::endl
				;
		}

		/** Compiles a grammar to the output file. */
		void compile(ast::grammar& g) {
			//print pre-amble
			out << "#pragma once" << std::endl
				<< std::endl
				<< "/* THE FOLLOWING HAS BEEN AUTOMATICALLY GENERATED BY THE EGG PARSER GENERATOR." << std::endl
				<< " * DO NOT EDIT. */" << std::endl
				<< std::endl
				;

			//print pre-code
			if ( ! g.pre.empty() ) {
				out << "// {%" << std::endl
					<< g.pre << std::endl
					<< "// %}" << std::endl
					<< std::endl
					;
			}

			//get needed includes
			out << "#include <string>" << std::endl
				<< "#include \"parser.hpp\"" << std::endl
				<< std::endl
				;

			//setup parser namespace
			out << "namespace " << name << " {" << std::endl
				<< std::endl
				;

			//pre-declare matchers
			for (auto it = g.rs.begin(); it != g.rs.end(); ++it) {
				ast::grammar_rule& r = **it;
				out << "\tbool " << r.name << "(parser::state&";
				if ( ! r.type.empty() ) {
					out << ", " << r.type << "&";
				}
				out << ");" << std::endl;
			}
			out << std::endl;

			//generate matching functions
			vars = variable_list(g);
			for (auto it = g.rs.begin(); it != g.rs.end(); ++it) {
				ast::grammar_rule& r = **it;
				compile(r);
			}

			//close parser namespace
			out << "} // namespace " << name << std::endl
				<< std::endl
				;
			
			//print post-code
			if ( ! g.post.empty() ) {
				out << "// {%" << std::endl
					<< g.post << std::endl
					<< "// %}" << std::endl
					<< std::endl
					;
			}
		}
		
	private:
		std::string name;	/** Name of the grammar */
		std::ostream& out;	/** Output stream to print to */
		variable_list vars;	/** Holds grammar rule types */
		int tabs;			/** Number of tabs for printer */
	}; /* class compiler */
	
} /* namespace visitor */

