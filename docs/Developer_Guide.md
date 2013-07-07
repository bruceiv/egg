# Egg Developer Guide #

This document is for people who wish to modify or contribute to the Egg project; developers who wish to use Egg should read the Readme and the Grammar Guide instead. 
Developers who wish to contribute to the project should read the Readme and the Grammar Guide first.

## Project Overview ##

Any Egg-generated parser (including the parser for Egg itself) uses the contents of the `parse` namespace from `parse.hpp` to provide a common interface and encapsulate input state. 
Egg grammars compile to headers which depend on `parse.hpp`; these headers define a namespace for the grammar (generally named the same as the grammar) which contains a function for each grammar rule. 
These functions take a `parse::state` reference encapsulating the input state as a parameter, and returns a `parse::result` - the `parse::result` contains a value (which may optionally be given any default constructable C++ type), or evaluates to false if the rule did not match. 
The Readme file and `parse.hpp` both have further information on these classes. 

The Egg grammar for Egg (defined in `egg.egg` and compiled to `egg.hpp`) builds an abstract syntax tree for the Egg grammar file it parses. 
The AST classes are all in the `ast` namespace defined in `ast.hpp`, and use the visitor pattern. 
All AST nodes inherit from `ast::matcher`, which defines a `void accept(ast::visitor*)` method. 
`ast::visitor` is the abstract base class of all visitors, but the `ast::default_visitor` method is defined with empty implementations of all the methods if desired. 
`ast::grammar_rule` and `ast::grammar` are not subclasses of `ast::matcher`, and must be handled differently - see `ast.hpp` for details.

Various visitors for the Egg AST are defined in the `visitors` directory. 
`printer.hpp` contains `visitor::printer`, a pretty-printer for Egg grammars, `normalizer.hpp` contains `visitor::normalizer`, which performs some basic simplifications on an Egg AST, and `compiler.hpp` contains `visitor::compiler` and some related classes, which together form a code generator for compiling Egg grammars. 
The Parsing Expression Grammar model that Egg uses is a formalization of recursive descent parsing, so the generated code follows this pattern. 
Egg uses anonymous C++11 lambdas to implement parenthesized subexpressions and some other constructs, so as not to pollute the grammar namespace unneccesarily. 

The Egg executable itself is defined in `main.cpp` in the root directory; this file is mostly concerned with command line argument parsing, and provides an executable interface to either pretty-print or compile an Egg grammar.

Finally, the `utils` directory contains some utility headers common to various parts of the project (currently just string manipulation), and the `grammars` directory contains some example grammars and tests for Egg. 
These grammars include simple test harnesses in their post-action for the sake of brevity; this is not reccomended usage, as Egg is designed to generate headers, and compilers complain about the `#pragma once` directive employed in a main file. 
The `grammars/tests` directory contains sample input (`*.in.txt`) and correct output (`*.out.txt`) for each grammar; these may be used for regression testing with the `test` target of `grammars/Makefile`. 

## Contributing ##

Egg is fairly relaxed in terms of coding style - emulate the existing code and you should be fine. 
As guidelines, I tend to use K&R style braces, 100 character lines, 4 character tabs for indentation and spaces after that for alignment as neccessary. 
The other main quirk of my personal coding style which this project follows is placing spaces on both sides of the parentheses for `if`, `while`, and `switch` statements (but only the outside side for `for` loops).

Egg is released under a MIT licence, so naturally all contributions should be available under that licence. 

If you fix a bug please include test cases that cover it int the `test` target of `grammars/Makefile` (more tests in general are welcome). 

If you are looking for a contribution to make, the `TODO` file in this directory contains a current list of known bugs, desired features, and possible code refactorings; check it out for ideas. 