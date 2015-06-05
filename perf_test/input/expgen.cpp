#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char** argv) {
	int n = atoi(argv[1]);
	string as(n, 'a');
	string cs(n, 'c');
	cout << as << cs << endl;
}
