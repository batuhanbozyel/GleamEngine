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
