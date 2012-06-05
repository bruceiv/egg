#include <iostream>

#include "egg.hpp"

int main(int argc, char** argv) {
	using namespace egg;
	
	Node root(GRAMMAR, "grammar");

	std::cout << root.desc << std::endl;

	return 0;
} /* main() */

