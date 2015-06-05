#pragma once

/*
 * Copyright (c) 2015 Aaron Moss
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

#include <set>
#include <string>

namespace strings {
	
	/// A set of prefixes of strings
	class prefix_set {
		using impl_set = std::set<std::string>;  ///< Underlying set type
		
	public:
		// STL defines
		using value_type = impl_set::value_type;
		using size_type = impl_set::size_type;
		using difference_type = impl_set::difference_type;
		using reference = impl_set::reference;
		using const_reference = impl_set::const_reference;
		using pointer = impl_set::pointer;
		using const_pointer = impl_set::const_pointer;
		using iterator = impl_set::iterator;
		using const_iterator = impl_set::const_iterator;
		
	private:
		/// Insert a new prefix into the set.
		/// Fails if a prefix already exists, removes all prefixed strings.
		void insert_pre(const std::string& t) {
			// empty set can always have a prefix added
			if ( s.empty() ) { s.insert(t); return; }
			
			// If the prefix set already contains the empty string, can't add anything
			auto low = s.begin();
			if ( low->empty() ) return;
			
			// The empty string clears the set
			if ( t.empty() ) {
				s.clear();
				s.insert(t);
				return;
			}
			
			std::string::size_type i = 0;
			do {
				char tc = t[i], lc = (*low)[i];
				
				// If the lower bound is less than the new string, update the bound
				if ( lc < tc ) {
					low = s.lower_bound(t.substr(0,i+1));

					// If no bound, or shares less than i characters with t, not a prefix; insert
					if ( low == s.end() || low->size() < i ) { s.insert(t); return; }
					for (std::string::size_type j = 0; j < i; ++j) {
						if ( t[j] != (*low)[j] ) { s.insert(t); return; }
					}
					// check if the new bound is a prefix
					if ( low->size() == i ) return;

					lc = (*low)[i]; // lc >= tc, by construction
				}
				
				// If the lower bound is greater than the new string, add the new string
				if ( lc > tc ) { s.insert(t); return; }
				
				// Otherwise proceed again with the next character
				++i;
			} while ( i < t.size() && i < low->size() );
			
			// If we reach here we've exhausted the input string, the lower bound, or both.
			// The first i characters of t and *low also match.
			
			// break on lower bound a prefix
			if ( i == low->size() ) return;
			
			// Must have exhausted t, then; low is first prefixed string, find last one and erase range
			std::string t2(t); ++t2.back();  // First string after set prefixed by t
			auto high = s.lower_bound(t2);   // First string not prefixed by t
			s.erase(low, high);
			s.insert(t);
		}
		
		/// Finds a prefix of the provided string, or a prefixed string
		iterator find_pre(const std::string& t) {
			auto low = s.begin();
			if ( t.empty() || s.empty() || low->empty() ) return low;
			
			std::string::size_type i = 0;
			do {
				char tc = t[i], lc = (*low)[i];
				
				// If the lower bound is less than the search string, update the bound
				if ( lc < tc ) {
					low = s.lower_bound(t.substr(0,i+1));

					// If no bound, or shares less than i characters with t, not a prefix; fail
					if ( low == s.end() || low->size() < i ) return s.end();
					for (std::string::size_type j = 0; j < i; ++j) {
						if ( t[j] != (*low)[j] ) return s.end();
					}
					// check if the new bound is a prefix
					if ( low->size() == i ) return low;

					lc = (*low)[i]; // lc >= tc, by construction
				}
				
				// If the lower bound is greater than the search string, fail
				if ( lc > tc ) return s.end();
				
				// Otherwise proceed again with the next character
				++i;
			} while ( i < t.size() && i < low->size() );
			
			// If we reach here we've exhausted the search string, the lower bound, or both.
			// The first i characters of t and *low also match, which means one is a prefix of the other.
			return low;
		}
	public:
		prefix_set() = default;
		template<typename It> prefix_set(It first, It last) : s() { insert(first, last); }
		prefix_set(const prefix_set&) = default;
		prefix_set(prefix_set&&) = default;
		prefix_set(std::initializer_list<std::string> sn) : s() { insert(sn); }
		prefix_set& operator= (const prefix_set&) = default;
		prefix_set& operator= (prefix_set&&) = default;
		prefix_set& operator= (std::initializer_list<std::string> sn) {
			clear();
			insert(sn);
			return *this;
		}
		void swap(prefix_set& o) { s.swap(o.s); }
		~prefix_set() = default;

		iterator begin() { return s.begin(); }
		const_iterator begin() const { return s.cbegin(); }
		const_iterator cbegin() const { return s.cbegin(); }
		iterator end() { return s.end(); }
		const_iterator end() const { return s.cend(); }
		const_iterator cend() const { return s.cend(); }

		bool empty() const { return s.empty(); }
		size_type size() const { return s.size(); }

		void clear() { s.clear(); }
		void insert(const std::string& a) { insert_pre(a); }
		template<typename It> void insert(It first, It last) {
			while ( first != last ) { insert(*first); ++first; }
		}
		void insert(std::initializer_list<std::string> sn) { for (const std::string& a : sn) insert(a); }
		
		size_type count(const std::string& t) const { return find_pre(t) == s.end() ? 0 : 1; }
		iterator find(const std::string& t) { return find_pre(t); }
		const_iterator find(const std::string& t) const { return find_pre(t); }
		
		/// Checks if the sets represented by the prefixes intersect
		bool intersects(const prefix_set& o) const {
			if ( empty() ) return false;
			if ( size() > o.size() ) return o.intersects(*this);
			for (const std::string& t : s) if ( o.find(t) != o.end() ) return true;
			return false;
		}
		/// Returns a copy of this set with all the strings prefixed by t
		prefix_set prefixed_with(const std::string& t) {
			prefix_set n;
			for (const std::string& r : s) n.s.insert(t + r);
			return n;
		}
		prefix_set prefixed_with(char c) {
			std::string pre(1, c);
			return prefixed_with(pre);
		}
		prefix_set prefixed_with(char c, char e) {
			prefix_set n;
			do {
				std::string pre(1, c);
				for (const std::string& r : s) n.s.insert(pre + r); 
				if ( c == e ) break;
				++c;
			} while (true);
			return n;
		}
		
	private:
		impl_set s;
	}; // prefix_set
}