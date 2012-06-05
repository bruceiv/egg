#ifndef _EGG_EGG_HPP_
#define _EGG_EGG_HPP_

#include <string>

namespace egg {

	typedef std::string String;

	/** Type of AST node  */
	enum NodeType {
		GRAMMAR,		/**< egg grammar  */
		NONTERMINAL,	/**< nonterminal */
		TERMINAL		/**< terminal */
	};
	
	/** Abstract syntax tree node */
	struct Node {
		
		Node(NodeType type);

		NodeType type;
	}; /* class Node */
	
	/** Gets the description corresponding to a given node type */
	String NodeDesc(NodeType type);
	/** Convenience for NodeDesc(node.type) */
	String NodeDesc(Node const& node);

} /* namespace egg */

#endif /* _EGG_EGG_HPP_ */

