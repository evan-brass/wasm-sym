#include "SymThread.h"

std::shared_ptr<naxos::Ns_Expression> SymThread::ToRep(bool x) { 
	return std::make_shared<naxos::Ns_Expression>(new naxos::NsIntVar(pm, 0, 1)); 
}

Thread::Options::Options(uint32_t value_stack_size, uint32_t call_stack_size)
	: value_stack_size(value_stack_size), call_stack_size(call_stack_size) {}

Thread::Thread(wabt::interp::Environment* env, const Options& options)
	: env_(env),
	value_stack_(options.value_stack_size),
	call_stack_(options.call_stack_size) {}