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
#include <typeinfo>
#include <utility>

#include "../derivs-mut.hpp"
#include "../utils/strings.hpp"

namespace derivs {
	
	/// Visitor with functor-like interface that checks if a node is a compound node
	class is_compound : visitor {
	public:
		is_compound(expr& e) { e.accept(this); }
		
		operator bool () { return compound; }
		
		void visit(fail_node& e)   { compound = false; }
		void visit(inf_node& e)    { compound = false; }
		void visit(eps_node& e)    { compound = false; }
		void visit(look_node& e)   { compound = false; }
		void visit(char_node& e)   { compound = false; }
		void visit(range_node& e)  { compound = false; }
		void visit(any_node& e)    { compound = false; }
		void visit(str_node& e)    { compound = false; }
		void visit(shared_node& e) { compound = false; }
		void visit(rule_node& e)   { compound = false; }
		void visit(not_node& e)    { compound = false; }
		void visit(map_node& e)    { compound = false; }
		void visit(alt_node& e)    { compound = true; }
		void visit(seq_node& e)    { compound = true; }
		
	private:
		bool compound;  ///< Is the node a compound node
	}; // is_compound
	
	// Pretty printer for derivatives
	class printer : visitor {
		void print_braced(expr& e) {
			if ( is_compound(e) ) {
				out << "(";
				e.accept(this);
				out << ")";
			} else {
				e.accept(this);
			}
		}
		
		void print_unbraced(expr& e) {
			e.accept(this);
		}
		
		void print_uint_set(const utils::uint_set& s) {
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
		
		void print_uint_map(const utils::uint_pfn& f) {
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
		
		void print_fns(const node* e) {
			out << "b";
			print_uint_set(e->back(i));
			out << "m";
			print_uint_set(e->match(i));
		}
	public:
		/// Default printer
		printer(std::ostream& out = std::cout) : out(out), nc(0) {}
		
		/// Rule name loading printer
		printer(std::ostream& out, std::map<expr*, std::string>& rs) 
			: out(out), rs(rs), nc(rs.size()) {}
		
		void print_rule(rule_node& e, ind i = 0) {
			this->i = i;
			auto it = rs.find(e.r.get());
			if ( it == rs.end() ) {
				out << "{RULE :??} ";
			} else {
				out << "{RULE :" << it->second << "} ";
			}
			print_fns(&e); out << " ";
			visit(e.r);
			out << std::endl;
		}
		
		/// Prints the expression
		void print(expr& e, ind i = 0) {
			this->i = i;
			if ( typeid(*e.get()) == typeid(rule_node) ) {
				rule_node* r = static_cast<rule_node*>(e.get());
				if ( rs.count(r->r.get()) == 0 ) {
					rs.insert(std::make_pair(r->r.get(), std::to_string(rs.size() - nc)));
				}
				pl.push_back(r);
			} else {
				e.accept(this);
				out << std::endl;
			}
			
			auto it = pl.begin();
			while ( it != pl.end() ) {
				print_rule(**it);
				++it;
			}
			pl.clear();
		}
		
		/// Prints a shared expression
		void print(shared_node& e, ind i = 0) {
			out << "x" << e.shared->refs << " ";
			print(e.shared->e, i);
		}
		
		/// Prints the derivative of the expression to the given output stream 
		//  (with the given named rules)
		static void print(std::ostream& out, expr& e, 
		                  std::map<expr*, std::string>& rs, ind i = 0) {
			printer p(out, rs);
			p.print(e, i);
		}
		
		/// Prints the derivative expression to the given output stream
		static void print(std::ostream& out, expr& e, ind i = 0) {
			std::map<expr*, std::string> rs;
			print(out, e, rs, i);
		}
		
		void visit(fail_node& e)  { out << "{FAIL}"; }
		
		void visit(inf_node& e)   { out << "{INF}"; }
		
		void visit(eps_node& e)   { out << "{EPS}"; }
		
		void visit(look_node& e)  { out << "{LOOK:" << e.b << "}"; }
		
		void visit(char_node& e)  { out << "\'" << strings::escape(e.c) << "\'"; }
		
		void visit(range_node& e) {
			out << "[" << strings::escape(e.b) << "-" << strings::escape(e.e) << "]";
		}
		
		void visit(any_node& e)   { out << "."; }
		
		void visit(str_node& e)   { out << "\"" << strings::escape(e.str()) << "\""; }
		
		void visit(shared_node& e) {
			out << "x" << e.shared->refs << " ";
			print_unbraced(e.shared->e);
		}
		
		void visit(rule_node& e)  {
			auto it = rs.find(e.r.get());
			if ( it == rs.end() ) {  // not printed this rule before
				unsigned int i = rs.size() - nc;
				rs.insert(std::make_pair(e.r.get(), std::to_string(i)));
				pl.push_back(&e);
				
				out << "{RULE ";
				print_fns(&e);
				out << " @" << i << "}";
			} else {  // printed this rule before
				out << "{RULE ";
				print_fns(&e);
				out << " @" << it->second << "}";
			}
		}
		
		void visit(not_node& e)   { out << "!"; print_unbraced(e.e); }
		
		void visit(map_node& e)   {
			out << "(map:";
			print_fns(&e);
			out << "g" << (unsigned int)e.gm;
			out << " ";
			print_uint_map(e.eg);
			out << " ";
			print_unbraced(e.e);
			out << ")";
		}
		
		void visit(alt_node& e)  {
			out << "(alt:";
			print_fns(&e);
			out << "g" << (unsigned int)e.gm;
			out << " ";
			print_uint_map(e.ag);
			out << " ";
			print_unbraced(e.a);
			out << " / ";
			print_uint_map(e.bg);
			out << " ";
			print_unbraced(e.b);
			out << ")";
		}
		
		void visit(seq_node& e) {
			out << "(seq:";
			print_fns(&e);
			out << "g" << (unsigned int)e.gm;
			out << " ";
			print_unbraced(e.a);
			out << " ++ ";
			print_unbraced(e.b);
			if ( ! e.bs.empty() ) {
				out << " <";
				auto it = e.bs.begin();
				out << " {" << (unsigned int)it->g;
				if ( it->gl > 0 ) { out << "." << (unsigned int)it->gl; }
				out << "} ";
				print_uint_map(it->eg);
				out << " ";
				print_unbraced(it->e);
				
				while (++it != e.bs.end()) {
					out << " | {" << (unsigned int)it->g;
					if ( it->gl > 0 ) { out << "." << (unsigned int)it->gl; }
					out << "} ";
					print_uint_map(it->eg);
					out << " ";
					print_unbraced(it->e);
				}
				out << ">";
			}
			if ( e.c.type() != fail_type ) {
				out << " \\\\ ";
				print_uint_map(e.cg);
				out << " ";
				print_unbraced(e.c);
			}
			out << ")";
		}
		
	private:
		std::ostream&                out;  ///< output stream
		std::map<expr*, std::string> rs;   ///< Rule identifiers
		unsigned int                 nc;   ///< Count of named rules
		std::list<rule_node*>        pl;   ///< List of rules to print
		ind                          i;    ///< Index of current character
	}; // class printer
}
