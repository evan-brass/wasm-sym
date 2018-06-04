// Author: Evan Brass


// STL
#include <memory>
#include <queue>
#include <iostream>

// WABT includes
#include <interp.h>
#include <binary-reader-interp.h>
#include <binary-reader.h>

// My Classes
#include "Path.h"
#include "SymThread.h"

using std::cout;
using std::endl;
using std::shared_ptr;
using std::unique_ptr;
using std::queue;
using std::vector;

using namespace wabt;
using namespace wabt::interp;

int main(int argc, char** argv) {
	// Root for our path tree
	shared_ptr<Path> pathRoot(new Path());

	// Queue to manage which paths we are advancing
	queue<unique_ptr<SymThread>> threads;

	// File contents
	vector<uint8_t> fileContents;

	if (argc >= 2) {
		// Load the wasm file that we are testing
		wabt::Result r = wabt::ReadFile(argv[1], &fileContents);

		// We will perform symbolic execution on all exports from a module

		if (Succeeded(r)) {
			DefinedModule* module = nullptr;
			{
				// Read in our module.
				ReadBinaryOptions options;
				r = ReadBinaryInterp(&(pathRoot->env), fileContents.data(), fileContents.size(), &options, nullptr, &module);

			}
			// Unfortunately, I'm not supporting any table imports
			if (module->table_imports.size() > 0) {
				cout << "Sorry, I don't currently support wasm files that require imports." << endl;
			}

			// Create threads for all the module's public exports
			for (Export module_export : module->exports) {
				// If the export is a function...
				if (module_export.kind == ExternalKind::Func) {
					// Then create a thread for the export
					threads.push(unique_ptr<SymThread>(new SymThread(&(pathRoot->env))));

					// Push arguments onto the value stack for the inputs to the exported function
					// TODO: These arguments, the memory in the environment, and maybe a few other things need to be replaced by symbolic values at some point

				}
			}

		}
		else {
			cout << "Unable to read file" << endl;
		}
	}
	else {
		cout << "Usage format: <WASM File> entrypoints" << endl;
	}
}