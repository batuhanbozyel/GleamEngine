#pragma once
#include "GraphicsObject.h"
#include "TextureDescriptor.h"
#include "PipelineStateDescriptor.h"

namespace Gleam {

class GraphicsDevice;

struct PipelineHandle
{
	size_t data = 0;

	NO_DISCARD operator size_t() const
	{
		return data;
	}

	FORCE_INLINE constexpr bool operator==(const PipelineHandle& other) const
	{
		return data == other.data;
	}

	FORCE_INLINE constexpr bool operator!=(const PipelineHandle& other) const
	{
		return !(*this == other);
	}
};

class Pipeline : public GraphicsObject
{
	friend class GraphicsDevice;

public:

	Pipeline() = default;

	Pipeline(const Pipeline& other) = default;

	Pipeline& operator=(const Pipeline& other) = default;

	Pipeline(PipelineHandle hash)
		: mHash(hash)
	{
		
	}

	virtual ~Pipeline() = default;

	bool operator==(const Pipeline& other) const
	{
		return mHash.data == other.mHash.data;
	}

	PipelineHandle GetHash() const
	{
		return mHash;
	}

private:

	PipelineHandle mHash = {};

};

class GraphicsPipeline : public Pipeline
{
	friend class GraphicsDevice;

public:

	GraphicsPipeline() = default;

	GraphicsPipeline(const GraphicsPipeline& other) = default;

	GraphicsPipeline& operator=(const GraphicsPipeline& other) = default;

	GraphicsPipeline(const GraphicsPipelineStateDescriptor& descriptor)
		: Pipeline(PipelineHandle{ eastl::hash<GraphicsPipelineStateDescriptor>()(descriptor) })
		, mDescriptor(descriptor)
	{

	}

	const GraphicsPipelineStateDescriptor& GetDescriptor() const
	{
		return mDescriptor;
	}

private:

	GraphicsPipelineStateDescriptor mDescriptor;

};

struct GraphicsPipelineHandle : PipelineHandle
{
	NO_DISCARD operator GraphicsPipeline() const
	{
		return GetPipeline();
	}

	NO_DISCARD const GraphicsPipeline& GetPipeline() const;
};

} // namespace Gleam

template <>
struct std::hash<Gleam::PipelineHandle>
{
	size_t operator()(Gleam::PipelineHandle handle) const
	{
		return handle.data;
	}
};

template <>
struct std::hash<Gleam::GraphicsPipelineHandle>
{
	size_t operator()(Gleam::GraphicsPipelineHandle handle) const
	{
		return handle.data;
	}
};

template <>
struct eastl::hash<Gleam::PipelineHandle>
{
	size_t operator()(Gleam::PipelineHandle handle) const
	{
		return std::hash<Gleam::PipelineHandle>()(handle);
	}
};

template <>
struct eastl::hash<Gleam::GraphicsPipelineHandle>
{
	size_t operator()(Gleam::GraphicsPipelineHandle handle) const
	{
		return std::hash<Gleam::GraphicsPipelineHandle>()(handle);
	}
};