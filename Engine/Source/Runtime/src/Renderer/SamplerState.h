#pragma once

namespace Gleam {

enum class FilterMode
{
	Point,
	Bilinear,
	Trilinear
};

enum class WrapMode
{
	Repeat,
	Clamp,
	Mirror,
	MirrorOnce
};

struct SamplerState
{
    FilterMode filterMode = FilterMode::Point;
    WrapMode wrapMode = WrapMode::Clamp;

	bool operator==(const SamplerState& other) const
    {
        return filterMode == other.filterMode && wrapMode == other.wrapMode;
    }
};

} // namespace Gleam
