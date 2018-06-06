#pragma once

#include <memory>

#include <interp.h>

#include "SymEnvironment.h"


class SymThread : public wabt::interp::Thread
{
public:
//	wabt::interp::Result Run(int num_instructions);

	const uint8_t* GetIstream() const override { return environment->istream_->data.data(); }

	explicit SymThread(const std::shared_ptr<SymEnvironment> & e) : wabt::interp::Thread(nullptr), environment(e) {};
	std::shared_ptr<SymEnvironment> environment;
};

