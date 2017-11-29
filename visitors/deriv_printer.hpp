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
#include "../visitors/printer.hpp"

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
		void visit(char_expr& e)  { compound = false; }
		void visit(except_expr& e) { compound = false; }
		void visit(range_expr& e) { compound = false; }
		void visit(except_range_expr& e) { compound = false; }
		void visit(any_expr& e)   { compound = false; }
		void visit(none_expr& e)  { compound = false; }
		void visit(str_expr& e)   { compound = false; }
		void visit(not_expr& e)   { compound = false; }
		void visit(alt_expr& e)   { compound = true; }
		void visit(opt_expr& e)   { compound = false; }
		void visit(or_expr& e)  { compound = true; }
		void visit(and_expr& e) { compound = true; }
		void visit(seq_expr& e)   { compound = true; }
		
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
		
		void print_unbraced(ptr<expr> e) {
			e->accept(this);
		}
		
		template<typename T>
		void print_set(const T& s) {
			out << "[";
			auto st = s.begin();
			if ( st != s.end() ) {
				out << *st;
				++st;
				while ( st != s.end() ) {
					out << "," << *st;
					++st;
				}
			}
			out << "]";
		}
		
		template<typename T>
		void print_map(const T& f) {
			out << "[";
			auto ft = f.begin();
			if ( ft != f.end() ) {
				out << ft->first << ":" << ft->second;
				++ft;
				while ( ft != f.end() ) {
					out << ", " << ft->first << ":" << ft->second;
					++ft;
				}
			}
			out << "]";
		}
		
		void print_fns(expr* e) {
			gen_set eb = e->back();
			if ( eb.empty() ) return;
			out << "b";
			print_set(eb);

			gen_set em = e->match();
			if ( em.empty() ) return;
			out << "m";
			print_set(em);
		}
	public:
		/// Default printer
		printer(std::ostream& out = std::cout) : out(out) {}
		
		/// Prints the expression
		void print(ptr<expr> e) {
			e->accept(this);
			out << std::endl;
		}
				
		/// Prints the derivative expression to the given output stream
		static void print(std::ostream& out, ptr<expr> e) {
			// std::map<expr*, std::string> rs;
			// print(out, e, rs);
			printer p(out);
			p.print(e);
		}
		
		void visit(fail_expr& e)  { out << "{FAIL}"; }
		
		void visit(inf_expr& e)   { out << "{INF}"; }
		
		void visit(eps_expr& e)   { out << "{EPS:" << e.g << "}"; }
		
		void visit(char_expr& e)  { out << "\'" << strings::escape(e.c) << "\'"; }

		void visit(except_expr& e)  { out << "[^" << strings::escape(e.c) << "]"; }
		
		void visit(range_expr& e) {
			out << "[" << strings::escape(e.b) << "-" << strings::escape(e.e) << "]";
		}

		void visit(except_range_expr& e) {
			out << "[^" << strings::escape(e.b) << "-" << strings::escape(e.e) << "]";
		}
		
		void visit(any_expr& e)   { out << "."; }

		void visit(none_expr& e)   { out << "$"; }
		
		void visit(str_expr& e)   { out << "\"" << strings::escape(e.str()) << "\""; }
		
		void visit(not_expr& e)   {
			out << "(not:" << e.g << " ";
			print_unbraced(e.e);
			out << ")";
		}
		
		void visit(alt_expr& e)  {
			out << "(alt:";
			if ( e.gl != no_gen ) { out << e.gl; }
			print_fns(&e);
			
			auto et = e.es.begin();
			if ( et != e.es.end() ) {
				out << " ";
				print_unbraced(*et);
			}

			while ( ++et != e.es.end() ) {
				out << " / ";
				print_unbraced(*et);
			}

			out << ")";
		}

		void visit(opt_expr& e)   {
			out << "(opt:" << e.gl << " ";
			print_unbraced(e.e);
			out << ")";
		}

		void visit(or_expr& e)  {
			out << "(or:";
						
			auto et = e.es.begin();
			if ( et != e.es.end() ) {
				out << " ";
				print_unbraced(*et);
			}

			while ( ++et != e.es.end() ) {
				out << " || ";
				print_unbraced(*et);
			}

			out << ")";
		}

		void visit(and_expr& e)  {
			out << "(and:";
						
			auto et = e.es.begin();
			if ( et != e.es.end() ) {
				out << " ";
				print_unbraced(*et);
			}

			while ( ++et != e.es.end() ) {
				out << " && ";
				print_unbraced(*et);
			}

			out << ")";
		}
		
		void visit(seq_expr& e) {
			out << "(seq:";
			if ( e.gl != no_gen ) { out << e.gl; }
			print_fns(&e);
			out << " ";
			print_unbraced(e.a);
			out << " ++ <";
			::visitor::printer{ out, ::visitor::printer::single_line }( e.b );
			out << ">";
			if ( ! e.bs.empty() ) {
				out << " {";
				auto it = e.bs.begin();
				out << "(" << (unsigned int)it->g;
				if ( e.gl == it->g ) { out << "*"; }
				out << ") ";
				print_unbraced(it->e);
				
				while (++it != e.bs.end()) {
					out << " | (" << (unsigned int)it->g;
					if ( e.gl == it->g ) { out << "*"; }
					out << ") ";
					print_unbraced(it->e);
				}
				out << "}";
			}
			out << ")";
		}
		
	private:
		std::ostream&                out;  ///< output stream
	}; // class printer
}

static inline void __attribute__((used)) dbgprt( derivs::ptr<derivs::expr> e ) {
	derivs::printer{ std::cerr }.print( e );
}

