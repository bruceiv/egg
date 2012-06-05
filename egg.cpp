#include "egg.hpp"

namespace egg {

	Node::Node(NodeType type) : type(type) {}
	
	String NodeDesc(NodeType type) {
		switch ( type ) {
			case GRAMMAR:		return "egg grammar";
			case NONTERMINAL:	return "nonterminal";
			case TERMINAL:		return "terminal";
		}
		return "";
	}
	
	String NodeDesc(const egg::Node& node) {
		return NodeDesc(node.type);
	}


} /* namespace egg */

