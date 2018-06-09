#pragma once

#include <memory>
#include <vector>
#include <list>

#include <interp.h>

#include "SymValue.h"

class SymEnvironment
{
public:
	// This is the basis for the higherarchy
	std::shared_ptr<SymEnvironment> parent;

	SymEnvironment(const std::shared_ptr<SymEnvironment> p);
	SymEnvironment() : SymEnvironment(nullptr) {}
	SymEnvironment(wabt::interp::Environment & e);

	wabt::interp::Module* GetModule(wabt::Index index) {
		assert(index < modules_->size());
		return (*modules_)[index].get();
	}
	wabt::interp::FuncSignature* GetFuncSignature(wabt::Index index) {
		assert(index < sigs_->size());
		return &(*sigs_)[index];
	}
	wabt::interp::Func* GetFunc(wabt::Index index) {
		assert(index < funcs_->size());
		return (*funcs_)[index].get();
	}
	wabt::interp::Global* GetGlobal(wabt::Index index) {
		assert(index < globals_.size());
		return &globals_[index];
	}
	wabt::interp::Memory* GetMemory(wabt::Index index) {
		assert(index < memories_.size());
		return &memories_[index];
	}
	wabt::interp::Table* GetTable(wabt::Index index) {
		assert(index < tables_.size());
		return &tables_[index];
	}

	// List of constraints that exist on this execution path (full constraints must be pulled from the environment chain).
	std::list<std::shared_ptr<naxos::Ns_Expression>> constraints;

protected:
	//friend class wabt::interp::Thread;
	friend class SymThread;

	// Share these three between all environments
	std::shared_ptr<std::vector<std::unique_ptr<wabt::interp::Module>>> modules_;
	std::shared_ptr<std::vector<wabt::interp::FuncSignature>> sigs_;
	std::shared_ptr<std::vector<std::unique_ptr<wabt::interp::Func>>> funcs_;
	std::shared_ptr<std::vector<uint8_t>> binary_;

	// TODO: These need to be symbolicified
	std::vector<wabt::interp::Memory> memories_;
	std::vector<wabt::interp::Table> tables_;
	std::vector<wabt::interp::Global> globals_;

};

