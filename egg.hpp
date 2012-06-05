#ifndef _EGG_EGG_HPP_
#define _EGG_EGG_HPP_

#include <string>

namespace egg {

	using std::string;

	/** Type of AST node  */
	enum NodeType {
		GRAMMAR,	/**< egg grammar  */
		NONTERMINAL,	/**< nonterminal */
		TERMINAL	/**< terminal */
	};

	/** Abstract syntax tree node */
	struct Node {
		
		Node(NodeType type, string desc);

		NodeType type;
		string desc;
	}; /* class Node */

} /* namespace egg */

#endif /* _EGG_EGG_HPP_ */

