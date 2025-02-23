#pragma once
#include "BlendState.h"

namespace Gleam {

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
    CompareFunction compareFunction = CompareFunction::Always;
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
	Triangles,
    TriangleStrip
};

struct PipelineStateDescriptor
{
    BlendState blendState{};
    DepthState depthState{};
    StencilState stencilState{};
    CullMode cullingMode = CullMode::Off;
	PrimitiveTopology topology = PrimitiveTopology::Triangles;
    bool alphaToCoverage = false;
	bool wireframe = false;

	bool operator==(const PipelineStateDescriptor& other) const
    {
        return  blendState == other.blendState &&
                depthState == other.depthState &&
                stencilState == other.stencilState &&
                cullingMode == other.cullingMode &&
                topology == other.topology &&
                alphaToCoverage == other.alphaToCoverage &&
				wireframe == other.wireframe;
    }
};

struct GraphicsPipelineStateDescriptor : PipelineStateDescriptor
{
	TArray<TextureFormat> colorFormats = {};
	TextureFormat depthFormat = TextureFormat::None;

	TString vertexEntry{};
	TString fragmentEntry{};
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

template <>
struct std::hash<Gleam::DepthState>
{
    size_t operator()(const Gleam::DepthState& depthState) const
    {
        size_t hash = 0;
        Gleam::hash_combine(hash, depthState.compareFunction);
        Gleam::hash_combine(hash, depthState.writeEnabled);
        return hash;
    }
};

template <>
struct std::hash<Gleam::StencilState>
{
    size_t operator()(const Gleam::StencilState& stencilState) const
    {
        std::size_t hash = 0;
        Gleam::hash_combine(hash, stencilState.enabled);
        Gleam::hash_combine(hash, stencilState.reference);
        Gleam::hash_combine(hash, stencilState.compareFunction);
        Gleam::hash_combine(hash, stencilState.readMask);
        Gleam::hash_combine(hash, stencilState.writeMask);
        Gleam::hash_combine(hash, stencilState.failOperation);
        Gleam::hash_combine(hash, stencilState.passOperation);
        Gleam::hash_combine(hash, stencilState.depthFailOperation);
        return hash;
    }
};

template <>
struct std::hash<Gleam::PipelineStateDescriptor>
{
    size_t operator()(const Gleam::PipelineStateDescriptor& descriptor) const
    {
        std::size_t hash = 0;
        Gleam::hash_combine(hash, descriptor.blendState);
        Gleam::hash_combine(hash, descriptor.depthState);
        Gleam::hash_combine(hash, descriptor.stencilState);
        Gleam::hash_combine(hash, descriptor.cullingMode);
        Gleam::hash_combine(hash, descriptor.topology);
        Gleam::hash_combine(hash, descriptor.alphaToCoverage);
		Gleam::hash_combine(hash, descriptor.wireframe);
        return hash;
    }
};

template <>
struct std::hash<Gleam::GraphicsPipelineStateDescriptor>
{
	size_t operator()(const Gleam::GraphicsPipelineStateDescriptor& descriptor) const
	{
		std::size_t hash = std::hash<Gleam::PipelineStateDescriptor>()(descriptor);
		Gleam::hash_combine(hash, descriptor.vertexEntry);
		Gleam::hash_combine(hash, descriptor.fragmentEntry);
		for (const auto colorFormat : descriptor.colorFormats)
		{
			Gleam::hash_combine(hash, colorFormat);
		}
		Gleam::hash_combine(hash, descriptor.depthFormat);
		return hash;
	}
};

GLEAM_ENUM(Gleam::CompareFunction, Guid("101AB027-6BC3-4323-8239-D26414769AE9"))
GLEAM_ENUM(Gleam::StencilOp, Guid("A47E53E9-5241-4D0D-B92D-53252F432AA0"))
GLEAM_ENUM(Gleam::CullMode, Guid("26E2A687-B6E2-40E1-9153-CDFA49036D5B"))
GLEAM_ENUM(Gleam::PrimitiveTopology, Guid("EA2043A9-969A-4B45-ABBE-E29CECC2D193"))

GLEAM_TYPE(Gleam::DepthState, Guid("8C7CB2F5-A47D-459B-88E4-23EB990E8B56"))
	GLEAM_FIELD(compareFunction, Serializable())
	GLEAM_FIELD(writeEnabled, Serializable())
GLEAM_END

GLEAM_TYPE(Gleam::StencilState, Guid("44649502-2AE5-47DD-9A9C-322E01BCE028"))
	GLEAM_FIELD(enabled, Serializable())
	GLEAM_FIELD(reference, Serializable())
	GLEAM_FIELD(compareFunction, Serializable())
	GLEAM_FIELD(readMask, Serializable())
	GLEAM_FIELD(writeMask, Serializable())
	GLEAM_FIELD(failOperation, Serializable())
	GLEAM_FIELD(passOperation, Serializable())
	GLEAM_FIELD(depthFailOperation, Serializable())
GLEAM_END

GLEAM_TYPE(Gleam::PipelineStateDescriptor, Guid("B7B4E150-285D-47CD-8116-DEE2CE7AAF9C"))
	GLEAM_FIELD(blendState, Serializable())
	GLEAM_FIELD(depthState, Serializable())
	GLEAM_FIELD(stencilState, Serializable())
	GLEAM_FIELD(cullingMode, Serializable())
	GLEAM_FIELD(topology, Serializable())
	GLEAM_FIELD(alphaToCoverage, Serializable())
	GLEAM_FIELD(wireframe, Serializable())
GLEAM_END

GLEAM_TYPE(Gleam::GraphicsPipelineStateDescriptor, Guid("413BE6F8-3433-4B3A-8ABF-31F89EE7AA1F"), bases<Gleam::PipelineStateDescriptor>)
	GLEAM_FIELD(colorFormats, Serializable())
	GLEAM_FIELD(depthFormat, Serializable())
	GLEAM_FIELD(vertexEntry, Serializable())
	GLEAM_FIELD(fragmentEntry, Serializable())
GLEAM_END
