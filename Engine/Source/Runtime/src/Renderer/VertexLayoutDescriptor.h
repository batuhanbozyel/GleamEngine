#pragma once

namespace Gleam {

enum class PrimitiveTopology
{
	Points,
	Lines,
	LineStrip,
	Triangles
};

enum class VertexAttributeFormat
{
	Float32,
	Float16,

	UInt32,
	UInt16,
	UInt8,

	SInt32,
	SInt16,
	SInt8,

	UNorm16,
	UNorm8,

	SNorm16,
	SNorm8
};

static constexpr uint32_t GetVertexAttributeFormatSize(VertexAttributeFormat format)
{
	switch (format)
	{
		case VertexAttributeFormat::Float32:	return 4;
		case VertexAttributeFormat::Float16:	return 2;
		case VertexAttributeFormat::UInt32:		return 4;
		case VertexAttributeFormat::UInt16:		return 2;
		case VertexAttributeFormat::UInt8:		return 1;
		case VertexAttributeFormat::SInt32:		return 4;
		case VertexAttributeFormat::SInt16:		return 2;
		case VertexAttributeFormat::SInt8:		return 1;
		case VertexAttributeFormat::UNorm16:	return 2;
		case VertexAttributeFormat::UNorm8:		return 1;
		case VertexAttributeFormat::SNorm16:	return 2;
		case VertexAttributeFormat::SNorm8:		return 1;
		default:								return 0;
	}
}

struct VertexAttribute
{
	uint32_t offset = 0;
	VertexAttributeFormat format = VertexAttributeFormat::Float32;
};

struct VertexLayoutDescriptor
{
	TArray<VertexAttribute> attributes{};
	uint32_t binding = 0;
	uint32_t stride = 0;

	VertexLayoutDescriptor(const TArray<VertexAttributeFormat>& formats)
		: attributes(formats.size())
	{
		uint32_t offset = 0;
		for (uint32_t i = 0; i < formats.size(); i++)
		{
			attributes[i].format = formats[i];
			attributes[i].offset = offset;
			offset += GetVertexAttributeFormatSize(formats[i]);
		}
		stride = offset;
	}
};

} // namespace Gleam