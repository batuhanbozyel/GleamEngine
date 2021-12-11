#pragma once
#include "WindowConfig.h"

namespace Gleam {

struct Version
{
	uint32_t major = 1;
	uint32_t minor = 0;
	uint32_t patch = 0;
};

struct ApplicationProperties
{
	Version appVersion;
	WindowProperties windowProps;
};

inline std::ostream& operator<<(std::ostream& os, const Version& version)
{
	return os << version.major << "." << version.minor << "." << version.patch;
}

} // namespace Gleam