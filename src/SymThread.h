#pragma once

#include <memory>
#include <queue>
#include <assert.h>

#include <stream.h>

#include <naxos.h>

#include "SymEnvironment.h"

#include <interp.h>

extern naxos::NsProblemManager pm;

const long UintMax = 1047483519; // This is a limitation of Naxos
const long UintMin = 0;
const long IntMax =  1047483519;
const long IntMin = -1047483647;

typedef std::shared_ptr<naxos::NsIntVar> Value;
typedef uint32_t CodePointer;

#define TRAP(type) return wabt::interp::Result::Trap##type
#define TRAP_UNLESS(cond, type) TRAP_IF(!(cond), type)
#define TRAP_IF(cond, type)    \
  do {                         \
    if (WABT_UNLIKELY(cond)) { \
      TRAP(type);              \
    }                          \
  } while (0)

#define CHECK_STACK() \
  TRAP_IF(value_stack_top_ >= value_stack_.size(), ValueStackExhausted)

#define PUSH_NEG_1_AND_BREAK_IF(cond) \
  if (WABT_UNLIKELY(cond)) {          \
    CHECK_TRAP(Push<int32_t>(-1));    \
    break;                            \
  }

class SymThread {
public:

	// Constructor
	explicit SymThread(const std::shared_ptr<SymEnvironment> & e) : environment(e) {
		static const uint32_t valueStackSize = 512 * 1024 / sizeof(::Value);
		static const uint32_t callStackSize = 64 * 1024;
		value_stack_.resize(valueStackSize);
		call_stack_.resize(callStackSize);
	};
	SymThread(const SymThread & c) : environment(c.environment), value_stack_(c.value_stack_), call_stack_(c.call_stack_), value_stack_top_(c.value_stack_top_), call_stack_top_(c.call_stack_top_), pc_(c.pc_) {

	}

	// Members
	std::shared_ptr<SymEnvironment> environment;
	std::vector<std::shared_ptr<naxos::NsIntVar>> value_stack_;
	std::vector<CodePointer> call_stack_;
	uint32_t value_stack_top_ = 0;
	uint32_t call_stack_top_ = 0;
	CodePointer pc_ = 0;

	// Functions
	void set_pc(CodePointer offset) { pc_ = offset; }
	CodePointer pc() const { return pc_; }

	wabt::Index NumValues() const { return value_stack_top_; }

	::Value Pop() {
		return value_stack_[--value_stack_top_];
	}
	::Value ValueAt(wabt::Index at) const {
		assert(at < value_stack_top_);
		return value_stack_[at];
	}

	template<typename T>
	::Value ToRepT(T x) {
		::Value ret(new naxos::NsIntVar(pm, IntMin, IntMax));
		ret->set(x);

		environment->vars.push_back(ret);

		return ret;
	}
	::Value ToRep(uint32_t value) {
		return ToRepT<uint32_t>(value);
	}
	::Value ToRep(int32_t value) {
		return ToRepT<int32_t>(value);
	}
	::Value ToRep(bool value) {
		return ToRepT<bool>(value);
	}
	::Value ToRep(::Value value) {
		::Value ret(new naxos::NsIntVar(pm, IntMin, IntMax));
		environment->constraints.emplace_back(new naxos::Ns_ExprConstrYeqZ(*ret, *value, true));
		environment->vars.push_back(ret);
		return ret;
	}
	//::Value ToRep(uint64_t);
	//::Value ToRep(int64_t);
	//::Value ToRep(float);
	//::Value ToRep(double);

	wabt::interp::Result PushRep(::Value value) {
		CHECK_STACK();
		value_stack_[value_stack_top_++] = value;
		return wabt::interp::Result::Ok;
	}
	template<typename T> wabt::interp::Result PushT(T value) {
		return PushRep(ToRep(value));
	}
	wabt::interp::Result Push(::Value value) {
			return PushRep(value);
	}
	wabt::interp::Result Push(uint32_t value) {
		return PushT<uint32_t>(value);
	}

	wabt::interp::Result Run(int num_instructions = 1);

	void dump();

protected:

	const uint8_t* GetIstream() const { return environment->binary_->data(); }

	::Value Top() {
		return Pick(1);
	};
	::Value Pick(wabt::Index depth) {
		auto & val = value_stack_[value_stack_top_ - depth];
		return val;
	};

	wabt::interp::Result Add();
	wabt::interp::Result Sub();
	wabt::interp::Result Mul();
	wabt::interp::Result Div();

	wabt::interp::Result Eq();
	wabt::interp::Result Ne();
	wabt::interp::Result Lt();
	wabt::interp::Result Le();
	wabt::interp::Result Gt();
	wabt::interp::Result Ge();

	void DropKeep(uint32_t drop_count, uint8_t keep_count) {
		assert(keep_count <= 1);
		if (keep_count == 1) {
			Pick(drop_count + 1) = Top();
		}
		value_stack_top_ -= drop_count;
	}

	wabt::interp::Result PushCall(const uint8_t* pc) {
		TRAP_IF(call_stack_top_ >= call_stack_.size(), CallStackExhausted);
		call_stack_[call_stack_top_++] = pc - GetIstream();
		return wabt::interp::Result::Ok;
	}
	CodePointer PopCall() {
		return call_stack_[--call_stack_top_];
	}

};


extern std::shared_ptr<std::queue<std::unique_ptr<SymThread>>> threadlist;

