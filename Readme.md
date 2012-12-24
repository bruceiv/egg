# Egg Parsing Expression Grammar Generator #

Egg is a parser generator for parsing expression grammars (PEGs). 
Its grammar is based on the grammar for Leg at http://piumarta.com/software/peg/peg.1.html . 
Parsing expression grammars are a formalization of recursive top-down parsers; they are similar to context free grammars, with the primary difference being that alternation in a PEG is ordered, while it is unordered in context free grammars. 
Due to this quality, a PEG-based parser generator such as Egg does not need a separate lexer, and can do parsing in one pass.

## Matching Rules ##

An Egg grammar consists of a list of rules, where each rule gives an identifier to a sequence of matching statements. 
Rule identifiers consist of a letter or underscore followed by any number of further letters, digits, or underscores. 
The most basic matching statements are character and string literals, surrounded by single or double quotes, respectively; a period '.' matches any single character. 
A semicolon ';' is an empty matcher; it always matches without consuming any input; it can be safely placed at the end of any grammar rule for stylistic purposes, or used at the end of an alternation to match an empty case. 
Grammar rules can also be matched (possibly recursively) by writing their identifier. 
Matching statements can be made optional by following them with a '?', repeatable by following them with '*', or repeatable at least once with '+'; statements can also be grouped with parentheses. 
The '|' operator can be used to introduce alternation into grammar rules; this alternation is ordered - if a sequence is matched, then a later sequence will not be tested. As an example, consider the following two grammar rules: 

    g1 = ( 'a'* | "ab" ) 'c'
    g2 = ( "ab" | 'a'* ) 'c'

Of the above two rules, g2 will match "abc", while g1 will not, because the 'a'* matcher will match, consuming the initial 'a', and then the following 'c' will not match, as the 'b' has yet to be consumed. 

PEGs also provide lookahead matchers, which match a given rule without consuming it; These can be constructed by prefixing a grammar rule with '&'. 
Similarly, a matcher prefixed with '!' does not consume the input, and only succeeds if the prefixed matcher doesn't match. 
These lookahead capabilities allow PEGs to match some grammars that cannot be represented by CFGs, such as the well-known a^n b^n c^n, which can be matched by the following Egg grammar: 

    G = &A 'a'* B
    A = 'a' A 'b' | ; # ; denotes end of rule, # introduces a comment
    B = 'b' B 'c' | ;

A sequence of matching rules can also be bracketed by '<' and '>', denoting a capturing block; the string that matches the rules inside the capturing block will be provided to the parser for use in its semantic actions.

A grammar rule may optionally be assigned a type by following the rule identifier with a colon and a second identifier. 
The return value of this type can be accessed as the variable "psVal" in semantic actions inside the rule. 
Similarly, a matcher for a typed grammar rule can be bound to a variable by following it with a colon and a second identifer; the return value of the rule will be bound to the given variable. 
Note that the identfier for the type of the rule must be a valid Egg identifier - C++ types that do not fit this pattern (such as templated types) can be aliased to a valid identifier using typedef. 
The following simple grammar functions as a basic calculator:

    sum : int =  prod : i { psVal = i; } ( 
                 '+' prod : i { psVal += i; } 
                 | '-' prod : i { psVal -= i; } )*
    prod : int = elem : i { psVal = i; } (
                 '*' elem : i { psVal *= i; } 
                 | '/' elem : i { psVal /= i; } )*
    elem : int = '(' sum : i ')' { psVal = i; }
                 | < [0-9]+ > { psVal = atoi(psCatch.c_str()); } 

## Semantic Actions ##

Egg grammars may include semantic actions in a sequence of matching rules. 
These actions are surrounded with curly braces, and will be included in the generated parser at their place of insertion. 
Variables made available to the semantic actions include the following: 
* ps - the parser object - public interface is as follows:
  * ps.pos - the current parser position
  * ps[i] - the i'th character of the input stream
  * ps.range(i, n) - returns a pair of iterators representing index i and n characters after index i (or the end of the input stream, if less than n characters)
  * ps.string(i, n) - the string represented by ps.range(i, n)
* psStart - the index of the start of the current match (or parenthesized matcher)
* included if after a capture:
  * psCatch - the index of the start of the most recent capture (also valid inside capturing sequences)
  * psCatchLen - the length of the most recent capture
  * psCapture - the string contained in the current capture
* psVal - the variable containing the return value of a typed rule

You may also include a special semantic action before and after the grammar rules; these rules are delimited with "{$" and "$}" and will be placed before and after the generated rules. 
The usual ps* variables are not defined in these actions.

## Egg Grammar ##

The following is an Egg grammar for Egg grammars - it is the authoritative representation of Egg syntax, and should also be an illustrative example of a moderately complex grammar: 

    grammar =		_ rule+ end_of_file
    
    rule =			bindable_id EQUAL choice SEMI?
    
    identifier =	< [A-Za-z_][A-Za-z_0-9]* > _

    bindable_id =	identifier ( ':' _ identifier )?
    
    choice =		sequence ( PIPE sequence )*
    
    sequence =		( expression | action )+
    
    expression =	( AND | NOT )? primary ( OPT | STAR | PLUS )? _
    
    primary =		bindable_id !EQUAL
    				| OPEN choice CLOSE
    				| char_literal
    				| str_literal
    				| char_class
    				| ANY
    				| BEGIN sequence END
    
    action =		'{' < ( action | !'}' . )* > '}' _
    
    char_literal =	'\'' < character > '\'' _
    
    str_literal =	'\"' < character* > '\"' _
    
    char_class =	'[' < ( !']' char_range )* > ']' _
    
    char_range =	character '-' character 
    				| character
    
    character =		'\\' [nrt\'\"\\]
    				| ![\'\"\\] .
    
    EQUAL =			'=' _
    SEMI =			';' _
    PIPE =			'|' _
    AND =			'&' _
    NOT =			'!' _
    OPT =			'?' _
    STAR =			'*' _
    PLUS =			'+' _
    OPEN =			'(' _
    CLOSE =			')' _
    ANY =			'.' _
    BEGIN =			'<' _
    END =			'>' _
    
    _ =		 		( space | comment )*
    space =			' ' | '\t' | end_of_line
    comment =		'#' ( !end_of_line . )* end_of_line
    end_of_line = 	"\r\n" | '\n' | '\r'
    end_of_file = 	!.

