#include "SymEnvironment.h"

#include <stream.h>

#include <naxos.h>

using std::shared_ptr;
using std::vector;
using std::unique_ptr;

using namespace wabt;
using namespace wabt::interp;

SymEnvironment::SymEnvironment(const shared_ptr<SymEnvironment> p) : parent(p) {
	if (parent == nullptr) {
		// These are shared pointers
		modules_ = shared_ptr<vector<unique_ptr<Module>>>(new vector<unique_ptr<Module>>());
		sigs_ = shared_ptr<vector<FuncSignature>>(new vector<FuncSignature>());
		funcs_ = shared_ptr<vector<unique_ptr<Func>>>(new vector<unique_ptr<Func>>());
		binary_ = shared_ptr<vector<uint8_t>>(new vector<uint8_t>());
	}
	else {
		// These are shared pointers
		modules_ = parent->modules_;
		sigs_ = parent->sigs_;
		funcs_ = parent->funcs_;
		binary_ = parent->binary_;

		// These are self owned (soon to be) symbolic constraint containers.
		// Use copy constructors
		/*memories_ = vector<Memory>(parent->memories_);
		tables_ = vector<Table>(parent->tables_);
		globals_ = vector<Global>(parent->globals_);*/
	}
}

SymEnvironment::SymEnvironment(wabt::interp::Environment & e) : SymEnvironment() {
	// Move the contents of the opbjects into the locations we allocated for them.
	*modules_ = std::move(e.modules_);
	*sigs_ = std::move(e.sigs_);
	*funcs_ = std::move(e.funcs_);
	*binary_ = std::move(e.istream_->data);

	// Move the rest of the objects (These aren't shared pointers so nothing special)
	/*memories_ = std::move(e.memories_);
	tables_ = std::move(e.tables_);
	globals_ = std::move(e.globals_);*/
}