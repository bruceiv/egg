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

#include <sstream>
#include <string>

namespace strings {
	using std::string;
	using std::stringstream;

	/** Returns a string representing the given character with all special 
	 *  characters '\n', '\r', '\t', '\\', '\'', and '\"' backslash-escaped. */
	string escape(const char c) {
		switch ( c ) {
		case '\n': return "\\n";
		case '\r': return "\\r";
		case '\t': return "\\t";
		case '\\': return "\\\\";
		case '\'': return "\\\'";
		case '\"': return "\\\"";
		default:   return string(1, c);
		}
	}

	/** Returns a string representing the given string with all special 
	 *  characters '\n', '\r', '\t', '\\', '\'', and '\"' backslash-escaped. */
	string escape(const string& s) {
		stringstream ss;
		for (auto iter = s.begin(); iter != s.end(); ++iter) {
			ss << escape(*iter);
		}
		return ss.str();
	}

	/** Converts one of the characters 'n', 'r', 't' to the escaped character 
	 *  '\n', etc. Non-escaped characters will be returned as is. */
	char unescaped_char(const char c) {
		switch ( c ) {
		case 'n': return '\n';
		case 'r': return '\r';
		case 't': return '\t';
		default: return c;
		}
	}

	string unescape(const string& s) {
		stringstream ss;
		for (auto it = s.begin(); it != s.end(); ++it) {
			char c = *it;
			if ( c == '\\' ) {
				++it;
				if ( it == s.end() ) break;
				c = *it;
				ss << unescaped_char(c);
			} else {
				ss << c;
			}
		}
		return ss.str();
	}

	/** Replaces all sequences of newlines with spaces. */
	string single_line(const string& s) {
		stringstream ss;
		bool hadLineBreak = false;
		for (auto iter = s.begin(); iter != s.end(); ++iter) {
			char c = *iter;
			if ( c == '\n' || c == '\r' ) { 
				if ( !hadLineBreak ) { ss << ' '; }
				hadLineBreak = true;
			} else {
				ss << c;
				hadLineBreak = false;
			}
		}
		return ss.str();
	}
} /* namespace strings */

