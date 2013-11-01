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
			out << "parse::matches<\'" << strings::escape(m.c) << "\'>(ps)";
		}

		void visit(ast::str_matcher& m) {
			out << "parse::matches(\"" << strings::escape(m.s) << "\", ps)";
		}
		
		void visit(ast::char_range& r) {
			if ( r.single() ) {
				out << "parse::matches<\'" << strings::escape(r.to) << "\'>(ps)";
			} else {
				out << "parse::in_range<\'" << strings::escape(r.from) 
				    << "\',\'" << strings::escape(r.to) << "\'>(ps)";
			}
		}
		
		void visit(ast::range_matcher& m) {
			if ( m.rs.empty() ) {
				out << "parse::matches<\'\\0\'>(ps)";
				if ( ! m.var.empty() ) out << "(" << m.var << ")";
				return;
			}
			
			if ( m.rs.size() == 1 ) {
				visit(m.rs.front());
				if ( ! m.var.empty() ) out << "(" << m.var << ")";
				return;
			}
			
			std::string indent(++tabs, '\t');

			//chain matcher ranges
			out << "( ";
			
			auto it = m.rs.begin();
			visit(*it);
			if ( ! m.var.empty() ) out << "(" << m.var << ")";
			
			while ( ++it != m.rs.end() ) {
				out << std::endl
					<< indent << "|| "
					;
				visit(*it);
				if ( ! m.var.empty() ) out << "(" << m.var << ")";
			}
			
			out << " )";
			
			--tabs;
		}

		void visit(ast::rule_matcher& m) {
			out << m.rule << "(ps)";
			if ( ! m.var.empty() ) { out << "(" << m.var << ")"; }
		}

		void visit(ast::any_matcher& m) {
			out << "parse::any(ps)";
			if ( ! m.var.empty() ) out << "(" << m.var << ")";
		}

		void visit(ast::empty_matcher& m) {
			out << "true";
		}

		void visit(ast::action_matcher& m) {
			//runs action code with all variables bound, then returns true
			out << "[&]() {" << m.a << " return true; }()";
		}

		void visit(ast::opt_matcher& m) {
			//runs matcher, returns true regardless
			out << "[&]() { ";
			m.m->accept(this);
			out << "; return true; }()";
		}

		void visit(ast::many_matcher& m) {
			std::string indent(tabs, '\t');
			++tabs;

			//runs matcher as many times as it will match
			out << "[&]() { while ( ";
			m.m->accept(this);
			out << " )" << std::endl 
			    << indent << "\t;" << std::endl
			    << indent << "return true; }()";
			
			--tabs;
		}

		void visit(ast::some_matcher& m) {
			std::string indent(tabs, '\t');
			++tabs;
			
			//runs matcher at least once, then ast many times as it will match
			out << "[&]() { if ( ";
			m.m->accept(this);
			out << " ) {" << std::endl
				<< indent << "\twhile ( ";
			m.m->accept(this);
			out << " )" << std::endl
				<< indent << "\t\t;" << std::endl
				<< indent << "\treturn true;" << std::endl
				<< indent << "} else { return false; } }()";

			--tabs;
		}
		
		void visit(ast::seq_matcher& m) {
			//empty sequence bad form, but always matches
			if ( m.ms.empty() ) {
				out << "true";
				return;
			}

			std::string indent(++tabs, '\t');

			//bind all variables but psStart
			out << "[&]() { " << std::endl
				<< indent << "parse::posn psStart = ps;" << std::endl
				<< indent << "if ( ";

			//test that all matchers match
			auto it = m.ms.begin();
			(*it)->accept(this);
			++it;
			while ( it != m.ms.end() ) {
				out << std::endl
					<< indent << "\t&& ";
				(*it)->accept(this);
				++it;
			}

			//match if so, reset otherwise
			out << " ) { return true; }" << std::endl
				<< indent << "else { ps = psStart; return false; } }()";

			--tabs;
		}
		
		void visit(ast::alt_matcher& m) {
			//empty alternation bad form, but always matches
			if ( m.ms.empty() ) {
				out << "true";
				return;
			}

			std::string indent(++tabs, '\t');

			//match any of the contained matchers
			//-contained matchers will all reset pos, so no need to do it here
			out << "( ";
			auto it = m.ms.begin();
			(*it)->accept(this);
			++it;
			while ( it != m.ms.end() ) {
				out << std::endl
					<< indent << "|| ";
				(*it)->accept(this);
				++it;
			}
			out << " )";
			
			--tabs;
		}

		void visit(ast::look_matcher& m) {
			std::string indent(++tabs, '\t');

			//bind all variables but psStart
			out << "[&]() {" << std::endl
				<< indent << "parse::posn psStart = ps;" << std::endl
				<< indent << "if ( ";
			//match iff contained matcher matches, always reset
			m.m->accept(this);
			out << " ) { ps = psStart; return true; }" << std::endl
				<< indent << "else { ps = psStart; return false; } }()";

			--tabs;
		}

		void visit(ast::not_matcher& m) {
			std::string indent(++tabs, '\t');

			//bind all variables but psStart
			out << "[&]() {" << std::endl
				<< indent << "parse::posn psStart = ps;" << std::endl
				<< indent << "if ( ";
			//match iff contained matcher fails, always reset
			m.m->accept(this);
			out << " ) { ps = psStart; return false; }" << std::endl
				<< indent << "else { ps = psStart; return true; } }()";

			--tabs;
		}

		void visit(ast::capt_matcher& m) {
			std::string indent(++tabs, '\t');

			//bind all variables
			out << "[&]() {" << std::endl
				<< indent << "parse::posn psStart = ps;" << std::endl
				<< indent << "if ( ";
			//match iff contained matcher matches
			m.m->accept(this);
			out << " ) {" << std::endl
				<< indent << "\t" << m.var << " = ps.string(psStart, ps - psStart);" << std::endl
				<< indent << "\treturn true;" << std::endl
				<< indent << "} else { return false; } }()";

			--tabs;
		}

		void compile(ast::grammar_rule& r) {
			bool typed = ! r.type.empty();

			//print prototype
			out << "\tparse::result<" << r.type << "> " << r.name << "(parse::state& ps) {" << std::endl
			//setup return point
				<< "\t\tparse::posn psStart = ps;" << std::endl
				;
			//setup return variable
			if ( typed ) out << "\t\t" << r.type << " psVal;" << std::endl;
			out << std::endl;

			//setup bound variables
			std::map<std::string, std::string> vs = vars.list(r);
			for (auto it = vs.begin(); it != vs.end(); ++it) {
				// skip uses of already bound variables
				if ( it->first == "psStart" ) continue;
				if ( typed && it->first == "psVal" ) continue;
				// add variable binding
				out << "\t\t" << it->second << " " << it->first << ";" << std::endl;
			}
			out << std::endl;

			//apply matcher
			out << "\t\tif ( ";
			r.m->accept(this);
			out << " ) { return parse::match(" 
					<< (typed? "psVal" : "parse::val") << "); }" << std::endl
				<< "\t\telse { return parse::fail<"
					<< (typed? r.type : "parse::value") << ">(); }" << std::endl
				<< std::endl
				;

			//close out method
			out << "\t}" << std::endl
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
				<< "#include \"parse.hpp\"" << std::endl
				<< std::endl
				;

			//setup parser namespace
			out << "namespace " << name << " {" << std::endl
				<< std::endl
				;

			//pre-declare matchers
			for (auto it = g.rs.begin(); it != g.rs.end(); ++it) {
				ast::grammar_rule& r = **it;
				out << "\tparse::result<" << r.type << "> " << r.name << "(parse::state&);" << std::endl;
			}
			out << std::endl;

			//generate matching functions
			vars = variable_list(g);
			for (auto it = g.rs.begin(); it != g.rs.end(); ++it) {
				ast::grammar_rule& r = **it;
				compile(r);
			}

			//close parser namespace
			out << "} /* namespace " << name << " */" << std::endl
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

