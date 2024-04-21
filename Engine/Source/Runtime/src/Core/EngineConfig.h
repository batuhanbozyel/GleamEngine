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

struct EngineConfig
{
	WindowConfig window;
	RendererConfig renderer;
};

inline std::ostream& operator<<(std::ostream& os, const Version& version)
{
	return os << version.major << "." << version.minor << "." << version.patch;
}

} // namespace Gleam

GLEAM_TYPE(Gleam::Version, Guid("C84FF047-5FB9-4DF1-A3E6-E6EE81258F52"))
	GLEAM_FIELD(major, Serializable())
	GLEAM_FIELD(minor, Serializable())
	GLEAM_FIELD(patch, Serializable())
GLEAM_END

GLEAM_TYPE(Gleam::EngineConfig, Guid("858441CE-4849-44D3-94B8-213F9E6D83C5"))
	GLEAM_FIELD(window, Serializable())
	GLEAM_FIELD(renderer, Serializable())
GLEAM_END