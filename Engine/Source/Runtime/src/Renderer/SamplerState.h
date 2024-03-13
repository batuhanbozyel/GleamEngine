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
    
	static constexpr TArray<SamplerState, 12> GetStaticSamplers()
    {
		return TArray<SamplerState, 12>{
			// Sampler_Point_Repeat
			SamplerState{
				.filterMode = FilterMode::Point,
				.wrapMode = WrapMode::Repeat
			},
			// Sampler_Point_Clamp
			SamplerState{
				.filterMode = FilterMode::Point,
				.wrapMode = WrapMode::Clamp
			},
			// Sampler_Point_Mirror
			SamplerState{
				.filterMode = FilterMode::Point,
				.wrapMode = WrapMode::Mirror
			},
			// Sampler_Point_MirrorOnce
			SamplerState{
				.filterMode = FilterMode::Point,
				.wrapMode = WrapMode::MirrorOnce
			},
			// Sampler_Bilinear_Repeat
			SamplerState{
				.filterMode = FilterMode::Bilinear,
				.wrapMode = WrapMode::Repeat
			},
			// Sampler_Bilinear_Clamp
			SamplerState{
				.filterMode = FilterMode::Bilinear,
				.wrapMode = WrapMode::Clamp
			},
			// Sampler_Bilinear_Mirror
			SamplerState{
				.filterMode = FilterMode::Bilinear,
				.wrapMode = WrapMode::Mirror
			},
			// Sampler_Bilinear_MirrorOnce
			SamplerState{
				.filterMode = FilterMode::Bilinear,
				.wrapMode = WrapMode::MirrorOnce
			},
			// Sampler_Trilinear_Repeat
			SamplerState{
				.filterMode = FilterMode::Trilinear,
				.wrapMode = WrapMode::Repeat
			},
			// Sampler_Trilinear_Clamp
			SamplerState{
				.filterMode = FilterMode::Trilinear,
				.wrapMode = WrapMode::Clamp
			},
			// Sampler_Trilinear_Mirror
			SamplerState{
				.filterMode = FilterMode::Trilinear,
				.wrapMode = WrapMode::Mirror
			},
			// Sampler_Trilinear_MirrorOnce
			SamplerState{
				.filterMode = FilterMode::Trilinear,
				.wrapMode = WrapMode::MirrorOnce
			}
		};
    }
};

} // namespace Gleam
