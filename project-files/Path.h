#pragma once

#include "SymEnv.h"
#include <memory>

class Path
{
private:
	SymEnv env;
	std::shared_ptr<Path> parent;
public:
	Path();
	~Path();
};

