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

#include <functional>
#include <iterator>
#include <initializer_list>
#include <utility>

#include "bag.hpp"

/// A bag where each element is augmented with a hash value to facilitate more efficient searching
template<typename T, typename Hash = std::hash<T>, typename KeyEqual = std::equal_to<T>>
class hash_bag : private Hash, private KeyEqual {
	using impl_type = bag<std::pair<std::size_t, T>>;
	
	inline std::size_t h(const T& x) const { return Hash::operator()(x); }
	inline bool eq(const T& x, const T& y) const { return KeyEqual::operator()(x, y); }
	
public:
	using value_type = T;
	using size_type = typename impl_type::size_type;
	using difference_type = typename impl_type::difference_type;
	using reference = T&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;
	
	class iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
	friend hash_bag;
	friend class const_iterator;
		using wrapped_iterator = typename hash_bag<T, Hash, KeyEqual>::impl_type::iterator;
		
		iterator(const wrapped_iterator& it) : it(it) {}
	public:
		iterator(const iterator&) = default;
		iterator(iterator&&) = default;
		iterator& operator= (const iterator&) = default;
		iterator& operator= (iterator&&) = default;
		~iterator() = default;
		
		reference operator* () { return it->second; }
		pointer operator-> () { return &it->second; }
		
		iterator& operator++ () { ++it; return *this; }
		iterator operator++ (int) { iterator tmp = *this; ++it; return tmp; }
		iterator& operator-- () { --it; return *this; }
		iterator operator-- (int) { iterator tmp = *this; --it; return tmp; }
		
		bool operator== (const iterator& o) const { return it == o.it; }
		bool operator!= (const iterator& o) const { return it != o.it; }
		
	private:
		wrapped_iterator it;
	}; // iterator
	
	class const_iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
	friend hash_bag;
		using wrapped_iterator = typename hash_bag<T, Hash, KeyEqual>::impl_type::iterator;
		using wrapped_const_iterator = typename hash_bag<T, Hash, KeyEqual>::impl_type::const_iterator;
		
		const_iterator(const wrapped_iterator& it) : it(it) {}
		const_iterator(const wrapped_const_iterator& it) : it(it) {}
	public:
		const_iterator(const const_iterator&) = default;
		const_iterator(const_iterator&&) = default;
		const_iterator(const iterator& o) : it(o.it) {}
		const_iterator(iterator&& o) : it(o.it) {}
		const_iterator& operator= (const const_iterator&) = default;
		const_iterator& operator= (const_iterator&&) = default;
		const_iterator& operator= (const iterator& o) { it = o.it; return *this; }
		const_iterator& operator= (iterator&& o) { it = o.it; return *this; }
		~const_iterator() = default;
		
		const_reference operator* () const { return it->second; }
		const_pointer operator-> () const { return &it->second; }
		
		const_iterator& operator++ () { ++it; return *this; }
		const_iterator operator++ (int) { const_iterator tmp = *this; ++it; return tmp; }
		const_iterator& operator-- () { --it; return *this; }
		const_iterator operator-- (int) { const_iterator tmp = *this; --it; return tmp; }
		
		bool operator== (const const_iterator& o) const { return it == o.it; }
		bool operator!= (const const_iterator& o) const { return it != o.it; }
		
	private:
		wrapped_const_iterator it;
	}; // const_iterator
	
	hash_bag() = default;
	template<typename It> hash_bag(It first, It last) { insert(first, last); }
	hash_bag(const hash_bag&) = default;
	hash_bag(hash_bag&&) = default;
	hash_bag(std::initializer_list<T> sn) { insert(sn); }
	hash_bag& operator= (const hash_bag&) = default;
	hash_bag& operator= (hash_bag&&) = default;
	hash_bag& operator= (std::initializer_list<T> sn) { clear(); insert(sn); return *this; }
	void swap(hash_bag& o) { b.swap(o.b); }
	~hash_bag() = default;
	
	iterator begin() { return iterator{b.begin()}; }
	const_iterator begin() const { return const_iterator{b.cbegin()}; }
	const_iterator cbegin() const { return const_iterator{b.cbegin()}; }
	iterator end() { return iterator{b.end()}; }
	const_iterator end() const { return const_iterator{b.cend()}; }
	const_iterator cend() const { return const_iterator{b.cend()}; }
	
	bool empty() const { return b.empty(); }
	size_type size() const { return b.size(); }
	size_type max_size() const { return b.max_size(); }
	void reserve(size_type n) { b.reserve(n); }
	size_type capacity() const { return b.capacity(); }
	void shrink_to_fit() { b.shrink_to_fit(); }
	
	void clear() { b.clear(); }
	void insert(const T& x) { b.emplace(h(x), x); }
	void insert(T&& x) { b.emplace(h(x), x); }
	template<typename It> void insert(It first, It last) {
		while ( first != last ) { const T& x = *first; b.emplace(h(x), x); ++first; }
	}
	void insert(std::initializer_list<T> sn) { for (const T& x : sn) b.emplace(h(x), x); }
	template<class... Args> void emplace(Args&&... args) {
		T x{std::forward<Args>(args)...};
		b.emplace(h(x), x);
	}
	iterator erase(iterator it) { return iterator{b.erase(it.it)}; }
	size_type erase(const T& x) {
		std::size_t hx = h(x);
		size_type k = 0;
		auto it = b.begin();
		while ( it != b.end() ) {
			if ( hx == it->first && eq(x, it->second) ) {
				it = b.erase(it);
				++k;
			} else {
				++it;
			}
		}
		return k;
	}
	
	size_type count(const T& x) const {
		std::size_t hx = h(x);
		size_type k = 0;
		for (auto it = b.begin(); it != b.end(); ++it) {
			if ( hx == it->first && eq(x, it->second) ) ++k;
		}
		return k;
	}
	iterator find(const T& x) {
		std::size_t hx = h(x);
		auto it = b.begin();
		while ( it != b.end() ) {
			if ( hx == it->first && eq(x, it->second) ) break;
			++it;
		}
		return iterator{it};
	}
	const_iterator find(const T& x) const {
		std::size_t hx = h(x);
		auto it = b.begin();
		while ( it != b.end() ) {
			if ( hx == it->first && eq(x, it->second) ) break;
			++it;
		}
		return const_iterator{it};
	}
	
private:
	impl_type b;  ///< Underlying storage
}; // hash_bag
