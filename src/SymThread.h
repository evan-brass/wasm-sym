#pragma once

#include <memory>
#include <queue>

#include <stream.h>

#include <naxos.h>

#include "SymEnvironment.h"

#include <interp.h>

const unsigned UintMax = 4294967295;
const unsigned UintMin = 0;
const unsigned IntMax =  2147483520;
const unsigned IntMin = -2147483648;

typedef std::shared_ptr<naxos::Ns_Expression> Value;

class SymThread {
public:
	struct Options {
		static const uint32_t kDefaultValueStackSize = 512 * 1024 / sizeof(Value);
		static const uint32_t kDefaultCallStackSize = 64 * 1024;

		explicit Options(uint32_t value_stack_size = kDefaultValueStackSize,
			uint32_t call_stack_size = kDefaultCallStackSize);

		uint32_t value_stack_size;
		uint32_t call_stack_size;
	};


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

	// Constructor
	explicit SymThread(const std::shared_ptr<SymEnvironment> & e) : environment(e) {};

	std::shared_ptr<SymEnvironment> environment;

	naxos::NsProblemManager pm;

	void set_pc(wabt::interp::IstreamOffset offset) { pc_ = offset; }
	wabt::interp::IstreamOffset pc() const { return pc_; }

	wabt::Index NumValues() const { return value_stack_top_; }
	wabt::interp::Result Push(Value) WABT_WARN_UNUSED;
	Value Pop();
	Value ValueAt(wabt::Index at) const;

	wabt::interp::Result Run(int num_instructions = 1);

	// Store a pointer to the threadlist so that threads can create more threads
	static std::shared_ptr<std::queue<std::unique_ptr<SymThread>>> threadlist;

protected:
	Value ToRep(bool);
	Value ToRep(uint32_t);
	//Value ToRep(uint64_t);
	Value ToRep(int32_t);
	//Value ToRep(int64_t);
	//Value ToRep(float);
	//Value ToRep(double);

	const uint8_t* GetIstream() const { return environment->binary_->data(); }

	wabt::interp::Memory* ReadMemory(const uint8_t** pc);
	template <typename MemType>
	Result GetAccessAddress(const uint8_t** pc, void** out_address);
	template <typename MemType>
	Result GetAtomicAccessAddress(const uint8_t** pc, void** out_address);

	Value Top();
	Value Pick(wabt::Index depth);

	// Push/Pop values with conversions, e.g. Push<float> will convert to the
	// ValueTypeRep (uint32_t) and push that. Similarly, Pop<float> will pop the
	// value and convert to float.
	template <typename T>
	Result Push(T) WABT_WARN_UNUSED;
	template <typename T>
	T Pop();

	// Push/Pop values without conversions, e.g. Push<float> will take a uint32_t
	// argument which is the integer representation of that float value.
	// Similarly, PopRep<float> will not convert the value to a float.
	template <typename T>
	Result PushRep(ValueTypeRep<T>) WABT_WARN_UNUSED;
	template <typename T>
	ValueTypeRep<T> PopRep();

	void DropKeep(uint32_t drop_count, uint8_t keep_count);

	wabt::interp::Result PushCall(const uint8_t* pc) WABT_WARN_UNUSED;
	wabt::interp::IstreamOffset PopCall();

	template <typename R, typename T> using UnopFunc = R(T);
	template <typename R, typename T> using UnopTrapFunc = Result(T, R*);
	template <typename R, typename T> using BinopFunc = R(T, T);
	template <typename R, typename T> using BinopTrapFunc = Result(T, T, R*);

	template <typename MemType, typename ResultType = MemType>
	Result Load(const uint8_t** pc) WABT_WARN_UNUSED;
	template <typename MemType, typename ResultType = MemType>
	Result Store(const uint8_t** pc) WABT_WARN_UNUSED;
	template <typename MemType, typename ResultType = MemType>
	Result AtomicLoad(const uint8_t** pc) WABT_WARN_UNUSED;
	template <typename MemType, typename ResultType = MemType>
	Result AtomicStore(const uint8_t** pc) WABT_WARN_UNUSED;
	template <typename MemType, typename ResultType = MemType>
	Result AtomicRmw(BinopFunc<ResultType, ResultType>,
		const uint8_t** pc) WABT_WARN_UNUSED;
	template <typename MemType, typename ResultType = MemType>
	Result AtomicRmwCmpxchg(const uint8_t** pc) WABT_WARN_UNUSED;

	template <typename R, typename T = R>
	Result Unop(UnopFunc<R, T> func) WABT_WARN_UNUSED;
	template <typename R, typename T = R>
	Result UnopTrap(UnopTrapFunc<R, T> func) WABT_WARN_UNUSED;

	template <typename R, typename T = R>
	Result Binop(BinopFunc<R, T> func) WABT_WARN_UNUSED;
	template <typename R, typename T = R>
	Result BinopTrap(BinopTrapFunc<R, T> func) WABT_WARN_UNUSED;

	std::shared_ptr<SymEnvironment> environment;
	std::vector<std::shared_ptr<naxos::Ns_Expression>> value_stack_;
	std::vector<wabt::interp::IstreamOffset> call_stack_;
	uint32_t value_stack_top_ = 0;
	uint32_t call_stack_top_ = 0;
	wabt::interp::IstreamOffset pc_ = 0;
};

