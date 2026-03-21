#pragma once
#include "BlendState.h"
#include "TextureFormat.h"
#include "Container/Array.h"
#include "Container/String.h"

namespace Gleam {

GENUM(CompareFunction, "101AB027-6BC3-4323-8239-D26414769AE9", Serializable)
{
    GITEM(Never, "B0E9E5EA-3874-45D3-9073-D2A949FA0ACD"),
    GITEM(Less, "11576547-FC6A-4ACA-8CE3-91361123F8F0"),
    GITEM(Equal, "DF45E93D-3239-4BD9-98B1-48F390A1648A"),
    GITEM(LessEqual, "5987EAD6-74B1-4433-B175-C7959EDE1FAE"),
    GITEM(Greater, "1BEC2C6E-6A3E-447B-BF79-E21F4F0BA383"),
    GITEM(NotEqual, "88F37455-3A78-4060-8782-7E8A0C59B37F"),
    GITEM(GreaterEqual, "11A6EBA5-D351-4759-B578-164B509EBFE8"),
    GITEM(Always, "C5198867-F462-441F-B25C-36BE4F415083")
};

GSTRUCT(DepthState, "8C7CB2F5-A47D-459B-88E4-23EB990E8B56", Serializable)
{
	GFIELD("BEF62653-55D7-4C26-B05A-7E1E3880F3B9", Serializable)
    CompareFunction compareFunction = CompareFunction::Always;

	GFIELD("9F3AC486-1204-42CC-B30A-34C9990D144E", Serializable)
    bool writeEnabled = false;

	bool operator==(const DepthState& other) const
    {
        return compareFunction == other.compareFunction && writeEnabled == other.writeEnabled;
    }
};

GENUM(StencilOp, "A47E53E9-5241-4D0D-B92D-53252F432AA0", Serializable)
{
    GITEM(Keep, "567EB71A-A8C6-45CC-AE5E-84E93D3FDD75"),
    GITEM(Zero, "CFBAD854-271A-40FB-81BA-EA9B92202C9F"),
    GITEM(Replace, "EE9A2302-AE00-4017-B6B7-9EB509C8866D"),
    GITEM(IncrementClamp, "8F168BD9-6CB0-438B-829F-A9A14E6C0302"),
    GITEM(IncrementWrap, "B0BAE5D2-778E-4576-8939-286D0A6901BC"),
    GITEM(DecrementClamp, "C479DB90-3B34-403A-8905-8A00460D3D84"),
    GITEM(DecrementWrap, "A714590A-8850-47D0-9C5D-CF758F2351E3"),
    GITEM(Invert, "E8562C81-4C65-41BF-8D9D-25E2A8383B32")
};

GSTRUCT(StencilState, "44649502-2AE5-47DD-9A9C-322E01BCE028", Serializable)
{
	GFIELD("A1258B52-5A88-4137-AB12-DE84F5D7D05E", Serializable)
    bool enabled = false;

	GFIELD("0439A06C-88BB-4A87-85A9-E3070B133C0F", Serializable)
    int reference = 1;

	GFIELD("4668F93C-3BCC-4F34-B76F-C5B3C51E07C8", Serializable)
	CompareFunction compareFunction = CompareFunction::Less;

	GFIELD("4D45DC0C-DE03-4AA4-B304-6B8E9389461B", Serializable)
    uint8_t readMask = 0xFF;

	GFIELD("8573D74D-D067-492B-828A-C6164F9767F4", Serializable)
    uint8_t writeMask = 0xFF;

	GFIELD("E86F50FB-5B87-4124-9EA3-678CFE94B618", Serializable)
	StencilOp failOperation = StencilOp::Keep;

	GFIELD("0CC2756B-4DAA-432B-A999-D84F32E849BF", Serializable)
    StencilOp passOperation = StencilOp::Keep;

	GFIELD("E3991E6D-53B9-4714-BE4B-C6770D68A8B5", Serializable)
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

GENUM(CullMode, "26E2A687-B6E2-40E1-9153-CDFA49036D5B", Serializable)
{
	GITEM(Off, "FB13C2EC-FE97-4D4C-AF70-07300A3D6BE2"),
	GITEM(Front, "8C0C2690-987E-463F-A625-D5B05F396FA8"),
	GITEM(Back, "FB43A56D-19EF-422F-A0F3-A66CCBDB9474")
};

GENUM(PrimitiveTopology, "EA2043A9-969A-4B45-ABBE-E29CECC2D193", Serializable)
{
	GITEM(Points, "6E4D58C6-BBFB-4692-BDF0-BAB7871696D2"),
	GITEM(Lines, "D71E99F3-5A52-4399-98A9-7C1D2BBFEDF7"),
	GITEM(LineStrip, "23E67F85-3A7E-45D0-8AAF-5B0FA0E07730"),
	GITEM(Triangles, "B5B5EABB-BB4D-4B8E-A5DA-2B3FA675DDC7"),
	GITEM(TriangleStrip, "0755D97F-372C-4A64-AE6F-B0C9F3431AFB")
};

GSTRUCT(GraphicsPipelineStateDescriptor, "413BE6F8-3433-4B3A-8ABF-31F89EE7AA1F", Serializable)
{
	GFIELD("A3FA2AC1-204D-407C-86C3-4EBF6121145A", Serializable)
	BlendState blendState{};

	GFIELD("E51436F3-FBF6-43C5-92C9-658FE1944C2C", Serializable)
	DepthState depthState{};

	GFIELD("96263BB0-EB92-468F-8A20-47CBE9A1D1FD", Serializable)
	StencilState stencilState{};

	GFIELD("9D8AACB7-7011-4BA3-AF70-5A6286CCC013", Serializable)
	CullMode cullingMode = CullMode::Off;

	GFIELD("E4DC430F-100B-4C5D-A3F3-201B53332C57", Serializable)
	PrimitiveTopology topology = PrimitiveTopology::Triangles;

	GFIELD("6CAECCC0-3CCF-42D9-9E59-05A865534544", Serializable)
	bool alphaToCoverage = false;

	GFIELD("28DA358E-239A-4784-9797-B6E89C30046A", Serializable)
	bool wireframe = false;

	GFIELD("FD3AFFD7-AB6C-4BE0-B9DB-2DF0FE9C69D0", Serializable)
	TArray<TextureFormat> colorFormats = {};

	GFIELD("6053A402-284D-4ECA-BE38-1D43E9FF5E83", Serializable)
	TextureFormat depthFormat = TextureFormat::None;

	GFIELD("B0932781-4564-4051-AE05-6C75F66AF334", Serializable)
	TString vertexEntry{};

	GFIELD("AD6505CC-90E8-4A24-873B-775568EE251C", Serializable)
	TString fragmentEntry{};

	bool operator==(const GraphicsPipelineStateDescriptor& other) const
	{
		return  blendState == other.blendState &&
				depthState == other.depthState &&
				stencilState == other.stencilState &&
				cullingMode == other.cullingMode &&
				topology == other.topology &&
				alphaToCoverage == other.alphaToCoverage &&
				wireframe == other.wireframe &&
				colorFormats == other.colorFormats &&
				depthFormat == other.depthFormat &&
				vertexEntry == other.vertexEntry &&
				fragmentEntry == other.fragmentEntry;
	}
};

GSTRUCT(ComputePipelineStateDescriptor, "C07E515A-F254-4413-8C1E-13173BB82121", Serializable)
{
	GFIELD("C5600539-09EB-4633-90A1-58C38CC54B15", Serializable)
	TString entryPoint{};

	bool operator==(const ComputePipelineStateDescriptor& other) const
	{
		return entryPoint == other.entryPoint;
	}
};

GSTRUCT(HitGroupDescriptor, "3BF561EE-C997-46E7-8E29-0AC34AADE508", Serializable)
{
	GFIELD("B088C7AE-6068-42B3-AC5B-DA9D88F8CAF1", Serializable)
	TString name{};

	GFIELD("E4AF9D00-FFB8-45DC-92A4-D49516C95CA1", Serializable)
	TString closestHitEntry{};

	GFIELD("4716A041-97B2-4EE4-9245-795B8709BC34", Serializable)
	TString anyHitEntry{};

	GFIELD("F42DAFF2-8F2E-4A1C-8AA4-EBAD1B860AE2", Serializable)
	TString intersectionEntry{};
};

GSTRUCT(RayTracingPipelineStateDescriptor, "7DE7283B-C509-4B81-8B79-9D6AE400D78A", Serializable)
{
	GFIELD("E10C3850-CD11-4AD0-A0CF-E6D79F2F899F", Serializable)
	TString rayGenerationEntry{};

	GFIELD("9184222A-F745-4666-983E-0899A044FD3B", Serializable)
	TString missEntry{};

	GFIELD("5FE237D1-99D3-4C8A-AD20-8F528B7BF0B4", Serializable)
	TArray<HitGroupDescriptor> hitGroups{};

	GFIELD("D12C58F4-D359-4DA8-9218-A8D6980CF09F", Serializable)
	uint32_t maxRecursionDepth = 1;

	GFIELD("379CF426-EB79-442C-B796-4450C8A6EED8", Serializable)
	uint32_t maxPayloadSize = 32;	// bytes, user-defined ray payload struct

	GFIELD("0074EEE1-FBCD-44BE-98DA-22FE2384E3D7", Serializable)
	uint32_t maxAttributeSize = 8;	// bytes, default = float2 barycentrics
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
struct std::hash<Gleam::GraphicsPipelineStateDescriptor>
{
	size_t operator()(const Gleam::GraphicsPipelineStateDescriptor& descriptor) const
	{
		std::size_t hash = 0;
		Gleam::hash_combine(hash, descriptor.blendState);
		Gleam::hash_combine(hash, descriptor.depthState);
		Gleam::hash_combine(hash, descriptor.stencilState);
		Gleam::hash_combine(hash, descriptor.cullingMode);
		Gleam::hash_combine(hash, descriptor.topology);
		Gleam::hash_combine(hash, descriptor.alphaToCoverage);
		Gleam::hash_combine(hash, descriptor.wireframe);
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

template <>
struct std::hash<Gleam::ComputePipelineStateDescriptor>
{
	size_t operator()(const Gleam::ComputePipelineStateDescriptor& descriptor) const
	{
		std::size_t hash = 0;
		Gleam::hash_combine(hash, descriptor.entryPoint);
		return hash;
	}
};

template <>
struct std::hash<Gleam::HitGroupDescriptor>
{
	size_t operator()(const Gleam::HitGroupDescriptor& descriptor) const
	{
		std::size_t hash = 0;
		Gleam::hash_combine(hash, descriptor.closestHitEntry);
		Gleam::hash_combine(hash, descriptor.anyHitEntry);
		Gleam::hash_combine(hash, descriptor.intersectionEntry);
		return hash;
	}
};

template <>
struct std::hash<Gleam::RayTracingPipelineStateDescriptor>
{
    size_t operator()(const Gleam::RayTracingPipelineStateDescriptor& descriptor) const
    {
		std::size_t hash = 0;
		Gleam::hash_combine(hash, descriptor.rayGenerationEntry);
		Gleam::hash_combine(hash, descriptor.missEntry);
		for (const auto& hitGroup : descriptor.hitGroups)
		{
			Gleam::hash_combine(hash, hitGroup);
		}
		return hash;
    }
};

template <>
struct eastl::hash<Gleam::DepthState>
{
	size_t operator()(const Gleam::DepthState& depthState) const
	{
		return std::hash<Gleam::DepthState>()(depthState);
	}
};

template <>
struct eastl::hash<Gleam::StencilState>
{
	size_t operator()(const Gleam::StencilState& stencilState) const
	{
		return std::hash<Gleam::StencilState>()(stencilState);
	}
};

template <>
struct eastl::hash<Gleam::GraphicsPipelineStateDescriptor>
{
	size_t operator()(const Gleam::GraphicsPipelineStateDescriptor& descriptor) const
	{
		return std::hash<Gleam::GraphicsPipelineStateDescriptor>()(descriptor);
	}
};

template <>
struct eastl::hash<Gleam::ComputePipelineStateDescriptor>
{
	size_t operator()(const Gleam::ComputePipelineStateDescriptor& descriptor) const
	{
		return std::hash<Gleam::ComputePipelineStateDescriptor>()(descriptor);
	}
};

template <>
struct eastl::hash<Gleam::HitGroupDescriptor>
{
	size_t operator()(const Gleam::HitGroupDescriptor& descriptor) const
	{
		return std::hash<Gleam::HitGroupDescriptor>()(descriptor);
	}
};

template <>
struct eastl::hash<Gleam::RayTracingPipelineStateDescriptor>
{
	size_t operator()(const Gleam::RayTracingPipelineStateDescriptor& descriptor) const
	{
		return std::hash<Gleam::RayTracingPipelineStateDescriptor>()(descriptor);
	}
};
