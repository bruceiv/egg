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
#include <functional>
#include <initializer_list>
#include <istream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility>

/** Implements parser state for an Egg parser.
 *  
 *  @author Aaron Moss
 */

namespace parser {
	
	typedef unsigned long ind;  /**< unsigned index type */
	
	/** Human-readable position type */
	struct posn {
	friend class state;
	private:
		// constructor only available to state; discourages messing with posn
		posn(ind index, ind line, ind col) : i(index), ln(line), cl(col) {}
	public:
		posn() : i(0), ln(0), cl(0) {}
		
		/** Checks by index if one position precedes another */
		bool operator < (const posn& o) const { return i < o.i; }
		
		bool operator <= (const posn& o) const { return i <= o.i; }
		bool operator > (const posn& o) const { return i > o.i; }
		bool operator >= (const posn& o) const { return i >= o.i; }
		bool operator == (const posn& o) const { return i == o.i; }
		bool operator != (const posn& o) const { return i != o.i; }
		
		/** @return the number of positions by which another position precedes 
		 *          this position; 0 for this before that.
		 */
		ind operator - (const posn& o) const {
			return ( i < o.i ) ? 0 : i - o.i;
		}
		
		/** @return the character index */
		ind index() const { return i; }
		/** @return the line in the file */
		ind line() const { return ln; }
		/** @return the column in the file */
		ind col() const { return cl; }
		
	private:
		ind i;   /**< input index */
		ind ln;  /**< line number */
		ind cl;  /**< column number */
	};
	
	/** Represents a parsing error.
	 *  Provides details about position and error. 
	 */
	struct error {
		error(const posn& p) : pos(p) {}
		error() : pos() {}
		
		/** Merges two errors.
		 *  Uses Bryan Ford's heuristic of "furthest forward error information".
		 */
		error& operator |= (const error& o) {
			if ( pos > o.pos /* || o.empty() */ ) return *this;
			if ( pos < o.pos /* || empty() */ ) return *this = o;
			
			expected.insert(o.expected.begin(), o.expected.end());
			messages.insert(o.messages.begin(), o.messages.end());
			return *this;
		}
		
		/** Adds an "expected" message */
		inline error& expect(const std::string& s) { expected./*emplace*/insert(s); return *this; }
		
		/** Adds a programmer-defined error message */
		inline error& message(const std::string& s) { messages./*emplace*/insert(s); return *this; }
		
		/** Tests both sets of messages for emptiness */
		inline bool empty() const { return expected.empty() && messages.empty(); }
		
		posn pos;                        /**< The position of the error */
		std::set<std::string> expected;  /**< Constructs expected here */
		std::set<std::string> messages;  /**< Error messages */
	}; // struct error
	
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
				ss << ": requested " << req.line() << ":" << req.col() 
				   << " < " << avail.line() << ":" << avail.col();
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
	
	/** Memoization table entry */
	struct memo {
		/** Typesafe dynamic type */
		class any {
		private:
			/** Untyped container class */
			struct dyn {
				virtual ~dyn() {}
	
				/** Gets the type of the held object */
				virtual const std::type_info& type() const = 0;
	
				/** Copies the held object */
				virtual dyn* clone() const = 0;
			};  // struct dyn

			/** Typed container class */
			template <typename T>
			struct of : public dyn {
				/** Value constructor */
				of (const T& v) : v(v) {}
	
				/** Gets the type of the held object */
				virtual const std::type_info& type() const { return typeid(T); }
	
				/** Returns a copy of the held object */
				virtual dyn* clone() const { return new of<T>(v); }
	
				/** The held value */	
				const T v;
			};  // struct of<T>
		
		public:
			/** Empty constructor */
			any() : p(nullptr) {}

			/** Typed constructor */
			template <typename T>
			any(const T& t) : p(new of<T>(t)) {}

			/** Copy constructor */
			any(const any& o) : p(o.p ? o.p->clone() : nullptr) {}

			/** Assignment operator */
			any& operator = (const any& o) {
				if ( &o != this ) {
					delete p;
					p = o.p ? o.p->clone() : nullptr;
				}
				return *this;
			}

			/** Typed assignment operator */
			template <typename T>
			any& operator = (const T& v) {
				delete p;
				p = new of<T>(v);
				return *this;
			}

			/** Contained type */
			const std::type_info& type() const {
				return p ? p->type() : typeid(void);
			}

			/** Bind value */
			template <typename T>
			void bind(T& v) {
				if ( type() == typeid(T) ) {
					v = static_cast<of<T>*>(p)->v;
				}
			}

			~any() { delete p; }

		private:
			dyn* p;
		};  // class memo::any
		
		/** Default constructor - sets up a failed match */
		memo() : success(false) {}
	
		/** Success constructor */
		template <typename T>
		memo(const posn& end, const T& result) : success(true), end(end), result(result) {}
	
		bool success;  ///< Did the parser match?
		posn end;      ///< Endpoint in case of a match
		any result;    ///< Result object (if any)
	};
	
	/** Parser state */
	class state {
	public:
		typedef char                              value_type;
		typedef std::basic_string<value_type>     string_type;
		typedef std::basic_istream<value_type>    stream_type;
		typedef std::deque<value_type>::iterator  iterator;
		typedef std::pair<iterator, iterator>     range_type;
	
	private:
		/** Read a single character into the parser.
		 *  @return was a character read?
		 */
		bool read() {
			int c = in.get();
			
			// Check EOF
			if ( c == std::char_traits<value_type>::eof() ) return false;
			
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
			value_type s[n];
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
		state(stream_type& in) : pos(), off(), str(), lines(), memo_table(), err(), in(in) {
			// first line starts at 0
			lines.push_back(0);
			// read first character
			read();
		}
		
		/** Reads at the cursor.
		 *  @return The character at the current position, or '\0' for end of 
		 *          stream.
		 */
		value_type operator() () const {
			ind i = pos.i - off.i;
			if ( i >= str.size() ) return '\0';
			return str[i];
		}
		
		/** Reads at the given position.
		 *  @param p    The position to read at (should have been previously seen)
		 *  @return the character at the given position, or '\0' for end of 
		 *          stream.
		 */
		value_type operator() (const posn& p) {
			if ( p < off ) throw forgotten_state_error(p, off);
			
			ind i = p.i - off.i;
			if ( i >= str.size() ) return '\0';
			return str[i];
		}
		
		/** @return the current position */
		struct posn posn() const { return pos; }
		
		/** @return the current offset in the stream */
		struct posn offset() const { return off; }
		
		/** Sets the cursor.
		 *  @param p    The position to set (should have previously been seen)
		 *  @throws forgotten_state_error on p < off (that is, moving to 
		 *  		position previously discarded)
		 */
		void set_posn(const struct posn& p) {
			// Fail on forgotten index
			if ( p < off ) throw forgotten_state_error(p, off);
			
			pos = p;
		}
		
		/** Advances position one step. 
		 *  Will not advance past end-of-stream.
		 */
		state& operator ++ () {
			ind i = pos.i - off.i;
			
			// ignore if already end of stream
			if ( i >= str.size() ) return *this;
			
			// update index
			++pos.i;
			
			// read more input if neccessary, terminating on end-of-stream
			if ( ++i == str.size() && !read() ) { ++pos.cl; return *this; }
			
			// update row and column
			ind j = pos.ln - off.ln;
			if ( j == lines.size() - 1 || pos.i < lines[j+1] ) {
				++pos.cl;
			} else {
				++pos.ln;
				pos.cl = 0;
			}
			
			return *this;
		}
		
		/** Advances position n steps.
		 *  Will not advance past end-of-stream
		 */
		state& operator += (ind n) {
			ind i = pos.i - off.i;
			
			// check if we need to read more input
			if ( i + n >= str.size() ) {
				// ignore if already end of stream
				if ( i >= str.size() ) return *this;
				
				// read extra
				ind nn = i + n + 1 - str.size();
				ind r = read(nn);
				
				// Check read to end of stream, update n to be there
				if ( r < nn ) n = str.size() - i;
			}
			
			// update position
			pos.i += n;
			for (ind j = pos.ln - off.ln + 1; j < lines.size(); ++j) {
				if ( pos.i >= lines[j] ) { ++pos.ln; } else break;
			}
			pos.cl = pos.i - lines[pos.ln - off.ln];
			
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
		range_type range(const struct posn& p, ind n) {
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
		string_type string(const struct posn& p, ind n) {
			range_type iters = range(p, n);
			return string_type(iters.first, iters.second);
		}
		
		/** Gets memoization table entry at the current position.
		 *  @param id    ID of the type to get the memoization entry for
		 *  @param m     Output parameter for memoization entry, if found
		 *  @return Was there a memoization entry?
		 */
		bool memo(ind id, struct memo& m) {
			// Get table iterator
			ind i = pos.i - off.i;
			if ( i >= memo_table.size() ) return false;
			auto& tab = memo_table[i];
			auto it = tab.find(id);
			
			// Break if nothing set
			if ( it == tab.end() ) return false;
			
			// Set output parameter
			m = it->second;
			return true;
		}
		
		/** Sets memoization table entry.
		 *  @param p     Position to set the memo table entry for; will silently ignore if position 
		 *               has been forgotten
		 *  @param id    ID of the type to set the memoization entry for
		 *  @param m     Memoization entry to set
		 *  @return Was a memoization table entry set?
		 */
		bool set_memo(const struct posn& p, ind id, const struct memo& m) {
			// ignore forgotten position
			if ( p < off ) return false;
			
			// ensure table initialized
			ind i = p.i - off.i;
			for (ind ii = memo_table.size(); ii <= i; ++ii) memo_table.emplace_back();
			
			// set table entry
			memo_table[i][id] = m;
			return true;
		}
		
		/** Get the parser's internal error object */
		const struct error& error() const { return err; }
		
		/** Adds an "expected" message at the current position */
		void expect(const std::string& s) {
			struct error e; e.pos = pos; e.expect(s);
			err |= e;
		}
		
		/** Adds a programmer-defined error message at the current position */
		void message(const std::string& s) {
			struct error e; e.pos = pos; e.message(s);
			err |= e;
		}
		
		/** Adds an unexplained error at the current position */
		void fail() {
			struct error e; e.pos = pos;
			err |= e;
		}
		
		/** Attempts to match a character at the current position */
		bool matches(value_type c) {
			if ( (*this)() != c ) return false;
			++(*this);
			return true;
		}
		
		/** Attempts to match a string at the current position */
		bool matches(const string_type& s) {
			if ( string(pos, s.size()) != s ) return false;
			(*this) += s.size();
			return true;
		}
		
		/** Attempts to match any character at the current position
		 *  @param psVal    The character matched, if any
		 */
		bool matches_any(value_type& psVal) {
			value_type c = (*this)();
			if ( c == '\0' ) return false;
			psVal = c;
			++(*this);
			return true;
		}
		
		/** Attempts to match any character at the current position */
		bool matches_any() {
			if ( (*this)() == '\0' ) return false;
			++(*this);
			return true;
		}
		
		/** Attempts to match a character in the given range at the current 
		 *  position.
		 *  @param s        The start of the range
		 *  @param e        The end of the range
		 *  @param psVal    The character matched, if any
		 */
		bool matches_in(value_type s, value_type e, value_type& psVal) {
			value_type c = (*this)();
			if ( c < s || c > e ) return false;
			psVal = c;
			++(*this);
			return true;
		}
		
		/** Attempts to match a character in the given range at the current 
		 *  position.
		 *  @param s        The start of the range
		 *  @param e        The end of the range
		 */
		bool matches_in(value_type s, value_type e) {
			value_type c = (*this)();
			if ( c < s || c > e ) return false;
			++(*this);
			return true;
		}
	private:
		/** Current parsing location */
		struct posn pos;
		/** Offset of start of str from the beginning of the stream */
		struct posn off;
		/** Characters currently in use by the parser */
		std::deque<value_type> str;
		/** Beginning indices of each line, starting from off.line */
		std::deque<ind> lines;
		/** Memoization tables for each stored input index */
		std::deque<std::unordered_map<ind, struct memo>> memo_table;
		/** Set of most recent parsing errors */
		struct error err;
		/** Input stream to read characters from */
		stream_type& in;
	}; /* class state */
	
	/** Parser combinator type */
	using combinator = std::function<bool(state&)>;
	/** List of parser combinators */
	using combinator_list = std::initializer_list<combinator>;
	/** Typed nonterminal type */
	template <typename T>
	using nonterminal = bool (*)(state&,T&);
	
	/** Character literal parser */
	combinator literal(state::value_type c) {
//		return [c](state& ps) { return ps.matches(c); };
		return [c](state& ps) {
			if ( ps.matches(c) ) { return true; }
			
			ps.fail();
			return false;
		};
	}
	
	/** Character literal parser 
	 *  @param psVal    Will be bound to character matched
	 */
	combinator literal(state::value_type c, state::value_type& psVal) {
		return [c,&psVal](state& ps) {
			if ( ps.matches(c) ) { psVal = c; return true; }
//			else return false;
			
			ps.fail();
			return false;
		};
	}
	
	/** String literal parser */
	combinator literal(const state::string_type& s) {
//		return [&s](state& ps) { return ps.matches(s); };
		return [&s](state& ps) {
			if ( ps.matches(s) ) { return true; }
			
			ps.fail();
			return false;
		};
	}
	
	/** Any character parser */
	combinator any() {
//		return [](state& ps) { return ps.matches_any(); };
		return [](state& ps) {
			if ( ps.matches_any() ) { return true; }
			ps.fail();
			return false;
		};
	}
	
	/** Any character parser
	 *  @param psVal    Will be bound to the character matched
	 */
	combinator any(state::value_type& psVal) {
//		return [&psVal](state& ps) { return ps.matches_any(psVal); };
		return [&psVal](state& ps) {
			if ( ps.matches_any(psVal) ) { return true; }
			
			ps.fail();
			return false;
		};
	}
	
	/** Character range parser parser */
	combinator between(state::value_type s, state::value_type e) {
//		return [s,e](state& ps) { return ps.matches_in(s, e); };
		return [s,e](state& ps) {
			if ( ps.matches_in(s, e) ) { return true; }
			
			ps.fail();
			return false;
		};
	}
	
	/** Character range parser
	 *  @param psVal    Will be bound to the character matched
	 */
	combinator between(state::value_type s, state::value_type e, state::value_type& psVal) {
//		return [s,e,&psVal](state& ps) { return ps.matches_in(s, e, psVal); };
		return [s,e,&psVal](state& ps) {
			if ( ps.matches_in(s, e, psVal) ) { return true; }
			
			ps.fail();
			return false;
		};
	}
	
	/** Matches all or none of a sequence of parsers */
	combinator sequence(combinator_list fs) {
		return [fs](state& ps) {
			posn psStart = ps.posn();
			for (auto f : fs) {
				if ( ! f(ps) ) { ps.set_posn(psStart); return false; }
			}
			return true;
		};
	}
	
	/** Matches one of a set of alternate parsers */
	combinator choice(combinator_list fs) {
		return [fs](state& ps) {
			for (auto f : fs) {
				if ( f(ps) ) return true;
			}
			return false;
		};
	}
	
	/** Matches a parser any number of times */
	combinator many(const combinator& f) {
		return [&f](state& ps) {
			while ( f(ps) )
				;
			return true;
		};
	}
	
	/** Matches a parser some positive number of times */
	combinator some(const combinator& f) {
		return [&f](state& ps) {
			if ( ! f(ps) ) return false;
			while ( f(ps) )
				;
			return true;
		};
	}
	
	/** Optionally matches a parser */
	combinator option(const combinator& f) {
		return [&f](state& ps) {
			f(ps);
			return true;
		};
	}
	
	/** Looks ahead to match a parser without consuming input */
	combinator look(const combinator& f) {
		return [&f](state& ps) {
			posn psStart = ps.posn();
			if ( f(ps) ) { ps.set_posn(psStart); return true; }
			return false;
		};
	}
	
	/** Looks ahead to not match a parser without consuming input */
	combinator look_not(const combinator& f) {
		return [&f](state& ps) {
			posn psStart = ps.posn();
			if ( f(ps) ) { ps.set_posn(psStart); return false; }
			return true;
		};
	}
	
	/** Binds a variable to a non-terminal */
	template <typename T>
	combinator bind(T& psVal, nonterminal<T> f) {
		return [&psVal,f](state& ps) { return f(ps, psVal); };
	}
	
	/** Binds a throwaway variable to a non-terminal */
	template <typename T>
	combinator unbind(nonterminal<T> f) {
		return [f](state& ps) { T _; return f(ps, _); };
	}
	
	/** Memoizes a combinator with the given memoization ID */
	combinator memoize(ind id, const combinator& f) {
		return [id,&f](state& ps) {
			memo m;
			if ( ps.memo(id, m) ) {
				if ( m.success ) ps.set_posn(m.end);
			} else {
				posn psStart = ps.posn();
				m.success = f(ps);
				m.end = ps.posn();
				ps.set_memo(psStart, id, m);
			}
			return m.success;
		};
	}
	
	/** Memoizes and binds a combinator with the given memoization ID */
	template <typename T>
	combinator memoize(ind id, T& psVal, const combinator& f) {
		return [id,&psVal,f](state& ps) {
			memo m;
			if ( ps.memo(id, m) ) {
				if ( m.success ) {
					m.result.bind(psVal);
					ps.set_posn(m.end);
				}
			} else {
				posn psStart = ps.posn();
				m.success = f(ps);
				m.end = ps.posn();
				if ( m.success ) m.result = psVal;
				ps.set_memo(psStart, id, m);
			}
			return m.success;
		};
	}
	
	namespace {
		/** Helper function for memoizing repetition */
		memo many_memoized(ind id, const combinator& f, state& ps) {
			memo m;
			if ( ps.memo(id, m) ) {
				if ( m.end > ps.posn() ) ps.set_posn(m.end);
			} else {
				posn psStart = ps.posn();
				if ( f(ps) ) {
					m = many_memoized(id, f, ps);
				}
				m.success = true;
				m.end = ps.posn();
				ps.set_memo(psStart, id, m);
			}
			return m;
		}
	} /* anonymous namespace */
	
	/** Memoizes a many-matcher  */
	combinator memoize_many(ind id, const combinator& f) {
		return [id,&f](state& ps) {
			many_memoized(id, f, ps);
			return true;
		};
	}
	
	/** Memoizes a some-matcher */
	combinator memoize_some(ind id, const combinator& f) {
		return [id,&f](state& ps) {
			posn psStart = ps.posn();
			many_memoized(id, f, ps);
			return ( ps.posn() > psStart );
		};
	}
	
	/** Captures a string */
	combinator capture(std::string& s, const combinator& f) {
		return [&s,&f](state& ps) {
			posn psStart = ps.posn();
			if ( ! f(ps) ) return false;
			s = ps.string(psStart, ps.posn() - psStart);
			return true;
		};
	}
	
	/** Empty parser; always matches */
	combinator empty() {
		return [](state&) { return true; };
	}
	
	/** Failure parser; inserts message */
	combinator fail(const std::string& s) {
		return [&s](state& ps) { ps.message(s); return false; };
	}
	
	/** Names a parser for better error messages */
	combinator named(const std::string& s, const combinator& f) {
		return [&s,&f](state& ps) {
			if ( f(ps) ) return true;
			
			ps.expect(s);
			return false;
		};
	}
	
} /* namespace parser */

