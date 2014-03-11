#pragma once

/*
 * Copyright (c) 2014 Aaron Moss
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
#include <list>
#include <string>
#include <map>
#include <utility>

#include "../derivs.hpp"
#include "../utils/strings.hpp"

namespace derivs {
	
	/// Visitor with functor-like interface that checks if a node is a compound node
	class is_compound : visitor {
	public:
		is_compound(expr* e) { e->accept(this); }
		is_compound(ptr<expr> e) { e->accept(this); }
		
		operator bool () { return compound; }
		
		void visit(fail_expr& e)  { compound = false; }
		void visit(inf_expr& e)   { compound = false; }
		void visit(eps_expr& e)   { compound = false; }
		void visit(look_expr& e)  { compound = false; }
		void visit(char_expr& e)  { compound = false; }
		void visit(range_expr& e) { compound = false; }
		void visit(any_expr& e)   { compound = false; }
		void visit(str_expr& e)   { compound = false; }
		void visit(rule_expr& e)  { compound = false; }
		void visit(not_expr& e)   { compound = false; }
		void visit(and_expr& e)   { compound = false; }
		void visit(map_expr& e)   { compound = false; }
		void visit(alt_expr& e)   { compound = true; }
		void visit(seq_expr& e)   { compound = true; }
		void visit(back_expr& e)  { compound = true; }
		
	private:
		bool compound;  ///< Is the node a compound node
	}; // is_compound
	
	// Pretty printer for derivatives
	class printer : visitor {
		void print_braced(ptr<expr> e) {
			if ( is_compound(e) ) {
				out << "(";
				e->accept(this);
				out << ")";
			} else {
				e->accept(this);
			}
		}
		
		void print_fns(expr* e) {
/*			out << "{";
			
			switch ( e->nbl() ) {
			case NBL:   out << "O"; break;
			case SHFT:  out << "."; break;
			case EMPTY: out << "0"; break;
			}
			
			switch ( e->lk() ) {
			case LOOK:  out << "^"; break;
			case READ:  out << "."; break;
			case PART:  out << "?"; break;
			}
			
			out << "} ";
*/		}
	public:
		printer(std::ostream& out = std::cout) : out(out) {}
		
		void print_rule(rule_expr& e) {
			auto it = rs.find(e.r.get());
			if ( it == rs.end() ) {
				out << "{RULE :??} ";
			} else {
				out << "{RULE :" << it->second << "} ";
			}
			print_braced(e.r);
			out << std::endl;
		}
		
		/// Prints the derivative expression to the given output stream
		static void print(std::ostream& out, ptr<expr> e) {
			printer p(out);
			if ( e->type() == rule_type ) {
				rule_expr* r = static_cast<rule_expr*>(e.get());
				p.rs.insert(std::make_pair(r->r.get(), 0));
				p.pl.push_back(static_cast<rule_expr*>(e.get()));
			} else {
				e->accept(&p);
				out << std::endl;
			}
			
			auto it = p.pl.begin();
			while ( it != p.pl.end() ) {
				p.print_rule(**it);
				++it;
			}
		}
		
		void visit(fail_expr& e)  { out << "{FAIL}"; }
		
		void visit(inf_expr& e)   { out << "{INF}"; }
		
		void visit(eps_expr& e)   { out << "{EPS}"; }
		
		void visit(look_expr& e)  { out << "{LOOK:" << e.g << "}"; }
		
		void visit(char_expr& e)  { out << "\'" << strings::escape(e.c) << "\'"; }
		
		void visit(range_expr& e) {
			out << "[" << strings::escape(e.b) << "-" << strings::escape(e.e) << "]";
		}
		
		void visit(any_expr& e)   { out << "."; }
		
		void visit(str_expr& e)   { out << "\"" << strings::escape(e.str()) << "\""; }
		
		void visit(rule_expr& e)  {
			auto it = rs.find(e.r.get());
			if ( it == rs.end() ) {  // not printed this rule before
				unsigned int i = rs.size();
				rs.insert(std::make_pair(e.r.get(), i));
				pl.push_back(&e);
				
				out << "{RULE @" << i << "}";
			} else {  // printed this rule before
				out << "{RULE @" << it->second << "}";
			}
		}
		
		void visit(not_expr& e)   { out << "!"; print_braced(e.e); }
		
		void visit(and_expr& e)   { out << "&"; print_braced(e.e); }
		
		void visit(map_expr& e)   {
			out << "{MAP:.." << (unsigned int)e.gen() << "} ";
			print_braced(e.e);
		}
		
		void visit(alt_expr& e)  {
			print_fns(&e);
			print_braced(e.a);
			out << " / ";
			print_braced(e.b);
		}
		
		void visit(seq_expr& e) {
			print_fns(&e);
			print_braced(e.a);
			out << " ";
			print_braced(e.b);
		}
		
		void visit(back_expr& e) {
			print_fns(&e);
			print_braced(e.a);
			out << " \\ ";
			print_braced(e.b);
			out << " <";
			if ( ! e.bs.empty() ) {
				auto it = e.bs.begin();
				out << " {" << it->g << "} ";
				print_braced(it->e);
				while (++it != e.bs.end()) {
					out << " | {" << it->g << "} ";
					print_braced(it->e);
				}
			}
			out << ">";
		}
		
	private:
		std::ostream& out;	               ///< output stream
		std::map<expr*, unsigned int> rs;  ///< Rule identifiers
		std::list<rule_expr*>         pl;  ///< List of rules to print
	}; // class printer
}

