
#include <memory>

#include "Path.h"
#include "interp.h"

using std::unique_ptr;
using namespace wabt;
using namespace wabt::interp;

int main(char**argc, int argv) {
	// Beginning of our path tree
	unique_ptr<Path> root(new Path());


	std::vector<uint8_t> file_data;
	
	wabt::Result result;
	
	// Read in the wasm file:
	result = ReadFile("add.wasm", &file_data);

	if (Succeeded(result)) {
		const bool kReadDebugNames = true;
		const bool kStopOnFirstError = true;
		const bool kFailOnCustomSectionError = true;
		ReadBinaryOptions options(s_features, s_log_stream.get(), kReadDebugNames,
			kStopOnFirstError, kFailOnCustomSectionError);
		result = ReadBinaryInterp(env, file_data.data(), file_data.size(),
			&options, error_handler, out_module);
	}
}