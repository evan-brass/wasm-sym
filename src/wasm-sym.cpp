// Author: Evan Brass


// STL
#include <memory>
#include <queue>
#include <iostream>
#include <string>

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
using std::string;

using namespace wabt;
using namespace wabt::interp;


// Queue to manage which paths we are advancing.  This is Global so that the SymThread's can reach it.
shared_ptr<queue<unique_ptr<SymThread>>> threads(new queue<unique_ptr<SymThread>>());

// Gloabl problem manager:
naxos::NsProblemManager pm;

shared_ptr<SymEnvironment> almostHighestParent(shared_ptr<SymEnvironment> i) {
	if (i == nullptr || i->parent == nullptr) {
		cout << "Already too high" << endl;
		exit(1);
	}
	else if (i->parent->parent == nullptr) {
		return i;
	}
	else {
		return almostHighestParent(i->parent);
	}
}

int main(int argc, char** argv) {

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

			// Manipulatable inputs:
			struct FunctionInputs {
				string funcName;
				vector<shared_ptr<naxos::NsIntVar>> vars;
				shared_ptr<SymEnvironment> env;
			};
			vector<FunctionInputs> tests;

			// Create threads for all the module's public exports
			for (Export module_export : module->exports) {
				// If the export is a function...
				if (module_export.kind == ExternalKind::Func) {
					shared_ptr<SymEnvironment> threadEnv(new SymEnvironment(environment));
					// Then create a thread for the export
					unique_ptr<SymThread> thread(new SymThread(threadEnv));

					// Store the function name and the original inputs that we supplied
					FunctionInputs testable;
					testable.env = threadEnv;

					Func* function = threadEnv->GetFunc(module_export.index);
					FuncSignature* signature = threadEnv->GetFuncSignature(function->sig_index);
					// Push arguments onto the value stack for the inputs to the exported function
					// TODO: These arguments, the memory in the threadEnv, and maybe a few other things need to be replaced by symbolic values at some point
					TypedValues symbolicArgs;
					symbolicArgs.reserve(signature->param_types.size());
					for (Type T : signature->param_types) {
						int64_t min;
						int64_t max;
						switch (T) {
						case Type::I32:
							min = IntMin;
							max = IntMax;
							break;
						default:
							cout << "Sorry, Only I32 types are supported at this time" << endl;
							exit(1);
						}
						::Value ret(new naxos::NsIntVar(pm, IntMin, IntMax));

						// Remember the original values that we test
						testable.vars.push_back(ret);

						thread->Push(ret);
					}

					// Set the program counter to the location of the exported function
					DefinedFunc * definedFunction = cast<DefinedFunc>(function);
					thread->set_pc(definedFunction->offset);

					// Remember the name of the functions that we test and where it is (We use offset to determine which function we've found a bug in.)
					testable.funcName = module_export.name;

					tests.push_back(testable);

					// Put the thread into the thread queue
					threads->push(std::move(thread));
				}
			}

			// Main execution loop
			while (!threads->empty()) {
				unique_ptr<SymThread> current = std::move(threads->front());
				threads->pop();

				interp::Result r = current->Run();
				if (r == interp::Result::Returned) {
					// Symbolic execution finished with no errors found.  Yay!
				}
				else if (r != interp::Result::Ok) {
					// Encountered a runtime error
					shared_ptr<SymEnvironment> iterator(current->environment);
					while (iterator != nullptr) {
						for (auto &var : iterator->vars) {
							const naxos::NsIntVar * temp = &(*var);
							cout << "Adding Variable to solver: " << *temp << " or " << *var << endl;
							pm.addVar(temp);
						}
						for (auto &constraint : iterator->constraints) {
							pm.add(*constraint);
						}
						iterator = iterator->parent;
					}

					FunctionInputs * buggedFunction = nullptr;
					for (auto &exported : tests) {
						if (exported.env == almostHighestParent(current->environment)) {
							buggedFunction = &exported;
							break;
						}
					}
					if (buggedFunction != nullptr) {
						naxos::NsIntVarArray toSolve;
						for (auto var : buggedFunction->vars) {
							toSolve.push_back(*var);
						}

						cout << "A crash may have been found.  Here's a dump:" << endl;
						current->dump();

						// This solves the ranges into a single set of inputs.  You would still need to debug the program to discover why it is crashing.
						pm.addGoal(new naxos::NsgLabeling(toSolve));

						cout << "Attempting to solve for the crash:" << endl;
						// Ask the solver to find a solution
						if (pm.nextSolution()) {
							cout << "The crash was found!" << endl;
							cout << "The bug can be reproduced by calling: " << buggedFunction->funcName << endl;
							cout << "With these parameters: " << endl;
							for (auto & param : buggedFunction->vars) {
								cout << "\t" << *param << endl;
							}
						}
						else {
							cout << "The crash turned out to be impossible." << endl;
						}
					}
					else {
						cout << "Something unexpected occured." << endl;
					}
				}
				else {
					threads->push(std::move(current));
				}
			}
		}
	}
	else {
		cout << "Usage format: wasm-sym <WASM File>" << endl;
	}

	cout << "Press any key and enter to close...";
	char x;
	std::cin >> x;
}