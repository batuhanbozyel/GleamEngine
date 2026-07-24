#pragma once
#include "WindowConfig.h"
#include "Renderer/RendererConfig.h"

#include <ostream>

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

inline std::ostream& operator<<(std::ostream& os, const Version& version)
{
	return os << version.major << "." << version.minor << "." << version.patch;
}

} // namespace Gleam