#pragma once
#include "BlendState.h"

namespace Gleam {

enum class PipelineBindPoint
{
    Graphics,
    Compute,
	RayTracing
};

enum class CompareFunction
{
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

	bool operator==(const DepthState& other) const
    {
        return compareFunction == other.compareFunction && writeEnabled == other.writeEnabled;
    }
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

	bool operator==(const StencilState& other) const
    {
        return  enabled == other.enabled &&
                reference == other.reference &&
                compareFunction == other.compareFunction &&
                readMask == other.readMask &&
                writeMask == other.writeMask &&
                failOperation == other.failOperation &&
                passOperation == other.passOperation &&
                depthFailOperation == other.depthFailOperation;
    }
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
    BlendState blendState;
    DepthState depthState;
    StencilState stencilState;
    CullMode cullingMode = CullMode::Off;
	PrimitiveTopology topology = PrimitiveTopology::Triangles;
    PipelineBindPoint bindPoint = PipelineBindPoint::Graphics;
    bool alphaToCoverage = false;

	bool operator==(const PipelineStateDescriptor& other) const
    {
        return  blendState == other.blendState &&
                depthState == other.depthState &&
                stencilState == other.stencilState &&
                cullingMode == other.cullingMode &&
                topology == other.topology &&
                bindPoint == other.bindPoint;
    }
};

namespace Utils {

static constexpr uint32_t PrimitiveTopologyVertexCount(PrimitiveTopology topology)
{
    switch (topology)
    {
        case PrimitiveTopology::Points: return 1;
        case PrimitiveTopology::Lines: return 2;
        case PrimitiveTopology::LineStrip: return 2;
        case PrimitiveTopology::Triangles: return 3;
        default: return 0;
    }
}

} // namespace Utils

} // namespace Gleam
