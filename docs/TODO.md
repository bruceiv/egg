## Feature Wishlist ##
- add &{ /\* actions \*/ } to the language
- Packrat parser state
- Look at adding cut syntax
- Unicode string support
  - Include Unicode escapes for character literals
- Add interpreter visitor (this may be non-trivial)
- Add doxygen-generated docs to the docs folder

## Bugs ##
- Cannot include ']' in a character class - should include an escape.
- Non-syntactic '{' and '}' characters in actions (e.g. those in comments or string literals) may break the parser if unmatched.

## Code Cleanup ##
- Maybe move to `unique_ptr` from `shared_ptr`
- Replace `ast::make_ptr()` and `ast::as_ptr()` with standard library equivalents
- Inline parse.hpp in generated grammars
  - This may have licencing ramifications - consider a Bison-style exception
- Modify makefile to remake `egg` from `egg.egg` or `egg-back.hpp` as appropriate
- Maybe add flag to make "#pragma once" optional in generated files