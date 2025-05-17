#pragma once
#include "WindowConfig.h"
#include "Renderer/RendererConfig.h"

namespace Gleam {

GSTRUCT(Version, "C84FF047-5FB9-4DF1-A3E6-E6EE81258F52", Serializable)
{
	GFIELD("B18AC6E9-D2E7-4315-9D39-4ABABD8D67B0", Serializable)
	uint32_t major = 1;

	GFIELD("C0536A56-7D3E-4F00-8353-BCABFA8376DE", Serializable)
	uint32_t minor = 0;

	GFIELD("0B16EB81-C6A8-46EF-AFC3-CAA70C01610D", Serializable)
	uint32_t patch = 0;
    
    constexpr Version() = default;
    constexpr Version(uint32_t major, uint32_t minor, uint32_t patch)
        : major(major), minor(minor), patch(patch)
    {
        
    }
};

GSTRUCT(EngineConfig, "858441CE-4849-44D3-94B8-213F9E6D83C5", Serializable)
{
	GFIELD("3E72FD33-B62F-4304-8D06-C6A7933D27D3", Serializable)
	WindowConfig window;

	GFIELD("ECE2D0A3-96EE-45CB-A47C-4715F6B7CD24", Serializable)
	RendererConfig renderer;
};

inline std::ostream& operator<<(std::ostream& os, const Version& version)
{
	return os << version.major << "." << version.minor << "." << version.patch;
}

} // namespace Gleam