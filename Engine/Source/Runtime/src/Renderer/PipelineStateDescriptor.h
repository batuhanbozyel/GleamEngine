#pragma once

namespace Gleam {

enum class PipelineBindPoint
{
    Graphics,
    Compute
};
    
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
    BlendOp colorBlendOperation = BlendOp::Add;
    BlendOp alphaBlendOperation = BlendOp::Add;
    BlendMode destinationAlphaBlendMode = BlendMode::Zero;
    BlendMode destinationColorBlendMode = BlendMode::Zero;
    BlendMode sourceAlphaBlendMode = BlendMode::Zero;
    BlendMode sourceColorBlendMode = BlendMode::Zero;
    ColorWriteMask writeMask = ColorWriteMask::All;

	bool operator==(const BlendState&) const = default;
};

enum class CompareFunction
{
    Disabled,
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

struct DepthState
{
    CompareFunction compareFunction = CompareFunction::Less;
    bool writeEnabled = false;

	bool operator==(const DepthState&) const = default;
};

enum class StencilOp
{
    Keep,
    Zero,
    Replace,
    IncrementClamp,
    IncrementWrap,
    DecrementClamp,
    DecrementWrap,
    Invert
};

struct StencilState
{
    bool enabled = false;
    int reference = 1;
    CompareFunction compareFunction = CompareFunction::Less;
    uint8_t readMask = 0xFF;
    uint8_t writeMask = 0xFF;
    StencilOp failOperation = StencilOp::Keep;
    StencilOp passOperation = StencilOp::Keep;
    StencilOp depthFailOperation = StencilOp::Keep;

	bool operator==(const StencilState&) const = default;
};

enum class CullMode
{
    Off,
    Front,
    Back
};

enum class PrimitiveTopology
{
	Points,
	Lines,
	LineStrip,
	Triangles
};

struct PipelineStateDescriptor
{
    TArray<BlendState, 8> blendStates;
    DepthState depthState;
    StencilState stencilState;
    CullMode cullingMode = CullMode::Off;
	PrimitiveTopology topology = PrimitiveTopology::Triangles;
    PipelineBindPoint bindPoint = PipelineBindPoint::Graphics;
    bool alphaToCoverage = false;

	bool operator==(const PipelineStateDescriptor&) const = default;
};

} // namespace Gleam
