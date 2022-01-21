#pragma once
#include "WindowConfig.h"
#include "Renderer/RendererConfig.h"

namespace Gleam {

struct Version
{
	uint32_t major = 1;
	uint32_t minor = 0;
	uint32_t patch = 0;
    
    constexpr Version() = default;
    constexpr Version(uint32_t major, uint32_t minor, uint32_t patch)
        : major(major), minor(minor), patch(patch)
    {
        
    }
};

struct ApplicationProperties
{
	Version appVersion;
	WindowProperties windowProps;
	RendererProperties rendererProps;
};

inline std::ostream& operator<<(std::ostream& os, const Version& version)
{
	return os << version.major << "." << version.minor << "." << version.patch;
}

} // namespace Gleam
