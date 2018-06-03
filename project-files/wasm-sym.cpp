
#include <memory>

#include "Path.h"
#include "interp.h"

using std::unique_ptr;
using namespace wabt;
using namespace wabt::interp;

int main(char**argc, int argv) {
	unique_ptr<Path> root(new Path());

	std::vector<uint8_t> file_data;
	
	wabt::Result result;
	
	result = ReadFile("add.wasm", &file_data);

	if (Succeeded(result)) {

	}
}