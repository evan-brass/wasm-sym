#pragma once

#include <memory>
#include <queue>

#include <interp.h>
#include <naxos.h>

#include "SymEnvironment.h"

#include <stream.h>
const unsigned UintMax = 4294967295;
const unsigned UintMin = 0;
const unsigned IntMax =  2147483520;
const unsigned IntMin = -2147483648;

class SymThread : public wabt::interp::Thread
{
typedef std::shared_ptr<naxos::Ns_Expression> Value;
public:
//	wabt::interp::Result Run(int num_instructions);
	Value Pick(wabt::Index depth) {
		auto & val = value_stack_[value_stack_top_ - depth];
		if (val == nullptr) {
			exit(1); // What happened????
					 //val = std::make_shared<naxos::Ns_Expression>(new naxos::NsIntVar(pm, 0, NsUintVarMax));
					 //environment->constraints.emplace_back(val);
		}
		return val;
	};

	// Convert into our representation (SharedNaxos NsIntVar's)
	Value ToRep(bool x) {
		auto ret = (new naxos::NsIntVar(pm, 0, 1));
		ret->set(x);

		environment->constraints.emplace_back(ret);

		return Value(environment->constraints.back());
	}
	Value ToRep(uint32_t x) {
		auto ret = new naxos::NsIntVar(pm, UintMin, UintMax);
		ret->set(x);

		environment->constraints.emplace_back(ret);

		return Value(environment->constraints.back());
	}
	Value ToRep(int32_t x) {
		auto ret = new naxos::NsIntVar(pm, IntMin, IntMax);
		ret->set(x);

		environment->constraints.emplace_back(ret);

		return Value(environment->constraints.back());
	}
	Value ToRep(Value x) { 
		return x; // This should work because of call by value.
	}
	naxos::NsProblemManager pm;
	Value Pop();
	Value ValueAt(wabt::Index at) const;
	// Store a pointer to the threadlist so that threads can create more threads
	static std::shared_ptr<std::queue<std::unique_ptr<SymThread>>> threadlist;

	const uint8_t* GetIstream() const override { return environment->binary_->data(); }
protected:
	Value ToRep(bool);
	Value ToRep(uint32_t);
	//Value ToRep(uint64_t);
	Value ToRep(int32_t);
	//Value ToRep(int64_t);
	//Value ToRep(float);
	//Value ToRep(double);

	Value Top();
	Value Pick(wabt::Index depth);
	ValueTypeRep<T> PopRep();

	explicit SymThread(const std::shared_ptr<SymEnvironment> & e) : wabt::interp::Thread(nullptr), environment(e) {};
	std::shared_ptr<SymEnvironment> environment;
	std::vector<std::shared_ptr<naxos::Ns_Expression>> value_stack_;
	uint32_t value_stack_top_ = 0;
};

