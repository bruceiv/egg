#ifndef _EGG_PARSE_HPP_
#define _EGG_PARSE_HPP_

#include <deque>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <utility>

/** Implements parser state for an Egg parser.
 *  
 *  @author Aaron Moss
 */

namespace parse {
	
	typedef long			ind;	/**< index type */
	typedef unsigned long	uind;	/**< unsigned index type */
	
	/** Error thrown when a parser is asked for state it has forgotten. */
	struct forgotten_state_error : public std::range_error {
		
		/** Default constructor.
		 *  @param req		Requested index
		 *  @param avail	Minimum available index
		 */
		forgotten_state_error(uind req, uind avail) throw() 
			: req(req), avail(avail) {}
		
		/** Inherited from std::exception. */
		const char* what() const throw() {
			try {
				std::stringstream ss("Forgotten state error");
				ss << ": requested " << req << " < " << avail;
				return ss.str().c_str();
			} catch (std::exception const& e) {
				return "Forgotten state error";
			}
		}
		
		/** requested index */
		uind req;
		/** minimum available index */
		uind avail;
	}; /* struct forgotten_range_error */
	
	/** Parser state */
	class state {
	public:
		typedef char								value_type;
		typedef std::deque<char>::iterator			iterator;
		typedef std::pair<iterator, iterator>		range_type;
		typedef std::deque<char>::difference_type	difference_type;
		typedef uind								size_type;
		
		/** Default constructor.
		 *  Initializes state at beginning of input stream.
		 *  @param in		The input stream to read from
		 */
		state(std::istream& in) : pos(0), str(), str_off(0), in(in) {}
		
		/** Indexing operator.
		 *  Returns character at specified position in the input stream, 
		 *  reading more input if necessary.
		 *  @param i		The index of the character to get
		 *  @return The i'th character of the input stream, or '\0' for i past 
		 *  		the end of file
		 *  @throws forgotten_state_error on i < str_begin (that is, asking for 
		 *  		input previously discarded)
		 */
		value_type operator[] (size_type i) {
			
			// Fail on forgotten index
			if ( i < str_off ) throw forgotten_state_error(i, str_off);
			
			// Get index into stored input
			uind ii = i - str_off;
			
			// Expand stored input if needed
			if ( ii > str.size() ) {
				uind n = ii - str.size();
				uind r = read(n);
				if ( r < n ) return '\0';
			}
			
			return str[ii];
		}
		
		/** Range operator.
		 *  Returns a pair of iterators, begin and end, containing up to the 
		 *  given number of elements, starting at the given index. The end 
		 *  iterator is not guaranteed to be dereferencable, though any 
		 *  iterator between it and the begin iterator (inclusive) is 
		 *  dereferencable. Returned begin and end iterators may be invalidated 
		 *  by calls to any non-const method of this class.
		 *  @param i		The index of the beginning of the range
		 *  @param n		The maximum number of elements in the range
		 *  @throws forgotten_state_error on i < str_begin (that is, asking for 
		 *  		input previously discarded)
		 */
		range_type range(size_type i, size_type n) {
			
			// Fail on forgotten index
			if ( i < str_off ) throw forgotten_state_error(i, str_off);
			
			// Get index into stored input
			uind ib = i - str_off;
			uind ie = ib + n;
			
			// Expand stored input if needed
			if ( ie > str.size() ) {
				uind nn = ie - str.size();
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
		
		/** Forgets all parsing state before the given index.
		 *  After this, reads or indexes before the given index will fail with 
		 *  an exception.
		 *  @param i		The index to forget to
		 */
		void forgetTo(size_type i) {
			
			// Ignore if already forgotten
			if ( i <= str_off ) return;
			
			// Get index in stored input to forget
			uind ii = i - str_off;
			
			// Forget stored input
			str.erase(str.begin(), str.begin()+ii);
		}
		
	private:
		/** Read more characters into the parser.
		 *  @param n		The number of characters to read
		 *  @return The number of characters read
		 */
		size_type read(size_type n) {
			char s[n];
			// Read into buffer
			in.read(s, n);
			// Count read characters
			uind r = in.gcount();
			// Add to stored input
			str.insert(str.end(), s, s+r);
			return r;
		}
		
	public:
		/** Current parsing location */
		size_type pos;
		
	private:
		/** Characters currently in use by the parser */
		std::deque<value_type> str;
		/** Offset of start of str from the beginning of the stream */
		size_type str_off;
		/** Input stream to read characters from */
		std::istream& in;
	}; /* class state */
	
} /* namespace parse */

#endif /* _EGG_PARSE_HPP_ */
