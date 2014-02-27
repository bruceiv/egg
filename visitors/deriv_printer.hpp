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
#include <string>

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
				out << "( ";
				e->accept(this);
				out << " )";
			} else {
				e->accept(this);
			}
		}
	public:
		printer(std::ostream& out = std::cout, int tabs = 0) : out(out), tabs(tabs) {}
		
		/// Prints the derivative expression to the given output stream
		static void print(std::ostream& out, ptr<expr> e) {
			printer p(out);
			e->accept(&p);
			out << std::endl;
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
		
		void visit(rule_expr& e)  { out << "{RULE}"; }
		
		void visit(not_expr& e)   {
			out << "!";
			print_braced(e.e);
		}
		
		void visit(and_expr& e)   {
			out << "&";
			print_braced(e.e);
		}
		
		void visit(map_expr& e) {
			out << "{MAP:.." << e.gen() << "} ";
			print_braced(e.e);
		}
		
		void visit(alt_expr& e) {
			std::string indent((4 * ++tabs), ' ');
			print_braced(e.a);
			out << "\n" << indent << "/ ";
			print_braced(e.b);
			--tabs;
		}
		
		void visit(seq_expr& e) {
			std::string indent((4 * ++tabs), ' ');
			print_braced(e.a);
			out << "\n" << indent;
			print_braced(e.b);
			--tabs;
		}
		
		void visit(back_expr& e) {
			std::string indent((4 * ++tabs), ' ');
			print_braced(e.a);
			out << "\n" << indent << "\\ ";
			print_braced(e.b);
			out << "\n" << indent << "<";
			if ( ! e.bs.empty() ) {
				auto it = e.bs.begin();
				out << " {" << it->g << "} ";
				print_braced(it->e);
				while (++it != e.bs.end()) {
					out << "\n" << indent << "| {" << it->g << "} ";
					print_braced(it->e);
				}
			}
			out << ">";
			--tabs;
		}
		
	private:
		std::ostream& out;	///< output stream
		int tabs;			///< current number of tab stops
	}; // printer

}

