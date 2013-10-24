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

#include <deque>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

/** Implements parser state for an Egg parser.
 *  
 *  @author Aaron Moss
 */

namespace parse {
	
	typedef unsigned long ind;  /**< unsigned index type */
	
	/** Human-readable position type */
	struct posn {
		posn() : i(0), line(0), col(0) {}
		posn(ind i, ind line, ind col) : i(i), line(line), col(col) {}
		
		bool operator < (const posn& o) { return i < o.i; }
		
		ind i;     /**< input index */
		ind line;  /**< line number */
		ind col;   /**< column number */
	};
	
	/** Error thrown when a parser is asked for state it has forgotten. */
	struct forgotten_state_error : public std::range_error {
		
		/** Default constructor.
		 *  @param req		Requested index
		 *  @param avail	Minimum available index
		 */
		forgotten_state_error(const posn& req, const posn& avail) throw() 
			: std::range_error("Forgotten state error"), 
			req(req), avail(avail) {}
		
		/** Inherited from std::exception. */
		const char* what() const throw() {
			try {
				std::stringstream ss("Forgotten state error");
				ss << ": requested " << req.line << ":" << req.col << " < " 
				   << avail.line << ":" << avail.col;
				return ss.str().c_str();
			} catch (std::exception const& e) {
				return "Forgotten state error";
			}
		}
		
		/** requested index */
		posn req;
		/** minimum available index */
		posn avail;
	}; /* struct forgotten_range_error */
	
	/** Parser state */
	class state {
	public:
		typedef char                               value_type;
		typedef std::deque<char>::iterator         iterator;
		typedef std::pair<iterator, iterator>      range_type;
	
	private:
		/** Read a single character into the parser.
		 *  @return was a character read?
		 */
		bool read() {
			int c = in.get();
			
			// Check EOF
			if ( c == std::char_traits<value_type>::eof ) return false;
			
			// Check newline
			if ( c == '\n' ) lines.push_back(off.i + str.size() + 1);
			
			// Add to stored input
			str.push_back(c);
			return true;
		}
		
		/** Read more characters into the parser.
		 *  @param n		The number of characters to read
		 *  @return The number of characters read
		 */
		ind read(ind n) {
			char s[n];
			// Read into buffer
			in.read(s, n);
			// Count read characters
			ind r = in.gcount();
			// Track newlines
			ind i_max = off.i + str.size();
			for (ind i = 0; i < r; ++i) {
				if ( s[i] == '\n' ) { lines.push_back(i_max + i + 1); }
			}
			// Add to stored input
			str.insert(str.end(), s, s+r);
			return r;
		}
		
	public:
		/** Default constructor.
		 *  Initializes state at beginning of input stream.
		 *  @param in		The input stream to read from
		 */
		state(std::istream& in) : pos(), off(), str(), lines(), in(in) {
			lines.push_back(0);
		}
		
		/** Reads at the cursor.
		 *  @return The character at the current position, or '\0' for end of 
		 *          stream.
		 */
		value_type operator() () const {
			ind i = pos.i - off.i;
			if ( i > str.size() ) return '\0';
			return str[i];
		}
		
		/** Gets the cursor.
		 *  @return the current position 
		 */
		operator posn () const { return pos; }
		
		/** Sets the cursor.
		 *  @param p    The position to set (should have previously been seen)
		 *  @throws forgotten_state_error on p < off (that is, moving to 
		 *  		position previously discarded)
		 */
		state& operator = (const posn& p) {
			// Fail on forgotten index
			if ( p < off ) throw forgotten_state_error(p, off);
			
			pos = p;
			
			return *this;
		}
		
		/** Advances position one step. 
		 *  Will not advance past end-of-stream.
		 */
		state& operator ++ () {
			ind i = pos.i - off.i;
			
			// ignore if already end of stream
			if ( i > str.size() ) return *this;
			
			// update index
			++pos.i;
			
			// read more input if neccessary, terminating on end-of-stream
			if ( i == str.size() && !read() ) return *this;
			
			// update row and column
			ind j = pos.line - off.line;
			if ( j == lines.size() - 1 || p.i < lines[j+1] ) {
				++p.col;
			} else {
				++p.line;
				p.col == 0;
			}
			
			return *this;
		}
		
		/** Range operator.
		 *  Returns a pair of iterators, begin and end, containing up to the 
		 *  given number of elements, starting at the given position. Returned 
		 *  begin and end iterators may be invalidated by calls to any 
		 *  non-const method of this class.
		 *  @param p		The beginning of the range
		 *  @param n		The maximum number of elements in the range
		 *  @throws forgotten_state_error on p < off (that is, asking for input 
		 *  		previously discarded)
		 */
		range_type range(const posn& p, ind n) {
			// Fail on forgotten index
			if ( p < off ) throw forgotten_state_error(p, off);
			
			// Get index into stored input
			ind ib = p.i - off.i;
			ind ie = ib + n;
			
			// Expand stored input if needed
			if ( ie > str.size() ) {
				ind nn = ie - str.size();
				read(nn);
			}
			
			// Get iterators, adjusting for the end of the input
			iterator bIter, eIter;
			
			if ( ie >= str.size() ) {
				eIter = str.end();
				if ( ib >= str.size() ) {
					bIter = str.end();
				} else {
					bIter = str.begin() + ib;
				}
			} else {
				bIter = str.begin() + ib;
				eIter = str.begin() + ie;
			}
			
			return range_type(bIter, eIter);
		}
		
		/** Substring operator.
		 *  Convenience for the string formed by the characters in range(p, n).
		 *  @param p		The beginning of the range
		 *  @param n		The maximum number of elements in the range
		 *  @throws forgotten_state_error on i < off (that is, asking for input 
		 *  		previously discarded)
		 */
		std::string string(const posn& p, size_type n) {
			range_type iters = range(i, n);
			return std::string(iters.first, iters.second);
		}	

	private:
		/** Current parsing location */
		posn pos;
		/** Offset of start of str from the beginning of the stream */
		posn off;
		/** Characters currently in use by the parser */
		std::deque<value_type> str;
		/** Beginning indices of each line, starting from off.line */
		std::deque<ind> lines;
		/** Input stream to read characters from */
		std::istream& in;
	}; /* class state */

	/** A generic parse result. */
	struct value {
		value() {}
		value(int) {}
	};

	/** A generic unsuccessful parse result. */
	struct failure {
		failure() {}
		failure(int) {}
	};

	/** A value instance */
	const value val = 0;
	
	/** A failure instance */
	const failure fails = 0;
	
	/** Represents a parsing error.
	 *  Provides details about position and error. */
	struct error {
		error(const posn& p) : pos(p) {}
		error() : pos() {}
		
		posn pos;  /**< The position of the error */
	};

	/** Wraps a parsing result.
	 *  Returns either the wrapped result or false.
	 *  @param T The wrapped result; should be default constructable. */
	template<typename T = value> 
	class result {
	public:
		result(const T& v) : val(v), success(true), err() {}
		result(const failure& f) : val(), success(false), err() {}
		result() : val(), success(false), err() {}

		/** Sets the result to a success containing v */
		result<T>& operator = (const T& v) { 
			val = v; success = true; return *this;
		}

		/** Sets the result to a failure */
		result<T>& operator = (const failure& f) { 
			success = false; return *this;
		}

		/** Copies a result */
		result<T>& operator = (const result<T>& o) {
			if ( o.success ) { success = true; val = o.val; }
			else { success = false; }
			err = o.err;
			return *this;
		}

		/** Gets result value out */
		operator T () { return val; }

		/** Gets the success value out */
		operator bool () { return success; }

		/** Gets result value out (explicit operator) */
		T operator * () { return val; }

		/** Binds the result (if successful) to a value */
		result<T>& operator () (T& bind) {
			if ( success ) { bind = val; }
			return *this;
		}
		
		/** Sets error information. */
		result<T>& with(const error& e) { err = e; return *this; }
		
		error err;     /**< Error information for this parse. */
	private:
		T val;         /**< The wrapped value. */
		bool success;  /**< The success of the parse. */
	}; /* class result<T> */

	/** Builds a positive result from a value.
	 *  @param T	The type of the wrapped result	
	 *  @param v	The value to wrap. */
	template<typename T>
	result<T> match(const T& v) { return result<T>(v); }

	/** Builds a failure result.
	 *  @param T	The type of the failure result. */
	template<typename T>
	result<T> fail() { return result<T>(fails); }
	
	/** Matcher for any character */
	result<state::value_type> any(state& ps) {
		state::value_type c = ps();
		if ( c == '\0' ) {
			return fail<state::value_type>().with(error(ps));
		}
		++ps;
		return match(c);
	}

	/** Matcher for a given character */
	template<state::value_type c>
	result<state::value_type> matches(parse::state& ps) {
		if ( ps() != c ) {
			return fail<state::value_type>().with(error(ps));
		}
		++ps;
		return match(c);
	}

	/** Matcher for a character range */
	template<state::value_type s, state::value_type e>
	result<state::value_type> in_range(parse::state& ps) {
		state::value_type c = ps();
		if ( c < s || c > e ) {
			return fail<state::value_type>().with(error(ps));
		}
		
		++ps;
		return match(c);
	}
	
} /* namespace parse */

