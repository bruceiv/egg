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

#include <initializer_list>
#include <utility>
#include <vector>

/// A basic collection that does not maintain uniqueness or a consistent iteration order.
/// Insertion and deletion happen in amortized constant time.
template<typename T>
class bag {
	using impl_type = typename std::vector<T>;
public:
	using value_type = T;
	using size_type = typename impl_type::size_type;
	using difference_type = typename impl_type::difference_type;
	using reference = typename impl_type::reference;
	using const_reference = typename impl_type::const_reference;
	using pointer = typename impl_type::pointer;
	using const_pointer = typename impl_type::const_pointer;
	using iterator = typename impl_type::iterator;
	using const_iterator = typename impl_type::const_iterator;
	
	bag() = default;
	template<typename It> bag(It first, It last) : v(first, last) {}
	bag(const bag&) = default;
	bag(bag&&) = default;
	bag(std::initializer_list<T> sn) : v{sn} {}
	bag& operator= (const bag&) = default;
	bag& operator= (bag&&) = default;
	bag& operator= (std::initializer_list<T> sn) { v = sn; return *this; }
	void swap(bag& o) { v.swap(o.v); }
	~bag() = default;
	
	iterator begin() { return v.begin(); }
	const_iterator begin() const { return v.cbegin(); }
	const_iterator cbegin() const { return v.cbegin(); }
	iterator end() { return v.end(); }
	const_iterator end() const { return v.cend(); }
	const_iterator cend() const { return v.cend(); }
	T* data() { return v.data(); }
	const T* data() const { return v.data(); }

	bool empty() const { return v.empty(); }
	size_type size() const { return v.size(); }
	size_type max_size() const { return v.max_size(); }
	void reserve(size_type n) { v.reserve(n); }
	size_type capacity() const { return v.capacity(); }
	void shrink_to_fit() { v.shrink_to_fit(); }
	
	void clear() { v.clear(); }
	void insert(const T& x) { v.push_back(x); }
	void insert(T&& x) { v.push_back(std::move(x)); }
	void insert(size_type n, const T& x) { for (size_type i = 0; i < n; ++i) v.push_back(x); }
	template<typename It> void insert(It first, It last) {
		while ( first != last ) { v.push_back(*first); ++first; }
	}
	void insert(std::initializer_list<T> sn) { for (const T& x : sn) v.push_back(x); }
	template<class... Args> void emplace(Args&&... args) { v.emplace_back(std::forward<Args>(args)...); }
	iterator erase(iterator it) {
		if ( it - begin() == size() - 1 ) {
			v.pop_back();
			return end();
		} else {
			std::swap(*it, v.back());
			v.pop_back();
			return it;
		}
	}
	size_type erase(const T& x) {
		size_type k = 0;
		iterator it = begin();
		while ( it != end() ) {
			if ( *it == x ) {
				it = erase(it);
				++k;
			} else {
				++it;
			}
		}
		return k;
	}
	
	size_type count(const T& x) const {
		size_type k = 0;
		for (const_iterator it = begin(); it != end(); ++it) if ( *it == x ) ++k;
		return k;
	}
	iterator find(const T& x) {
		iterator it = begin();
		while ( it != end() ) { if ( *it == x ) break; ++it; }
		return it;
	}
	const_iterator find(const T& x) const {
		const_iterator it = begin();
		while ( it != end() ) { if ( *it == x ) break; ++it; }
		return it;
	}
	
private:
	impl_type v;  ///< Underlying storage
}; // bag<T>

template<typename T>
void swap(bag<T>& lhs, bag<T>& rhs) { lhs.swap(rhs); }
