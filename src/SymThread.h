#pragma once

#include <interp.h>

class SymThread : public wabt::interp::Thread
{
public:
	explicit SymThread(wabt::interp::Environment * e) : wabt::interp::Thread(e) {

	}
};

