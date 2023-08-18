#pragma once

namespace Gleam {

enum class FilterMode
{
	Point,
	Bilinear,
	Trilinear,
    COUNT
};

enum class WrapMode
{
	Repeat,
	Clamp,
	Mirror,
	MirrorOnce,
    COUNT
};

struct SamplerState
{
    FilterMode filterMode = FilterMode::Point;
    WrapMode wrapMode = WrapMode::Clamp;

	bool operator==(const SamplerState& other) const
    {
        return filterMode == other.filterMode && wrapMode == other.wrapMode;
    }
    
    static constexpr TArray<SamplerState, 12> GetAllVariations()
    {
        uint32_t index = 0;
        TArray<SamplerState, 12> samplerStates;
        for (uint32_t filterMode = 0; filterMode < static_cast<size_t>(FilterMode::COUNT); filterMode++)
        {
            for (uint32_t wrapMode = 0; wrapMode < static_cast<size_t>(WrapMode::COUNT); wrapMode++)
            {
                samplerStates[index].filterMode = static_cast<FilterMode>(filterMode);
                samplerStates[index].wrapMode = static_cast<WrapMode>(wrapMode);
                index++;
            }
        }
        return samplerStates;
    }
};

} // namespace Gleam

template <>
struct std::hash<Gleam::SamplerState>
{
    size_t operator()(const Gleam::SamplerState& state) const
    {
        return static_cast<size_t>(state.filterMode) * static_cast<size_t>(Gleam::WrapMode::MirrorOnce) + static_cast<size_t>(state.wrapMode);
    }
};
