#pragma once
#include "Core/GUID.h"
#include "Reflection/Macro.h"

namespace Gleam {

GENUM(BlendOp, "D26E6319-703C-41B1-BA5F-013F93C24997", Serializable)
{
    GITEM(Add, "A2F9E4C3-1D28-4B7E-9536-E21F04A8C97D"),
    GITEM(Subtract, "D8C71A9B-4E63-42F0-B279-85FC30D6E4AB"),
    GITEM(ReverseSubtract, "65E8F237-B9CA-4D10-A36F-2BC17D845E29"),
    GITEM(Min, "3F09B6C5-7D48-4E21-A90B-F57DC3E819A4"),
    GITEM(Max, "92A4C187-6FD3-45EB-B0C9-24A8E7D31B56")
};

GENUM(BlendMode, "8828343E-F79D-4D31-98E9-ED4583DACF37", Serializable)
{
	GITEM(Zero, "F1B6A2E7-C9D5-48F0-A3B7-264E9D58C71A"),
	GITEM(One, "49D2E7B8-A1C6-4F93-87D0-5B9EA4C2F38D"),
	GITEM(DstColor, "6B38E7A9-25D1-4FC7-B940-31A8E76C52D9"),
	GITEM(SrcColor, "C93D17B8-A4F6-45E0-8D92-7BC41A6F3E58"),
	GITEM(OneMinusDstColor, "D6A8F9C3-4B27-5E10-9A38-2FC71D4E59B6"),
	GITEM(SrcAlpha, "37B2E9C5-8F6A-4D21-9E73-A8D4F56C1B90"),
	GITEM(OneMinusSrcColor, "91C6A8E3-F7D2-4B59-A036-7E9D28F1C5B4"),
	GITEM(DstAlpha, "4E9D7B36-A1C8-4F52-B937-28D6E9F3A5C1"),
	GITEM(OneMinusDstAlpha, "72A9F5E8-B3C1-4D76-9E30-5B8F72A4D16C"),
	GITEM(SrcAlphaClamp, "E76D2C9B-5A83-4F17-B049-3A6D8E2F5C97"),
	GITEM(OneMinusSrcAlpha, "83C9D2A5-F6B7-4E19-8D30-5A4E7B9C2D68")
};

GENUM(ColorWriteMask, "F918BF9C-F295-470B-AC85-3935F3870C6F", Serializable)
{
	GITEM(Alpha, "2E8FC6A7-B9D3-4E17-95A6-3F8D1C7B4E29"),
	GITEM(Blue, "7A5F3D9C-E2B1-4867-93D0-5C8B4E7A2F19"),
	GITEM(Green, "C9B8D7E6-5F4A-4321-B098-7A6D5F4E3C2B"),
	GITEM(Red, "38F9E7D6-C5B4-4A32-9D18-7E6F5D4C3B2A"),
	GITEM(All, "74D65B3A-E9C8-4F17-B2D0-39A8E7C6B5F4")
};

GSTRUCT(BlendState, "44649502-2AE5-47DD-9A9C-322E01BCE028", Serializable)
{
	GFIELD("E17B2C9A-8F3D-4A56-B2E9-D1C7F84A396B", Serializable)
	bool enabled = false;

	GFIELD("7A5B3C8D-9E2F-41A7-B063-D8E9F7C5A4B3", Serializable)
	BlendOp colorBlendOperation = BlendOp::Add;

	GFIELD("D3E8F7C6-5B4A-4932-A1B0-E9D8C7F6A5B4", Serializable)
	BlendOp alphaBlendOperation = BlendOp::Add;

	GFIELD("2A9B8C7D-6E5F-4A31-B9C0-D8E7F6A5B4C3", Serializable)
	BlendMode sourceColorBlendMode = BlendMode::Zero;

	GFIELD("F8E7D6C5-B4A3-4921-80F7-E6D5C4B3A291", Serializable)
	BlendMode sourceAlphaBlendMode = BlendMode::Zero;

	GFIELD("A3B2C1D0-E9F8-4765-B4A3-C2D1E0F9A8B7", Serializable)
	BlendMode destinationColorBlendMode = BlendMode::Zero;

	GFIELD("6F5E4D3C-2B1A-4098-87F6-E5D4C3B2A1F0", Serializable)
	BlendMode destinationAlphaBlendMode = BlendMode::Zero;

	GFIELD("9E8D7C6B-5A49-4F38-B2C7-D6E5F4A3B2C1", Serializable)
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
