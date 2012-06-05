#include <iostream>

#include "egg.hpp"

int main(int argc, char** argv) {
	using namespace egg;
	
	Node root(GRAMMAR);

	std::cout << NodeDesc(root) << std::endl;

	return 0;
} /* main() */

