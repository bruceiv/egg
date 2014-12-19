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
#include "../visitors/dlf-printer.hpp"

static int _DBG_IDENT = 0;

#define IN_DBG(msg, act) do { for(int _i = 0; _i < _DBG_IDENT; ++_i) std::cout << ' '; std::cout << msg; act; } while(false)
#define PRE_DBG(msg, act) do { _DBG_IDENT += 2; IN_DBG(msg, act); } while(false)
#define POST_DBG(msg, act) do { IN_DBG(msg, act); _DBG_IDENT -= 2; } while(false)

#define IN_DBG_ARC(msg, arc) IN_DBG(msg, dlf::printer::next(arc))
#define PRE_DBG_ARC(msg, arc) PRE_DBG(msg, dlf::printer::next(arc))
#define POST_DBG_ARC(msg, arc) POST_DBG(msg, dlf::printer::next(arc))
