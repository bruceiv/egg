# Egg Parsing Expression Grammar Guide #

Egg is a parser generator for parsing expression grammars (PEGs). 
Its grammar is based on the grammar of Ian Piumarta's [`leg`](http://piumarta.com/software/peg/). 
Parsing expression grammars are a formalization of recursive top-down parsers; they are similar to context free grammars, with the primary difference being that alternation in a PEG is ordered, while it is unordered in context free grammars. 
Due to this quality, a PEG-based parser generator such as Egg does not need a separate lexer, and can do parsing in one pass.

## Matching Rules ##

An Egg grammar consists of a list of rules, where each rule gives an identifier to a sequence of matching statements. 
Rule identifiers consist of a letter or underscore followed by any number of further letters, digits, or underscores. 
The most basic matching statements are character and string literals, surrounded by single or double quotes, respectively; a period `.` matches any single character. 
A set of characters can be matched with a character range statement; this statement is enclosed in square brackets and contains a set of characters (or ranges of characters) to match; `[abcxyz]` and `[a-cx-z]` match the same sets of characters. 
A semicolon `;` is an empty matcher; it always matches without consuming any input; it can be safely placed at the end of any grammar rule for stylistic purposes, or used at the end of an alternation to match an empty case. 
Grammar rules can also be matched (possibly recursively) by writing their identifier. 
Matching statements can be made optional by following them with a `?`, repeatable by following them with `*`, or repeatable at least once with `+`; statements can also be grouped with parentheses. 
The `|` operator can be used to introduce alternation into grammar rules; this alternation is ordered - if a sequence is matched, then a later sequence will not be tested. As an example, consider the following two grammar rules: 

    g1 = ( 'a'* | "ab" ) 'c'
    g2 = ( "ab" | 'a'* ) 'c'

Of the above two rules, `g2` will match "abc", while `g1` will not, because the `'a'*` matcher will match, consuming the initial 'a', and then the following 'c' will not match, as the 'b' has yet to be consumed. 

PEGs also provide lookahead matchers, which match a given rule without consuming it; These can be constructed by prefixing a grammar rule with `&`. 
Similarly, a matcher prefixed with `!` does not consume the input, and only succeeds if the prefixed matcher doesn't match. 
These lookahead capabilities allow PEGs to match some grammars that cannot be represented by CFGs, such as the well-known a^n b^n c^n (n > 0), which can be matched by the following Egg grammar: 

    G = &(A 'c') 'a'+ B !.
    A = 'a' A 'b' | "ab"
    B = 'b' B 'c' | "bc"

A grammar rule may optionally be assigned a type by following the rule identifier with a colon and a second identifier. 
This second identifier can be a C++ type - namespaces & member typedefs are supported, as are templated classes, though pointer and reference types are not supported for implementation reasons (you may, however, use smart pointer classes). 
The type of a rule must also be default-constructable. 
The return value of this type can be accessed as the variable `psVal` in semantic actions inside the rule. 
Similarly, a matcher for a typed grammar rule can be bound to a variable by following it with a colon and a second identifer; the return value of the rule will be bound to the given variable. 
Character range matchers and the `.` any matcher may be bound to character variables using the same syntax. 
The following simple grammar functions as a basic calculator (an executable version of this calculator that also handles whitespace is available in `grammars/calc.egg`):

    sum : int =  prod : i { psVal = i; } ( 
                 '+' prod : i { psVal += i; } 
                 | '-' prod : i { psVal -= i; } )*
    prod : int = elem : i { psVal = i; } (
                 '*' elem : i { psVal *= i; } 
                 | '/' elem : i { psVal /= i; } )*
    elem : int = '(' sum : i ')' { psVal = i; }
                 | < '-'?[0-9]+ > : s { psVal = atoi(s.c_str()); }

A sequence of matching rules can also be surrounded by angle brackets `<` and `>`, denoting a capturing block; the closing bracket must be followed by a `:` bound string variable to bind the matched string to.

Finally, comments can be started with a `#`, they end at end-of-line.

## Error Handling ##

Rules can be given a name for error reporting by placing an _error string_ after the rule name (and type). 
An error string begins and ends with `` ` `` characters, and may contain any character except tabs and newlines (`` ` `` and `\` must be escaped as `` \` `` and `\\`, respectively). 
If a rule that is so annotated fails to match it will add its error string to the list of "expected" messages in the parser's error object at the current position. 
As a convenience, if you specify an empty error string, the name of the rule will be used. 

## Semantic Actions ##

Egg grammars may include semantic actions in a sequence of matching rules. 
These actions are surrounded with curly braces, and will be included in the generated parser at their place of insertion. 
Semantic actions have access to any bound variables in the current rule, as well as `ps`, the parser state object, and `psVal`, the return value for a typed rule. 
A subset of the interface for the parser state variables is below:

- `ps` - the state object - public interface is as follows:
  - `ps.posn()` - the current parser position
    - `p.line()` - the input line of position `p`
    - `p.col()` - the input column of position `p`
    - `p.index()` - the character index of position `p`
  - `ps.err` - The error information at the current point in the parse
    - `e.pos` - The position of the error
    - `e.expected` - A set of things expected at the error position
    - `e.messages` - A set of error messages at the error position
  - `ps()` - the character at the current position
  - `ps(p)` - the character at position p
  - `ps.range(p, n)` - returns a pair of iterators representing position `p` and `n` characters after position `p` (or the end of the input stream, if less than `n` characters)
  - `ps.string(p, n)` - the string represented by `ps.range(p, n)`
- `psVal` - the variable containing the return value of a typed rule; will be default-constructed by caller on its rule start.

You may also include a special semantic action before and after the grammar rules; these rules are delimited with `{$` and `$}` and will be placed before and after the generated rules. 
The usual `ps` variables are not defined in these actions.

## Egg Grammar ##

The following is an Egg grammar for Egg grammars - it is an authoritative representation of Egg syntax, and should also be an illustrative example of a moderately complex grammar (see `egg.egg` for how to build an abstract syntax tree from this grammar): 

    grammar =		_ out_action? rule+ out_action? end_of_file

    out_action =	OUT_BEGIN ( !OUT_END . )* OUT_END _
    
    rule =			rule_lhs choice
    
    rule_lhs =		identifier ( BIND type_id )? err_string? EQUAL
    
    identifier =	[A-Za-z_][A-Za-z_0-9]* _

    type_id =		identifier ( "::" _ type_id )* 
    					( '<' _ type_id ( ',' _ type_id )* '>' _ )?
    
    err_string =	'`' ( "\\\\" | "\\`" | ![`\t\n\r] . )* '`' _
    
    choice =		sequence ( PIPE sequence )*
    
    sequence =		( expression | action )+
    
    expression =	AND primary
    				| NOT primary 
    				| primary ( OPT | STAR | PLUS )? 
    
    primary =		!rule_lhs identifier ( BIND identifier )?
    					# above rule avoids parsing rule def'n as invocation
    				| OPEN choice CLOSE
    				| char_literal
    				| str_literal
    				| char_class ( BIND identifier )?
    				| ANY ( BIND identifier )?
    				| EMPTY
    				| BEGIN sequence END BIND identifier
    
    action =		!OUT_BEGIN '{' ( action | !'}' . )* '}' _
    
    char_literal =	'\'' character '\'' _
    
    str_literal =	'\"' character* '\"' _
    
    char_class =	'[' ( !']' char_range )* ']' _
    
    char_range =	character '-' character 
    				| character
    
    character =		'\\' [nrt\'\"\\]
    				| ![\'\"\\] .
    
	OUT_BEGIN =		"{%"
    OUT_END =		"%}"
    BIND =			':' _
    EQUAL =			'=' _
    PIPE =			'|' _
    AND =			'&' _
    NOT =			'!' _
    OPT =			'?' _
    STAR =			'*' _
    PLUS =			'+' _
    OPEN =			'(' _
    CLOSE =			')' _
    ANY =			'.' _
    EMPTY =			';' _
    BEGIN =			'<' _
    END =			'>' _
    
    _ =		 		( space | comment )*
    space =			' ' | '\t' | end_of_line
    comment =		'#' ( !end_of_line . )* end_of_line
    end_of_line = 	"\r\n" | '\n' | '\r'
    end_of_file = 	!.

