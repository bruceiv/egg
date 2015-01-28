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

/** Tests for DLF code. Returns 0 for all passed, 1 otherwise */

#include <initializer_list>
#include <vector>

#include "../dlf.hpp"

#include "../utils/test.hpp"

template<typename T>
std::vector<T> list(std::initializer_list<T> ls) { return std::vector<T>(ls); }

dlf::cut_node* as_cut(dlf::ptr<dlf::node> n) { return dlf::as_ptr<dlf::cut_node>(n).get(); }

void test_arc(tester& test) {
	using namespace dlf;	

	test.setup("arc");

	cut_node* cn = new cut_node(arc{}, 0);
	ptr<node> n{cn};
	test.equal(cn, as_ptr<cut_node>(n).get(), "gets input pointer back from ptr<node>::as_ptr<X>().get()");

	arc a{fail_node::make()};
	a.block(n);
	test.equal_set(a.blocking, list({cn}), "got single node back from cutset");
	test.equal_set(cn->blocked, list({&a}), "got single arc back from blockset");

	a.block(n);
	test.equal_set(a.blocking, list({cn}), "got readded node back from cutset");
	test.equal_set(cn->blocked, list({&a}), "got single arc back from blockset");

	ptr<node> m = cut_node::make(arc{}, 1);
	a.block(m);
	test.equal_set(a.blocking, list({cn, as_cut(m)}), "got new node back from cutset");
	test.equal_set(as_cut(m)->blocked, list({&a}), "got new arc back from blockset");

	arc b{fail_node::make()};
	b.block(n);
	test.equal_set(b.blocking, list({cn}), "b.block(n), b.blocking");
	test.equal_set(cn->blocked, list({&a, &b}), "b.block(n), n.blocked");

	n.reset();
	test.equal_set(a.blocking, list({as_cut(m)}), "n.reset, a.blocking");
	test.equal_set(b.blocking, list<cut_node*>({}), "n.reset, b.blocking");

	n = cut_node::make(arc{}, 2);
	cn = nullptr;
	{
		arc c{fail_node::make(), cutset{as_cut(n), as_cut(m)}};
		test.equal_set(c.blocking, list({as_cut(n), as_cut(m)}), "c, c.blocking");
		test.equal_set(as_cut(n)->blocked, list({&c}), "c, n.blocked");
		test.equal_set(as_cut(m)->blocked, list({&a, &c}), "c, m.blocked");
	}
	test.equal_set(as_cut(n)->blocked, list<arc*>({}), "~c, n.blocked");
	test.equal_set(as_cut(m)->blocked, list({&a}), "~c, m.blocked");

	test.cleanup();
}

int main(int argc, char** argv) {
	tester test;

	test_arc(test);
	
	return test.success() ? 0 : 1;
}

