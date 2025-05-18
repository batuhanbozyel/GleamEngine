#pragma once
#include "BlendState.h"

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

GSTRUCT(PipelineStateDescriptor, "B7B4E150-285D-47CD-8116-DEE2CE7AAF9C", Serializable)
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

GSTRUCT(GraphicsPipelineStateDescriptor, "413BE6F8-3433-4B3A-8ABF-31F89EE7AA1F", Serializable)
	: PipelineStateDescriptor
{
	GFIELD("FD3AFFD7-AB6C-4BE0-B9DB-2DF0FE9C69D0", Serializable)
	TArray<TextureFormat> colorFormats = {};

	GFIELD("6053A402-284D-4ECA-BE38-1D43E9FF5E83", Serializable)
	TextureFormat depthFormat = TextureFormat::None;

	GFIELD("B0932781-4564-4051-AE05-6C75F66AF334", Serializable)
	TString vertexEntry{};

	GFIELD("AD6505CC-90E8-4A24-873B-775568EE251C", Serializable)
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
