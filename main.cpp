#include <iostream>

#include "egg.hpp"

int main(int argc, char** argv) {
	
	if ( ! egg::parse(std::cin) ) {
		std::cout << "\tPARSE FAILURE" << std::endl;
	}

	return 0;
} /* main() */

