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
#include "../visitors/printer.hpp"
#include "../visitors/dlf-printer.hpp"

static int _DBG_IDENT = 0;

#define DBG(msg) do { for(int _i = 0; _i < _DBG_IDENT; ++_i) std::cout << ' '; std::cout << msg; } while(false)
#define PRE_DBG(msg) do { _DBG_IDENT += 2; DBG(msg); } while(false)
#define POST_DBG(msg) do { DBG(msg); _DBG_IDENT -= 2; } while(false)

#define DBG_ARC(msg, arc) DBG(msg; dlf::printer::next(arc))
#define PRE_DBG_ARC(msg, arc) PRE_DBG(msg; dlf::printer::next(arc))
#define POST_DBG_ARC(msg, arc) POST_DBG(msg; dlf::printer::next(arc))

#define DBG_MATCHER(msg, matcher) DBG(msg; ::visitor::printer::print_single(matcher, std::cout, (_DBG_IDENT+3)/4); std::cout << std::endl)
#define PRE_DBG_MATCHER(msg, matcher) PRE_DBG(msg; ::visitor::printer::print_single(matcher, std::cout, (_DBG_IDENT+3)/4); std::cout << std::endl)
#define POST_DBG_MATCHER(msg, matcher) POST_DBG(msg; ::visitor::printer::print_single(matcher, std::cout, (_DBG_IDENT+3)/4); std::cout << std::endl)