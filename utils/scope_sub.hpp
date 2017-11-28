#pragma once

#include <utility>

/// Substitutes a value for a new one, resetting once leaving scope
template<typename T>
struct ScopeSub {
	T& val;
	T oldVal;

	ScopeSub( T& target, T&& sub ) : val(target), oldVal( std::move(target) ) {
		target = std::move(sub);
	}

	~ScopeSub() { val = std::move(oldVal); }
};

template<typename T>
ScopeSub<T> sub_in_scope( T& target, T&& sub ) {
	return { target, std::move(sub) };
}
