#pragma once

#include "SymEnv.h"
#include <memory>

class Path
{
private:
	std::shared_ptr<Path> parent;
public:
	SymEnv env;
	Path();
	~Path();
};

