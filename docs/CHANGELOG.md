# Changelog #

## v0.2.1 ##

- Added `@` and `~` error message inserters to Egg grammar.
- Added `` `error` `` rule error names to the Egg grammar.

## v0.2.0 ##

- Ground-up rewrite to parser code, too many breaking changes to list here (though there are now syntactic constructs for a lot of the references to parser internals in the Egg grammar, so hopefuly future breaking changes can be done more behind the scenes). 
- Changed parser namespace to `parser` from `parse`.
- Changed position type to `parser::posn` struct with line and column numbers.
- Eliminated `parse::result<T>` type, replaced it with an extra reference parameter to nonterminals (which now return `bool`).
- Added error reporting structure to parser state with "expected" and "messages" error sets (uses Ford's "last non-empty error" heuristic for merging errors).
- Changed parsers to be based on parser combinators typed as `std::function<bool(parser::state&)>` - this is likely less performant, and removes the ability to usefully define the old `psStart` start position variable, but does allow quicker iteration on the codebase.
  I intend to re-implement a direct code generator at some point in the future, and move the combinators off into an optional header.
- Added ability to bind variables to `.` expression, character classes, and capturing matchers. 
  It is now mandatory to bind a string variable to capturing matchers, this replaces the old `psCatch`, `psCatchLen`, and `psCapture` variables.

## v0.1.0 ##

- Initial release.