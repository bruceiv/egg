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
#include <utility>
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

	arc d{a};
	test.equal_set(d.blocking, list({as_cut(m)}), "copy d, d.blocking");
	test.equal_set(as_cut(m)->blocked, list({&a, &d}), "copy d, m.blocked");

	arc e{std::move(d)};
	test.equal_set(e.blocking, list({as_cut(m)}), "move e, e.blocking");
	test.equal_set(as_cut(m)->blocked, list({&a, &e}), "move e, m.blocked");

	b.block(n);
	b = e;
	test.equal_set(b.blocking, list({as_cut(m)}), "copy assign b, b.blocking");
	test.equal_set(as_cut(m)->blocked, list({&a, &b, &e}), "copy assign b, m.blocked");
	test.equal_set(as_cut(n)->blocked, list<arc*>({}), "copy assign b, n.blocked");

	b.clear_blocks();
	test.equal_set(b.blocking, list<cut_node*>({}), "b.clear_blocks(), b.blocking");
	test.equal_set(as_cut(m)->blocked, list({&a, &e}), "b.clear_blocks(), m.blocked");

	e.block(n);
	b = std::move(e);
	test.equal_set(b.blocking, list({as_cut(n), as_cut(m)}), "move assign b, b.blocking");
	test.equal_set(as_cut(m)->blocked, list({&a, &b}), "move assign b, m.blocked");
	test.equal_set(as_cut(n)->blocked, list({&b}), "move assign b, n.blocked");

	a.swap(b);
	test.equal_set(a.blocking, list({as_cut(n), as_cut(m)}), "a.swap(b), a.blocking");
	test.equal_set(b.blocking, list({as_cut(m)}), "a.swap(b), b.blocking");
	test.equal_set(as_cut(n)->blocked, list({&a}), "a.swap(b), n.blocked");
	test.equal_set(as_cut(m)->blocked, list({&a, &b}), "a.swap(b), m.blocked");
	
	std::swap(a, b);
	test.equal_set(a.blocking, list({as_cut(m)}), "std::swap(a, b), a.blocking");
	test.equal_set(b.blocking, list({as_cut(n), as_cut(m)}), "std::swap(a, b), b.blocking");
	test.equal_set(as_cut(n)->blocked, list({&b}), "std::swap(a, b), n.blocked");
	test.equal_set(as_cut(m)->blocked, list({&a, &b}), "std::swap(a, b), m.blocked");

	arc f{b};
	f.block();
	test.check(f.blocked(), "f.block()/blocked()");
	f.clear_blocks();

	ptr<node> l = cut_node::make(arc{}, 3);
	b.block_all(cutset{as_cut(n), as_cut(l)});
	test.equal_set(b.blocking, list({as_cut(n), as_cut(m), as_cut(l)}), "b.block_all(), b.blocking");
	test.equal_set(as_cut(n)->blocked, list({&b}), "b.block_all(), n.blocked");
	test.equal_set(as_cut(l)->blocked, list({&b}), "b.block_all(), l.blocked");

	ptr<node> k = cut_node::make(arc{}, 4);
	b.block_only(cutset{as_cut(n), as_cut(k)});
	test.equal_set(b.blocking, list({as_cut(n)}), "b.block_only(), b.blocking");
	test.equal_set(as_cut(n)->blocked, list({&b}), "b.block_only(), n.blocked");
	test.equal_set(as_cut(m)->blocked, list({&a}), "b.block_only(), m.blocked");
	test.equal_set(as_cut(l)->blocked, list<arc*>({}), "b.block_only(), l.blocked");
	test.equal_set(as_cut(k)->blocked, list<arc*>({}), "b.block_only(), k.blocked"); 

	test.cleanup();
}

void test_cut(tester& test) {
	using namespace dlf;	

	test.setup("cut_node");

	arc a{match_node::make()};
	ptr<node> n = cut_node::make(arc{}, 0, cut_node::blockset{&a});
	test.equal_set(as_cut(n)->blocked, list({&a}), "make with blockset, n.blocked");
	test.equal_set(a.blocking, list({as_cut(n)}), "make with blockset, a.blocking");

	arc b{char_node::make(arc{}, 'b')};
	{
		cut_node nn(*as_cut(n));
		test.equal_set(nn.blocked, list({&a}), "copy construct, nn.blocked");
		test.equal_set(a.blocking, list({as_cut(n), &nn}), "copy construct, a.blocking");
	
		cut_node nm(std::move(nn));
		test.equal_set(nm.blocked, list({&a}), "move construct, nm.blocked");
		test.equal_set(a.blocking, list({as_cut(n), &nm}), "move construct, a.blocking");

		cut_node m(arc{}, 1, cut_node::blockset{&b});
		m = nm;
		test.equal_set(m.blocked, list({&a}), "copy assign, m.blocked");
		test.equal_set(a.blocking, list({as_cut(n), &nm, &m}), "copy assign, a.blocking");
		test.equal_set(b.blocking, list<cut_node*>({}), "copy assign, b.blocking");

		cut_node l(arc{}, 2, cut_node::blockset{&b});
		l = std::move(nm);
		test.equal_set(l.blocked, list({&a}), "move assign, l.blocked");
		test.equal_set(a.blocking, list({as_cut(n), &m, &l}), "move assign, a.blocking");
		test.equal_set(b.blocking, list<cut_node*>({}), "move assign, b.blocking");
	}
	test.equal_set(a.blocking, list({as_cut(n)}), "delete cuts, remove from blockset");

	test.cleanup();
}

int main(int argc, char** argv) {
	tester test;

	test_arc(test);
	test_cut(test);
	
	return test.success() ? 0 : 1;
}

