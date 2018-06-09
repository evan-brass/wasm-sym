// Author: Evan Brass


// STL
#include <memory>
#include <queue>
#include <iostream>

// WABT includes
#include <interp.h>
#include <binary-reader-interp.h>
#include <binary-reader.h>
#include <cast.h>

// My Classes
#include "SymThread.h"
#include "SymEnvironment.h"

using std::cout;
using std::endl;
using std::shared_ptr;
using std::unique_ptr;
using std::queue;
using std::vector;

using namespace wabt;
using namespace wabt::interp;

//ExecResult RunFunction(Index func_index, const TypedValues& args) {
//	ExecResult exec_result;
//	Func* func = env_->GetFunc(func_index);
//	FuncSignature* sig = env_->GetFuncSignature(func->sig_index);
//
//	exec_result.result = PushArgs(sig, args);
//	if (exec_result.result == Result::Ok) {
//		exec_result.result =
//			func->is_host ? thread_.CallHost(cast<HostFunc>(func))
//			: RunDefinedFunction(cast<DefinedFunc>(func)->offset);
//		if (exec_result.result == Result::Ok) {
//			CopyResults(sig, &exec_result.values);
//		}
//	}
//}

int main(int argc, char** argv) {
	// Root for our path tree
	// This was to do a sort of copy on write method of threadEnv duplication, but I'll have to do that later.
//	shared_ptr<Path> pathRoot(new Path());

	// Queue to manage which paths we are advancing
	shared_ptr<queue<unique_ptr<SymThread>>> threads(new queue<unique_ptr<SymThread>>());
	SymThread::threadlist = threads;

	// File contents
	vector<uint8_t> fileContents;

	if (argc >= 2) {
		// Load the wasm file that we are testing
		wabt::Result r = wabt::ReadFile(argv[1], &fileContents);

		// We will perform symbolic execution on all exports from a module

		if (Succeeded(r)) {
			DefinedModule* module = nullptr;
			Environment tempEnv;
			{
				// Read in our module.
				ReadBinaryOptions options;
				r = ReadBinaryInterp(&tempEnv, fileContents.data(), fileContents.size(), &options, nullptr, &module);

			}
			// Unfortunately, I'm not supporting any table imports
			if (module->table_imports.size() > 0) {
				cout << "Sorry, I don't currently support wasm files that require imports." << endl;
			}

			// Turn our Environment into a SymEnvironment
			shared_ptr<SymEnvironment> environment(new SymEnvironment(tempEnv));

			// Create threads for all the module's public exports
			for (Export module_export : module->exports) {
				// If the export is a function...
				if (module_export.kind == ExternalKind::Func) {
					shared_ptr<SymEnvironment> threadEnv(new SymEnvironment(environment));
					// Then create a thread for the export
					unique_ptr<SymThread> thread(new SymThread(threadEnv));

					Func* function = threadEnv->GetFunc(module_export.index);
					FuncSignature* signature = threadEnv->GetFuncSignature(function->sig_index);
					// Push arguments onto the value stack for the inputs to the exported function
					// TODO: These arguments, the memory in the threadEnv, and maybe a few other things need to be replaced by symbolic values at some point
					TypedValues symbolicArgs;
					symbolicArgs.reserve(signature->param_types.size());
					for (Type T : signature->param_types) {
						thread->Push(Value()); // This needs to be symbolic
					}

					// Set the program counter to the location of the exported function
					DefinedFunc * definedFunction = cast<DefinedFunc>(function);
					thread->set_pc(definedFunction->offset);

					// Put the thread into the thread queue
					threads.push(std::move(thread));
				}
			}

			// Main execution loop
			while (!threads.empty()) {
				unique_ptr<SymThread> current = std::move(threads.front());
				threads.pop();

				interp::Result r = current->Run();
				if (r == interp::Result::Returned) {
					// Symbolic execution finished with no errors found.  Yay!
				}
				else if (r != interp::Result::Ok) {
					// Encountered a runtime error
					// TODO: Run the constraint solver and see if we can generate a test case.
				}
				else {
					threads.push(std::move(current));
				}
			}
		}
		else {
			cout << "Unable to read file" << endl;
		}
	}
	else {
		cout << "Usage format: wasm-sym <WASM File>" << endl;
	}
}