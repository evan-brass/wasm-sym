#include "SymThread.h"

#include <iostream>

using namespace wabt;
using namespace wabt::interp;

using std::shared_ptr;
using std::unique_ptr;
using std::cout;
using std::endl;
using naxos::NsIntVar;
using naxos::Ns_ExprConstrYeqC;
using naxos::Ns_ExprConstrYeqZ;

void SymThread::dump() {
	cout << "Value Stack:" << endl;
	for (unsigned i = 0; i < value_stack_top_; ++i) {
		auto v = value_stack_[i];
		cout << "\t" << *v << ": pm*=" << v->pm <<  endl;
	}
}

wabt::interp::Result SymThread::Add() {
	::Value rh = Pop();
	::Value lh = Pop();
	::Value ret(new NsIntVar());
	*ret = *lh + *rh;
	environment->vars.push_back(ret);
	Push(ret);
	return wabt::interp::Result::Ok;
}
wabt::interp::Result SymThread::Sub() {
	::Value rh = Pop();
	::Value lh = Pop();
	::Value ret(new NsIntVar());
	*ret = *lh - *rh;
	environment->vars.push_back(ret);
	Push(ret);
	return wabt::interp::Result::Ok;
}
wabt::interp::Result SymThread::Mul() {
	::Value rh = Pop();
	::Value lh = Pop();
	::Value ret(new NsIntVar());
	*ret = *lh * *rh;
	environment->vars.push_back(ret);
	Push(ret);
	return wabt::interp::Result::Ok;
}
wabt::interp::Result SymThread::Div() {
	::Value rh = Pop();
	::Value lh = Pop();
	::Value ret(new NsIntVar());
	*ret = *lh / *rh;
	environment->vars.push_back(ret);
	Push(ret);
	return wabt::interp::Result::Ok;
}


wabt::interp::Result SymThread::Eq() {
	::Value rh = Pop();
	::Value lh = Pop();
	::Value ret(new NsIntVar());
	*ret = *lh == *rh;
	environment->vars.push_back(ret);
	Push(ret);
	return wabt::interp::Result::Ok;
}
wabt::interp::Result SymThread::Lt() {
	::Value rh = Pop();
	::Value lh = Pop();
	::Value ret(new NsIntVar());
	*ret = *lh < *rh;
	environment->vars.push_back(ret);
	Push(ret);
	return wabt::interp::Result::Ok;
}
wabt::interp::Result SymThread::Gt() {
	::Value rh = Pop();
	::Value lh = Pop();
	::Value ret(new NsIntVar());
	*ret = *lh > *rh;
	environment->vars.push_back(ret);
	Push(ret);
	return wabt::interp::Result::Ok;
}
wabt::interp::Result SymThread::Le() {
	::Value rh = Pop();
	::Value lh = Pop();
	::Value ret(new NsIntVar());
	*ret = *lh <= *rh;
	environment->vars.push_back(ret);
	Push(ret);
	return wabt::interp::Result::Ok;
}
wabt::interp::Result SymThread::Ge() {
	::Value rh = Pop();
	::Value lh = Pop();
	::Value ret(new NsIntVar());
	*ret = *lh >= *rh;
	environment->vars.push_back(ret);
	Push(ret);
	return wabt::interp::Result::Ok;
}
wabt::interp::Result SymThread::Ne() {
	::Value rh = Pop();
	::Value lh = Pop();
	::Value ret(new NsIntVar());
	*ret = *lh != *rh;
	environment->vars.push_back(ret);
	Push(ret);
	return wabt::interp::Result::Ok;
}

template <typename T>
inline T ReadUxAt(const uint8_t* pc) {
	T result;
	memcpy(&result, pc, sizeof(T));
	return result;
}

template <typename T>
inline T ReadUx(const uint8_t** pc) {
	T result = ReadUxAt<T>(*pc);
	*pc += sizeof(T);
	return result;
}

inline uint8_t ReadU8At(const uint8_t* pc) {
	return ReadUxAt<uint8_t>(pc);
}

inline uint8_t ReadU8(const uint8_t** pc) {
	return ReadUx<uint8_t>(pc);
}

inline uint32_t ReadU32At(const uint8_t* pc) {
	return ReadUxAt<uint32_t>(pc);
}

inline uint32_t ReadU32(const uint8_t** pc) {
	return ReadUx<uint32_t>(pc);
}

inline uint64_t ReadU64At(const uint8_t* pc) {
	return ReadUxAt<uint64_t>(pc);
}

inline uint64_t ReadU64(const uint8_t** pc) {
	return ReadUx<uint64_t>(pc);
}

inline Opcode ReadOpcode(const uint8_t** pc) {
	uint8_t value = ReadU8(pc);
	if (Opcode::IsPrefixByte(value)) {
		// For now, assume all instructions are encoded with just one extra byte
		// so we don't have to decode LEB128 here.
		uint32_t code = ReadU8(pc);
		return Opcode::FromCode(value, code);
	}
	else {
		// TODO(binji): Optimize if needed; Opcode::FromCode does a log2(n) lookup
		// from the encoding.
		return Opcode::FromCode(value);
	}
}

#undef TRAP
#undef TRAP_UNLESS
#undef TRAP_IF
#undef CHECK_STACK
#undef PUSH_NEG_1_AND_BREAK_IF
#undef GOTO
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

#define GOTO(offset) pc = &istream[offset]

// Differs from the normal CHECK_RESULT because this one is meant to return the
// interp Result type.
#undef CHECK_RESULT
#define CHECK_RESULT(expr)   \
  do {                       \
    if (WABT_FAILED(expr)) { \
      return Result::Error;  \
    }                        \
  } while (0)

// Differs from CHECK_RESULT since it can return different traps, not just
// Error. Also uses __VA_ARGS__ so templates can be passed without surrounding
// parentheses.
#define CHECK_TRAP(...)            \
  do {                             \
    wabt::interp::Result result = (__VA_ARGS__); \
    if (result != wabt::interp::Result::Ok) {    \
      return result;               \
    }                              \
  } while (0)

wabt::interp::Result SymThread::Run(int num_instructions) {
	dump();

	wabt::interp::Result result = wabt::interp::Result::Ok;

	const uint8_t* istream = GetIstream();
	const uint8_t* pc = &istream[pc_];
	for (int i = 0; i < num_instructions; ++i) {
		Opcode opcode = ReadOpcode(&pc);
		assert(!opcode.IsInvalid());
		switch (opcode) {
		/*case Opcode::Select: {
			uint32_t cond = Pop<uint32_t>();
			::Value false_ = Pop();
			::Value true_ = Pop();
			CHECK_TRAP(Push(cond ? true_ : false_));
			break;
		}*/

		case Opcode::Br:
			GOTO(ReadU32(&pc));
			break;

		case Opcode::BrIf: {
			CodePointer new_pc = ReadU32(&pc);
			::Value x = Pop();
			// Create two new environments for the split.
			shared_ptr<SymEnvironment> e1(new SymEnvironment(environment));
			e1->constraints.emplace_back(new Ns_ExprConstrYeqC(*x, 0, true));
			environment = e1;

			shared_ptr<SymEnvironment> e2(new SymEnvironment(environment)); // This will be for our new thread
			unique_ptr<SymThread> newThread(new SymThread(*this));
			e2->constraints.emplace_back(new Ns_ExprConstrYeqC(*x, 0, false));
			newThread->environment = e2;
			newThread->pc_ = new_pc;
			break;
		}

		/*case Opcode::BrTable: {
			Index num_targets = ReadU32(&pc);
			CodePointer table_offset = ReadU32(&pc);
			uint32_t key = Pop<uint32_t>();
			CodePointer key_offset =
				(key >= num_targets ? num_targets : key) * WABT_TABLE_ENTRY_SIZE;
			const uint8_t* entry = istream + table_offset + key_offset;
			CodePointer new_pc;
			uint32_t drop_count;
			uint8_t keep_count;
			read_table_entry_at(entry, &new_pc, &drop_count, &keep_count);
			DropKeep(drop_count, keep_count);
			GOTO(new_pc);
			break;
		}*/

		case Opcode::Return:
			if (call_stack_top_ == 0) {
				result = wabt::interp::Result::Returned;
				goto exit_loop;
			}
			GOTO(PopCall());
			break;

		case Opcode::Unreachable:
			TRAP(Unreachable);
			break;

		case Opcode::I32Const:
			CHECK_TRAP(Push(ToRep(ReadU32(&pc))));
			break;

		/*case Opcode::I64Const:
			CHECK_TRAP(Push(ReadU64(&pc)));
			break;*/

		/*case Opcode::F32Const:
			CHECK_TRAP(PushRep<float>(ReadU32(&pc)));
			break;*/

		/*case Opcode::F64Const:
			CHECK_TRAP(PushRep<double>(ReadU64(&pc)));
			break;*/

		/*case Opcode::GetGlobal: {
			Index index = ReadU32(&pc);
			assert(index < environment->globals_.size());
			CHECK_TRAP(Push(environment->globals_[index].typed_value.value));
			break;
		}

		case Opcode::SetGlobal: {
			Index index = ReadU32(&pc);
			assert(index < environment->globals_.size());
			environment->globals_[index].typed_value.value = Pop();
			break;
		}*/

		case Opcode::GetLocal: {
			CHECK_TRAP(PushRep(Pick(ReadU32(&pc))));
			break;
		}

		case Opcode::SetLocal: {
			::Value value = Pop();
			Pick(ReadU32(&pc)) = value;
			break;
		}

		case Opcode::TeeLocal:
			Pick(ReadU32(&pc)) = Top();
			break;

		case Opcode::Call: {
			CodePointer offset = ReadU32(&pc);
			CHECK_TRAP(PushCall(pc));
			GOTO(offset);
			break;
		}

		/*case Opcode::CallIndirect: {
			Index table_index = ReadU32(&pc);
			Table* table = &environment->tables_[table_index];
			Index sig_index = ReadU32(&pc);
			Index entry_index = Pop<uint32_t>();
			TRAP_IF(entry_index >= table->func_indexes.size(), UndefinedTableIndex);
			Index func_index = table->func_indexes[entry_index];
			TRAP_IF(func_index == kInvalidIndex, UninitializedTableElement);
			Func* func = environment->funcs_[func_index].get();
			TRAP_UNLESS(environment->FuncSignaturesAreEqual(func->sig_index, sig_index),
				IndirectCallSignatureMismatch);
			if (func->is_host) {
				CallHost(cast<HostFunc>(func));
			}
			else {
				CHECK_TRAP(PushCall(pc));
				GOTO(cast<DefinedFunc>(func)->offset);
			}
			break;
		}*/

		/*case Opcode::InterpCallHost: {
			Index func_index = ReadU32(&pc);
			CallHost(cast<HostFunc>(environment->funcs_[func_index].get()));
			break;
		}*/

		/*case Opcode::I32Load8S:
			CHECK_TRAP(Load<int8_t, uint32_t>(&pc));
			break;

		case Opcode::I32Load8U:
			CHECK_TRAP(Load<uint8_t, uint32_t>(&pc));
			break;

		case Opcode::I32Load16S:
			CHECK_TRAP(Load<int16_t, uint32_t>(&pc));
			break;

		case Opcode::I32Load16U:
			CHECK_TRAP(Load<uint16_t, uint32_t>(&pc));
			break;

		case Opcode::I64Load8S:
			CHECK_TRAP(Load<int8_t, uint64_t>(&pc));
			break;

		case Opcode::I64Load8U:
			CHECK_TRAP(Load<uint8_t, uint64_t>(&pc));
			break;

		case Opcode::I64Load16S:
			CHECK_TRAP(Load<int16_t, uint64_t>(&pc));
			break;

		case Opcode::I64Load16U:
			CHECK_TRAP(Load<uint16_t, uint64_t>(&pc));
			break;

		case Opcode::I64Load32S:
			CHECK_TRAP(Load<int32_t, uint64_t>(&pc));
			break;

		case Opcode::I64Load32U:
			CHECK_TRAP(Load<uint32_t, uint64_t>(&pc));
			break;

		case Opcode::I32Load:
			CHECK_TRAP(Load<uint32_t>(&pc));
			break;

		case Opcode::I64Load:
			CHECK_TRAP(Load<uint64_t>(&pc));
			break;

		case Opcode::F32Load:
			CHECK_TRAP(Load<float>(&pc));
			break;

		case Opcode::F64Load:
			CHECK_TRAP(Load<double>(&pc));
			break;

		case Opcode::I32Store8:
			CHECK_TRAP(Store<uint8_t, uint32_t>(&pc));
			break;

		case Opcode::I32Store16:
			CHECK_TRAP(Store<uint16_t, uint32_t>(&pc));
			break;

		case Opcode::I64Store8:
			CHECK_TRAP(Store<uint8_t, uint64_t>(&pc));
			break;

		case Opcode::I64Store16:
			CHECK_TRAP(Store<uint16_t, uint64_t>(&pc));
			break;

		case Opcode::I64Store32:
			CHECK_TRAP(Store<uint32_t, uint64_t>(&pc));
			break;

		case Opcode::I32Store:
			CHECK_TRAP(Store<uint32_t>(&pc));
			break;

		case Opcode::I64Store:
			CHECK_TRAP(Store<uint64_t>(&pc));
			break;

		case Opcode::F32Store:
			CHECK_TRAP(Store<float>(&pc));
			break;

		case Opcode::F64Store:
			CHECK_TRAP(Store<double>(&pc));
			break;

		case Opcode::I32AtomicLoad8U:
			CHECK_TRAP(AtomicLoad<uint8_t, uint32_t>(&pc));
			break;

		case Opcode::I32AtomicLoad16U:
			CHECK_TRAP(AtomicLoad<uint16_t, uint32_t>(&pc));
			break;

		case Opcode::I64AtomicLoad8U:
			CHECK_TRAP(AtomicLoad<uint8_t, uint64_t>(&pc));
			break;

		case Opcode::I64AtomicLoad16U:
			CHECK_TRAP(AtomicLoad<uint16_t, uint64_t>(&pc));
			break;

		case Opcode::I64AtomicLoad32U:
			CHECK_TRAP(AtomicLoad<uint32_t, uint64_t>(&pc));
			break;

		case Opcode::I32AtomicLoad:
			CHECK_TRAP(AtomicLoad<uint32_t>(&pc));
			break;

		case Opcode::I64AtomicLoad:
			CHECK_TRAP(AtomicLoad<uint64_t>(&pc));
			break;

		case Opcode::I32AtomicStore8:
			CHECK_TRAP(AtomicStore<uint8_t, uint32_t>(&pc));
			break;

		case Opcode::I32AtomicStore16:
			CHECK_TRAP(AtomicStore<uint16_t, uint32_t>(&pc));
			break;

		case Opcode::I64AtomicStore8:
			CHECK_TRAP(AtomicStore<uint8_t, uint64_t>(&pc));
			break;

		case Opcode::I64AtomicStore16:
			CHECK_TRAP(AtomicStore<uint16_t, uint64_t>(&pc));
			break;

		case Opcode::I64AtomicStore32:
			CHECK_TRAP(AtomicStore<uint32_t, uint64_t>(&pc));
			break;

		case Opcode::I32AtomicStore:
			CHECK_TRAP(AtomicStore<uint32_t>(&pc));
			break;

		case Opcode::I64AtomicStore:
			CHECK_TRAP(AtomicStore<uint64_t>(&pc));
			break;*/

		/*case Opcode::MemoryGrow: {
			Memory* memory = ReadMemory(&pc);
			uint32_t old_page_size = memory->page_limits.initial;
			uint32_t grow_pages = Pop<uint32_t>();
			uint32_t new_page_size = old_page_size + grow_pages;
			uint32_t max_page_size = memory->page_limits.has_max
				? memory->page_limits.max
				: WABT_MAX_PAGES;
			PUSH_NEG_1_AND_BREAK_IF(new_page_size > max_page_size);
			PUSH_NEG_1_AND_BREAK_IF(
				static_cast<uint64_t>(new_page_size) * WABT_PAGE_SIZE > UINT32_MAX);
			memory->data.resize(new_page_size * WABT_PAGE_SIZE);
			memory->page_limits.initial = new_page_size;
			CHECK_TRAP(Push<uint32_t>(old_page_size));
			break;
		}*/

		case Opcode::I32Add:
			CHECK_TRAP(Add());
			break;

		case Opcode::I32Sub:
			CHECK_TRAP(Sub());
			break;

		case Opcode::I32Mul:
			CHECK_TRAP(Mul());
			break;

		/*case Opcode::I32DivS:
			CHECK_TRAP(BinopTrap(IntDivS<int32_t>));
			break;

		case Opcode::I32DivU:
			CHECK_TRAP(BinopTrap(IntDivU<uint32_t>));
			break;

		case Opcode::I32RemS:
			CHECK_TRAP(BinopTrap(IntRemS<int32_t>));
			break;

		case Opcode::I32RemU:
			CHECK_TRAP(BinopTrap(IntRemU<uint32_t>));
			break;*/

		/*case Opcode::I32And:
			CHECK_TRAP(Add());
			break;*/

		/*case Opcode::I32Or:
			CHECK_TRAP(Binop(IntOr<uint32_t>));
			break;

		case Opcode::I32Xor:
			CHECK_TRAP(Binop(IntXor<uint32_t>));
			break;

		case Opcode::I32Shl:
			CHECK_TRAP(Binop(IntShl<uint32_t>));
			break;

		case Opcode::I32ShrU:
			CHECK_TRAP(Binop(IntShr<uint32_t>));
			break;

		case Opcode::I32ShrS:
			CHECK_TRAP(Binop(IntShr<int32_t>));
			break;*/

		case Opcode::I32Eq:
			CHECK_TRAP(Eq());
			break;

		case Opcode::I32Ne:
			CHECK_TRAP(Ne());
			break;

		case Opcode::I32LtS:
			CHECK_TRAP(Lt());
			break;

		case Opcode::I32LeS:
			CHECK_TRAP(Le());
			break;

		case Opcode::I32LtU:
			CHECK_TRAP(Lt());
			break;

		case Opcode::I32LeU:
			CHECK_TRAP(Le());
			break;

		case Opcode::I32GtS:
			CHECK_TRAP(Gt());
			break;

		case Opcode::I32GeS:
			CHECK_TRAP(Ge());
			break;

		case Opcode::I32GtU:
			CHECK_TRAP(Gt());
			break;

		case Opcode::I32GeU:
			CHECK_TRAP(Ge());
			break;

		/*case Opcode::I32Clz:
			CHECK_TRAP(Push<uint32_t>(Clz(Pop<uint32_t>())));
			break;

		case Opcode::I32Ctz:
			CHECK_TRAP(Push<uint32_t>(Ctz(Pop<uint32_t>())));
			break;

		case Opcode::I32Popcnt:
			CHECK_TRAP(Push<uint32_t>(Popcount(Pop<uint32_t>())));
			break;

		case Opcode::I32Eqz:
			CHECK_TRAP(Unop(IntEqz<uint32_t, uint32_t>));
			break;*/

		/*case Opcode::I64Add:
			CHECK_TRAP(Binop(Add<uint64_t>));
			break;

		case Opcode::I64Sub:
			CHECK_TRAP(Binop(Sub<uint64_t>));
			break;

		case Opcode::I64Mul:
			CHECK_TRAP(Binop(Mul<uint64_t>));
			break;

		case Opcode::I64DivS:
			CHECK_TRAP(BinopTrap(IntDivS<int64_t>));
			break;

		case Opcode::I64DivU:
			CHECK_TRAP(BinopTrap(IntDivU<uint64_t>));
			break;

		case Opcode::I64RemS:
			CHECK_TRAP(BinopTrap(IntRemS<int64_t>));
			break;

		case Opcode::I64RemU:
			CHECK_TRAP(BinopTrap(IntRemU<uint64_t>));
			break;

		case Opcode::I64And:
			CHECK_TRAP(Binop(IntAnd<uint64_t>));
			break;

		case Opcode::I64Or:
			CHECK_TRAP(Binop(IntOr<uint64_t>));
			break;

		case Opcode::I64Xor:
			CHECK_TRAP(Binop(IntXor<uint64_t>));
			break;

		case Opcode::I64Shl:
			CHECK_TRAP(Binop(IntShl<uint64_t>));
			break;

		case Opcode::I64ShrU:
			CHECK_TRAP(Binop(IntShr<uint64_t>));
			break;

		case Opcode::I64ShrS:
			CHECK_TRAP(Binop(IntShr<int64_t>));
			break;

		case Opcode::I64Eq:
			CHECK_TRAP(Binop(Eq<uint64_t>));
			break;

		case Opcode::I64Ne:
			CHECK_TRAP(Binop(Ne<uint64_t>));
			break;

		case Opcode::I64LtS:
			CHECK_TRAP(Binop(Lt<int64_t>));
			break;

		case Opcode::I64LeS:
			CHECK_TRAP(Binop(Le<int64_t>));
			break;

		case Opcode::I64LtU:
			CHECK_TRAP(Binop(Lt<uint64_t>));
			break;

		case Opcode::I64LeU:
			CHECK_TRAP(Binop(Le<uint64_t>));
			break;

		case Opcode::I64GtS:
			CHECK_TRAP(Binop(Gt<int64_t>));
			break;

		case Opcode::I64GeS:
			CHECK_TRAP(Binop(Ge<int64_t>));
			break;

		case Opcode::I64GtU:
			CHECK_TRAP(Binop(Gt<uint64_t>));
			break;

		case Opcode::I64GeU:
			CHECK_TRAP(Binop(Ge<uint64_t>));
			break;

		case Opcode::I64Clz:
			CHECK_TRAP(Push<uint64_t>(Clz(Pop<uint64_t>())));
			break;

		case Opcode::I64Ctz:
			CHECK_TRAP(Push<uint64_t>(Ctz(Pop<uint64_t>())));
			break;

		case Opcode::I64Popcnt:
			CHECK_TRAP(Push<uint64_t>(Popcount(Pop<uint64_t>())));
			break;*/

		/*case Opcode::F32Add:
			CHECK_TRAP(Binop(Add<float>));
			break;

		case Opcode::F32Sub:
			CHECK_TRAP(Binop(Sub<float>));
			break;

		case Opcode::F32Mul:
			CHECK_TRAP(Binop(Mul<float>));
			break;

		case Opcode::F32Div:
			CHECK_TRAP(Binop(FloatDiv<float>));
			break;

		case Opcode::F32Min:
			CHECK_TRAP(Binop(FloatMin<float>));
			break;

		case Opcode::F32Max:
			CHECK_TRAP(Binop(FloatMax<float>));
			break;

		case Opcode::F32Abs:
			CHECK_TRAP(Unop(FloatAbs<float>));
			break;

		case Opcode::F32Neg:
			CHECK_TRAP(Unop(FloatNeg<float>));
			break;

		case Opcode::F32Copysign:
			CHECK_TRAP(Binop(FloatCopySign<float>));
			break;

		case Opcode::F32Ceil:
			CHECK_TRAP(Unop(FloatCeil<float>));
			break;

		case Opcode::F32Floor:
			CHECK_TRAP(Unop(FloatFloor<float>));
			break;

		case Opcode::F32Trunc:
			CHECK_TRAP(Unop(FloatTrunc<float>));
			break;

		case Opcode::F32Nearest:
			CHECK_TRAP(Unop(FloatNearest<float>));
			break;

		case Opcode::F32Sqrt:
			CHECK_TRAP(Unop(FloatSqrt<float>));
			break;

		case Opcode::F32Eq:
			CHECK_TRAP(Binop(Eq<float>));
			break;

		case Opcode::F32Ne:
			CHECK_TRAP(Binop(Ne<float>));
			break;

		case Opcode::F32Lt:
			CHECK_TRAP(Binop(Lt<float>));
			break;

		case Opcode::F32Le:
			CHECK_TRAP(Binop(Le<float>));
			break;

		case Opcode::F32Gt:
			CHECK_TRAP(Binop(Gt<float>));
			break;

		case Opcode::F32Ge:
			CHECK_TRAP(Binop(Ge<float>));
			break;

		case Opcode::F64Add:
			CHECK_TRAP(Binop(Add<double>));
			break;

		case Opcode::F64Sub:
			CHECK_TRAP(Binop(Sub<double>));
			break;

		case Opcode::F64Mul:
			CHECK_TRAP(Binop(Mul<double>));
			break;

		case Opcode::F64Div:
			CHECK_TRAP(Binop(FloatDiv<double>));
			break;

		case Opcode::F64Min:
			CHECK_TRAP(Binop(FloatMin<double>));
			break;

		case Opcode::F64Max:
			CHECK_TRAP(Binop(FloatMax<double>));
			break;

		case Opcode::F64Abs:
			CHECK_TRAP(Unop(FloatAbs<double>));
			break;

		case Opcode::F64Neg:
			CHECK_TRAP(Unop(FloatNeg<double>));
			break;

		case Opcode::F64Copysign:
			CHECK_TRAP(Binop(FloatCopySign<double>));
			break;

		case Opcode::F64Ceil:
			CHECK_TRAP(Unop(FloatCeil<double>));
			break;

		case Opcode::F64Floor:
			CHECK_TRAP(Unop(FloatFloor<double>));
			break;

		case Opcode::F64Trunc:
			CHECK_TRAP(Unop(FloatTrunc<double>));
			break;

		case Opcode::F64Nearest:
			CHECK_TRAP(Unop(FloatNearest<double>));
			break;

		case Opcode::F64Sqrt:
			CHECK_TRAP(Unop(FloatSqrt<double>));
			break;

		case Opcode::F64Eq:
			CHECK_TRAP(Binop(Eq<double>));
			break;

		case Opcode::F64Ne:
			CHECK_TRAP(Binop(Ne<double>));
			break;

		case Opcode::F64Lt:
			CHECK_TRAP(Binop(Lt<double>));
			break;

		case Opcode::F64Le:
			CHECK_TRAP(Binop(Le<double>));
			break;

		case Opcode::F64Gt:
			CHECK_TRAP(Binop(Gt<double>));
			break;

		case Opcode::F64Ge:
			CHECK_TRAP(Binop(Ge<double>));
			break;

		case Opcode::I32TruncSF32:
			CHECK_TRAP(UnopTrap(IntTrunc<int32_t, float>));
			break;

		case Opcode::I32TruncSSatF32:
			CHECK_TRAP(Unop(IntTruncSat<int32_t, float>));
			break;

		case Opcode::I32TruncSF64:
			CHECK_TRAP(UnopTrap(IntTrunc<int32_t, double>));
			break;

		case Opcode::I32TruncSSatF64:
			CHECK_TRAP(Unop(IntTruncSat<int32_t, double>));
			break;

		case Opcode::I32TruncUF32:
			CHECK_TRAP(UnopTrap(IntTrunc<uint32_t, float>));
			break;

		case Opcode::I32TruncUSatF32:
			CHECK_TRAP(Unop(IntTruncSat<uint32_t, float>));
			break;

		case Opcode::I32TruncUF64:
			CHECK_TRAP(UnopTrap(IntTrunc<uint32_t, double>));
			break;

		case Opcode::I32TruncUSatF64:
			CHECK_TRAP(Unop(IntTruncSat<uint32_t, double>));
			break;

		case Opcode::I32WrapI64:
			CHECK_TRAP(Push<uint32_t>(Pop<uint64_t>()));
			break;

		case Opcode::I64TruncSF32:
			CHECK_TRAP(UnopTrap(IntTrunc<int64_t, float>));
			break;

		case Opcode::I64TruncSSatF32:
			CHECK_TRAP(Unop(IntTruncSat<int64_t, float>));
			break;

		case Opcode::I64TruncSF64:
			CHECK_TRAP(UnopTrap(IntTrunc<int64_t, double>));
			break;

		case Opcode::I64TruncSSatF64:
			CHECK_TRAP(Unop(IntTruncSat<int64_t, double>));
			break;

		case Opcode::I64TruncUF32:
			CHECK_TRAP(UnopTrap(IntTrunc<uint64_t, float>));
			break;

		case Opcode::I64TruncUSatF32:
			CHECK_TRAP(Unop(IntTruncSat<uint64_t, float>));
			break;

		case Opcode::I64TruncUF64:
			CHECK_TRAP(UnopTrap(IntTrunc<uint64_t, double>));
			break;

		case Opcode::I64TruncUSatF64:
			CHECK_TRAP(Unop(IntTruncSat<uint64_t, double>));
			break;

		case Opcode::I64ExtendSI32:
			CHECK_TRAP(Push<uint64_t>(Pop<int32_t>()));
			break;

		case Opcode::I64ExtendUI32:
			CHECK_TRAP(Push<uint64_t>(Pop<uint32_t>()));
			break;*/

		/*case Opcode::F32ConvertSI32:
			CHECK_TRAP(Push<float>(Pop<int32_t>()));
			break;

		case Opcode::F32ConvertUI32:
			CHECK_TRAP(Push<float>(Pop<uint32_t>()));
			break;

		case Opcode::F32ConvertSI64:
			CHECK_TRAP(Push<float>(Pop<int64_t>()));
			break;

		case Opcode::F32ConvertUI64:
			CHECK_TRAP(Push<float>(wabt_convert_uint64_to_float(Pop<uint64_t>())));
			break;

		case Opcode::F32DemoteF64: {
			typedef FloatTraits<float> F32Traits;
			typedef FloatTraits<double> F64Traits;

			uint64_t value = PopRep<double>();
			if (WABT_LIKELY((IsConversionInRange<float, double>(value)))) {
				CHECK_TRAP(Push<float>(FromRep<double>(value)));
			}
			else if (IsInRangeF64DemoteF32RoundToF32Max(value)) {
				CHECK_TRAP(PushRep<float>(F32Traits::kMax));
			}
			else if (IsInRangeF64DemoteF32RoundToNegF32Max(value)) {
				CHECK_TRAP(PushRep<float>(F32Traits::kNegMax));
			}
			else {
				uint32_t sign = (value >> 32) & F32Traits::kSignMask;
				uint32_t tag = 0;
				if (F64Traits::IsNan(value)) {
					tag = F32Traits::kQuietNanBit |
						((value >> (F64Traits::kSigBits - F32Traits::kSigBits)) &
							F32Traits::kSigMask);
				}
				CHECK_TRAP(PushRep<float>(sign | F32Traits::kInf | tag));
			}
			break;
		}

		case Opcode::F32ReinterpretI32:
			CHECK_TRAP(PushRep<float>(Pop<uint32_t>()));
			break;

		case Opcode::F64ConvertSI32:
			CHECK_TRAP(Push<double>(Pop<int32_t>()));
			break;

		case Opcode::F64ConvertUI32:
			CHECK_TRAP(Push<double>(Pop<uint32_t>()));
			break;

		case Opcode::F64ConvertSI64:
			CHECK_TRAP(Push<double>(Pop<int64_t>()));
			break;

		case Opcode::F64ConvertUI64:
			CHECK_TRAP(
				Push<double>(wabt_convert_uint64_to_double(Pop<uint64_t>())));
			break;

		case Opcode::F64PromoteF32:
			CHECK_TRAP(Push<double>(Pop<float>()));
			break;

		case Opcode::F64ReinterpretI64:
			CHECK_TRAP(PushRep<double>(Pop<uint64_t>()));
			break;

		case Opcode::I32ReinterpretF32:
			CHECK_TRAP(Push<uint32_t>(PopRep<float>()));
			break;

		case Opcode::I64ReinterpretF64:
			CHECK_TRAP(Push<uint64_t>(PopRep<double>()));
			break;*/

		/*case Opcode::I32Rotr:
			CHECK_TRAP(Binop(IntRotr<uint32_t>));
			break;

		case Opcode::I32Rotl:
			CHECK_TRAP(Binop(IntRotl<uint32_t>));
			break;

		case Opcode::I64Rotr:
			CHECK_TRAP(Binop(IntRotr<uint64_t>));
			break;

		case Opcode::I64Rotl:
			CHECK_TRAP(Binop(IntRotl<uint64_t>));
			break;

		case Opcode::I64Eqz:
			CHECK_TRAP(Unop(IntEqz<uint32_t, uint64_t>));
			break;

		case Opcode::I32Extend8S:
			CHECK_TRAP(Unop(IntExtendS<uint32_t, int8_t>));
			break;

		case Opcode::I32Extend16S:
			CHECK_TRAP(Unop(IntExtendS<uint32_t, int16_t>));
			break;

		case Opcode::I64Extend8S:
			CHECK_TRAP(Unop(IntExtendS<uint64_t, int8_t>));
			break;

		case Opcode::I64Extend16S:
			CHECK_TRAP(Unop(IntExtendS<uint64_t, int16_t>));
			break;

		case Opcode::I64Extend32S:
			CHECK_TRAP(Unop(IntExtendS<uint64_t, int32_t>));
			break;*/

		case Opcode::InterpAlloca: {
			uint32_t old_value_stack_top = value_stack_top_;
			size_t count = ReadU32(&pc);
			value_stack_top_ += count;
			CHECK_STACK();
			memset(&value_stack_[old_value_stack_top], 0, count * sizeof(::Value));
			break;
		}

		case Opcode::InterpBrUnless: {
			CodePointer new_pc = ReadU32(&pc);
			::Value x = Pop();
			// Create two new environments for the split.
			shared_ptr<SymEnvironment> e1(new SymEnvironment(environment));
			e1->constraints.emplace_back(new Ns_ExprConstrYeqC(*x, 0, false));
			environment = e1;

			shared_ptr<SymEnvironment> e2(new SymEnvironment(environment)); // This will be for our new thread
			unique_ptr<SymThread> newThread(new SymThread(*this));
			e2->constraints.emplace_back(new Ns_ExprConstrYeqC(*x, 0, true));
			newThread->environment = e2;
			newThread->pc_ = new_pc;
			break;
		}

		case Opcode::Drop:
			(void)Pop();
			break;

		case Opcode::InterpDropKeep: {
			uint32_t drop_count = ReadU32(&pc);
			uint8_t keep_count = *pc++;
			DropKeep(drop_count, keep_count);
			break;
		}

		case Opcode::Nop:
			break;

			// The following opcodes are either never generated or should never be
			// executed.
		case Opcode::Block:
		case Opcode::Catch:
		case Opcode::Else:
		case Opcode::End:
		case Opcode::If:
		case Opcode::IfExcept:
		case Opcode::InterpData:
		case Opcode::Invalid:
		case Opcode::Loop:
		case Opcode::Rethrow:
		case Opcode::Throw:
		case Opcode::Try:
			WABT_UNREACHABLE;
			break;
		}
	}

exit_loop:
	pc_ = pc - istream;
	return result;
}