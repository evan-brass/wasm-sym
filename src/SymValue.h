#pragma once

#include <naxos.h>
#include <interp.h>

class SymValue : public naxos::NsIntVar{
public:
	SymValue() : naxos::NsIntVar() {}
	
	static naxos::NsProblemManager pm;

protected:
	wabt::Type type;

};