#pragma once

namespace Gleam {

enum class BlendOp
{
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

enum class BlendMode
{
    Zero,
    One,
    DstColor,
    SrcColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcColor,
    DstAlpha,
    OneMinusDstAlpha,
    SrcAlphaClamp,
    OneMinusSrcAlpha
};

enum class ColorWriteMask
{
    Alpha,
    Blue,
    Green,
    Red,
    All
};

struct BlendState
{
    bool enabled = false;
    BlendOp colorBlendOperation = BlendOp::Add;
    BlendOp alphaBlendOperation = BlendOp::Add;
    BlendMode sourceColorBlendMode = BlendMode::Zero;
    BlendMode sourceAlphaBlendMode = BlendMode::Zero;
    BlendMode destinationColorBlendMode = BlendMode::Zero;
    BlendMode destinationAlphaBlendMode = BlendMode::Zero;
    ColorWriteMask writeMask = ColorWriteMask::All;

	bool operator==(const BlendState& other) const
    {
        return  enabled == other.enabled &&
                colorBlendOperation == other.colorBlendOperation &&
                alphaBlendOperation == other.alphaBlendOperation &&
                sourceColorBlendMode == other.sourceColorBlendMode &&
                sourceAlphaBlendMode == other.sourceAlphaBlendMode &&
                destinationColorBlendMode == other.destinationColorBlendMode &&
                destinationAlphaBlendMode == other.destinationAlphaBlendMode &&
                writeMask == other.writeMask;
    }
};

} // namespace Gleam

template <>
struct std::hash<Gleam::BlendState>
{
    size_t operator()(const Gleam::BlendState& blendState) const
    {
        size_t hash = 0;
        Gleam::hash_combine(hash, blendState.enabled);
        Gleam::hash_combine(hash, blendState.colorBlendOperation);
        Gleam::hash_combine(hash, blendState.alphaBlendOperation);
        Gleam::hash_combine(hash, blendState.sourceColorBlendMode);
        Gleam::hash_combine(hash, blendState.sourceAlphaBlendMode);
        Gleam::hash_combine(hash, blendState.destinationColorBlendMode);
        Gleam::hash_combine(hash, blendState.destinationAlphaBlendMode);
        Gleam::hash_combine(hash, blendState.writeMask);
        return hash;
    }
};

GLEAM_ENUM(Gleam::BlendOp, Guid("D26E6319-703C-41B1-BA5F-013F93C24997"))
GLEAM_ENUM(Gleam::BlendMode, Guid("8828343E-F79D-4D31-98E9-ED4583DACF37"))
GLEAM_ENUM(Gleam::ColorWriteMask, Guid("F918BF9C-F295-470B-AC85-3935F3870C6F"))
GLEAM_TYPE(Gleam::BlendState, Guid("44649502-2AE5-47DD-9A9C-322E01BCE028"))
	GLEAM_FIELD(enabled, Serializable())
	GLEAM_FIELD(colorBlendOperation, Serializable())
	GLEAM_FIELD(alphaBlendOperation, Serializable())
	GLEAM_FIELD(sourceColorBlendMode, Serializable())
	GLEAM_FIELD(sourceAlphaBlendMode, Serializable())
	GLEAM_FIELD(destinationColorBlendMode, Serializable())
	GLEAM_FIELD(destinationAlphaBlendMode, Serializable())
	GLEAM_FIELD(writeMask, Serializable())
GLEAM_END
