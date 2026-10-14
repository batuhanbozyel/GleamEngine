#pragma once
#include "Container/String.h"

namespace Gleam {

struct ProcessResult
{
	int exitCode = -1;
	TString output;
};

ProcessResult RunProcess(const TStringView commandLine);

} // namespace Gleam
