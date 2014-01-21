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

#include <string>
#include <memory>

/**
 * Implements derivative parsing for parsing expression grammars, according to the algorithm 
 * described by Aaron Moss in 2014 (currently unpublished - contact the author for a manuscript).
 * 
 * The basic idea of this derivative parsing algorithm is to repeatedly take the "derivative" of a 
 * parsing expression with respect to the next character in the input sequence, where the 
 * derivative is a parsing expression which matches the suffixes of all strings in the language of 
 * the original expression which start with the given prefix.
 */
namespace derivs {
	template <typename T>
	using ptr = std::shared_ptr<T>;
	
	/// Type of expression node
	enum expr_type {
		fail_type,
		inf_type,
		eps_type,
		look_type,
		char_type,
		range_type,
		any_type,
		str_type,
		rule_type,
		not_type,
		and_type,
		seq_type,
		alt_type,
		e_back_type,
		l_back_type,
		el_back_type
	}; // enum expr_type
	
	/// Nullability mode
	enum nbl_mode {
		NBL,    ///< Matches on all strings
		EMPTY,  ///< Matches on empty string, but not all strings
		SHFT    ///< Does not match on empty string
	}; // enum nbl_mode
	
	/// Lookahead mode
	enum lk_mode {
		LOOK,  ///< Lookahead expression
		PART,  ///< Partial lookahead expression
		READ   ///< Non-lookahead expression
	}; // enum lk_mode
	
	/// Abstract base class for parsing expressions
	struct expr {
		/// Derivative of this expression with respect to c
		virtual ptr<expr> d(char c) const = 0;
		
		/// Is this expression nullable?
		virtual nbl_mode nbl() const = 0;
		
		/// Is this a lookahead expression?
		virtual lk_mode lk() const = 0;
		
		/// Expression node type
		virtual expr_type type() const = 0;
	}; // struct expr
	
	/// Makes an expression pointer given the constructor arguments for some subclass T
	template <typename T, typename... Args>
	inline ptr<expr> expr_ptr(Args... args) {
		return std::static_ptr_cast<expr>(std::make_shared<T>(args...));
	}
	
	class fail_expr;
	class inf_expr;
	class eps_expr;
	class look_expr;
	class char_expr;
	class range_expr;
	class any_expr;
	class str_expr;
	class rule_expr;
	class not_expr;
	class and_expr;
	class seq_expr;
	class alt_expr;
	class e_back_expr;
	class l_back_expr;
	class el_back_expr;
	
	/// A failure parsing expression
	struct fail_expr : public expr {
		ptr<expr> d(char) const {
			// A failure expression can't un-fail - no strings to match with any prefix
			return expr_ptr<fail_expr>();
		}
		
		nbl_mode  nbl()  const { return SHFT; }
		lk_mode   lk()   const { return READ; }
		expr_type type() const { return fail_type; }
	}; // struct fail_expr
	
	/// An infinite loop failure parsing expression
	struct inf_expr : public expr {
		ptr<expr> d(char) const {
			// An infinite loop expression never breaks, ill defined with any prefix
			return expr_ptr<inf_expr>();
		}
		
		nbl_mode  nbl()   const { return SHFT; }
		lk_mode   lk()    const { return READ; }
		expr_type type()  const { return inf_type; }
	}; // struct inf_expr
	
	/// An empty success parsing expression
	struct eps_expr : public expr {
		ptr<expr> d(char) const {
			// No prefixes to remove from language containing the empty string; all fail
			return expr_ptr<fail_expr>();
		}
		
		nbl_mode  nbl()   const { return NBL; }
		lk_mode   lk()    const { return READ; }
		expr_type type()  const { return eps_type; }
	}; // struct eps_expr
	
	/// A lookahead success parsing expression
	struct look_expr : public expr {
		look_expr(int gen = 0) : gen(gen) {}
		
		ptr<expr> d(char) const {
			// Lookahead success is just a marker, so persists (character will be parsed by sequel)
			return expr_ptr<look_expr>(gen);
		}
		
		nbl_mode  nbl()   const { return NBL; }
		lk_mode   lk()    const { return LOOK; }
		expr_type type()  const { return look_type; }
		
		int gen;  ///< Lookahead generation
	}; // struct look_expr
	
	/// A single-character parsing expression
	struct char_expr : public expr {
		char_expr(char c) : c(c) {}
		
		ptr<expr> d(char x) const {
			// Single-character expression either consumes matching character or fails
			return ( c == x ) ? 
				expr_ptr<eps_expr>() : 
				expr_ptr<fail_expr>();
		}
		
		nbl_mode  nbl()  const { return SHFT; }
		lk_mode   lk()   const { return READ; }
		expr_type type() const { return char_type; }
		
		char c; ///< Character represented by the expression
	}; // struct char_expr
	
	/// A character range parsing expression
	struct range_expr : public expr {
		range_expr(char b, char e) : b(b), e(e) {}
		
		ptr<expr> d(char x) const {
			// Character range expression either consumes matching character or fails
			return ( b <= x && x <= e ) ?
				expr_ptr<eps_expr>() : 
				expr_ptr<fail_expr>();
		}
		
		nbl_mode  nbl()  const { return SHFT; }
		lk_mode   lk()   const { return READ; }
		expr_type type() const { return range_type; }
		
		char b;  ///< First character in expression range 
		char e;  ///< Last character in expression range
	}; // struct range_expr
	
	/// A parsing expression which matches any character
	struct any_expr : public expr {
		ptr<expr> d(char) const {
			// Any-character expression consumes any character
			return expr_ptr<eps_expr>();
		}
		
		nbl_mode  nbl()  const { return SHFT; }
		lk_mode   lk()   const { return READ; }
		expr_type type() const { return any_type; }
	}; // struct any_expr
	
	/// A parsing expression representing a character string
	struct str_expr : public expr {
	private:
		/// Ref-counted cons-list representation of a string
		struct str_node {
			char c;              ///< Character in node
			ptr<str_node> next;  ///< Next node
			
			/// Constructs a new string node with a null next character
			str_node(char c) : c(c), next() {}
			
			/// Constructs a new string node with the given next character
			str_node(char c, ptr<str_node> next) : c(c), next(next) {}
			
			/// Constructs a new series of string nodes for the given string.
			/// Warning: string should have length of at least one
			str_node(const std::string& s) {
				// make node for end of string
				std::string::const_iterator it = s.rbegin();
				ptr<str_node> last = std::make_shared<str_node>(*it);
				
				// make nodes for internal characters
				++it;
				while ( it != s.rend() ) {
					last = std::make_shared<str_node>(*it, last);
					++it;
				}
				
				// setup this node
				c = last->c;
				next = last->next;
			}
			
			/// Gets the string of the given length out of a list of nodes
			std::string str(unsigned long len) const {
				const str_node* n = this;
				
				char a[len];
				for (unsigned long i = 0; i < len; ++i) {
					a[i] = n->c;
					n = n->next;
				}
				
				return std::string(a, len);
			}
		}; // struct str_node
		
		/// Constructs a string expression from a string node and a length
		str_expr(str_node s, unsigned long len) : s(s), len(len) {}
		/// Constructs a string expression from a standard string (should be non-empty)
		str_expr(std::string t) : s(t), len(t.size()) {}
		
	public:
		/// Makes an appropriate expression node for the length of the string
		static ptr<expr> make(std::string t) {
			switch ( t.size() ) {
			case 0:  return expr_ptr<eps_expr>();
			case 1:  return expr_ptr<char_expr>(t[0]);
			default: return ptr<expr>(new str_expr(t));
			}
		}
		
		ptr<expr> d(char x) const {
			// Check that the first character matches
			if ( s.c != x ) return expr_ptr<fail_expr>();
			
			// Otherwise return a character or string expression, as appropriate
			return ( len == 2 ) ? 
				expr_ptr<char_expr>(s.next.c) :
				ptr<expr>(new str_expr(s.next, len - 1));
		}
		
		nbl_mode  nbl()  const { return SHFT; }
		lk_mode   lk()   const { return READ; }
		expr_type type() const { return str_type; }
		
		/// Gets the string represented by this node
		std::string str() const { return s.str(); }
		
	private:
		str_node s;         ///< Internal string representation
		unsigned long len;  ///< length of internal string
	}; // struct str_expr
	
} // namespace derivs

