#pragma once
#include "GraphicsObject.h"
#include "TextureDescriptor.h"
#include "ShaderBindingTable.h"
#include "PipelineStateDescriptor.h"

namespace Gleam {

class GraphicsDevice;

enum class PipelineType
{
	Graphics,
	Compute,
	RayTracing,
	Mesh
};

using NativePipelineHandle = size_t;

struct PipelineHandle
{
	size_t data = 0;
	PipelineType type = PipelineType::Graphics;

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

	FORCE_INLINE constexpr bool IsValid() const
	{
		return data != 0;
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
		: Pipeline(PipelineHandle{ eastl::hash<GraphicsPipelineStateDescriptor>()(descriptor), PipelineType::Graphics })
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

class ComputePipeline : public Pipeline
{
	friend class GraphicsDevice;

public:

	ComputePipeline() = default;

	ComputePipeline(const ComputePipeline& other) = default;

	ComputePipeline& operator=(const ComputePipeline& other) = default;

	ComputePipeline(const ComputePipelineStateDescriptor& descriptor)
		: Pipeline(PipelineHandle{ eastl::hash<ComputePipelineStateDescriptor>()(descriptor), PipelineType::Compute })
		, mDescriptor(descriptor)
	{

	}

	const ComputePipelineStateDescriptor& GetDescriptor() const
	{
		return mDescriptor;
	}

private:

	ComputePipelineStateDescriptor mDescriptor;
};

class RayTracingPipeline : public Pipeline
{
	friend class GraphicsDevice;

public:

	RayTracingPipeline() = default;

	RayTracingPipeline(const RayTracingPipeline& other) = default;

	RayTracingPipeline& operator=(const RayTracingPipeline& other) = default;

	RayTracingPipeline(const RayTracingPipelineStateDescriptor& descriptor)
		: Pipeline(PipelineHandle{ eastl::hash<RayTracingPipelineStateDescriptor>()(descriptor), PipelineType::RayTracing })
		, mDescriptor(descriptor)
	{

	}

	const ShaderBindingTable& GetShaderBindingTable() const
	{
		return mShaderBindingTable;
	}

	const RayTracingPipelineStateDescriptor& GetDescriptor() const
	{
		return mDescriptor;
	}

private:

	ShaderBindingTable mShaderBindingTable;
	RayTracingPipelineStateDescriptor mDescriptor;
};

class MeshPipeline : public Pipeline
{
	friend class GraphicsDevice;

public:

	MeshPipeline() = default;

	MeshPipeline(const MeshPipeline& other) = default;

	MeshPipeline& operator=(const MeshPipeline& other) = default;

	MeshPipeline(const MeshPipelineStateDescriptor& descriptor)
		: Pipeline(PipelineHandle{ eastl::hash<MeshPipelineStateDescriptor>()(descriptor), PipelineType::Mesh })
		, mDescriptor(descriptor)
	{

	}

	const MeshPipelineStateDescriptor& GetDescriptor() const
	{
		return mDescriptor;
	}

private:

	MeshPipelineStateDescriptor mDescriptor;
};

struct GraphicsPipelineHandle : PipelineHandle
{
	GraphicsPipelineHandle()
		: PipelineHandle{ .data = 0, .type = PipelineType::Graphics }
	{

	}

	GraphicsPipelineHandle(size_t hash)
		: PipelineHandle{ .data = hash, .type = PipelineType::Graphics }
	{

	}

	NO_DISCARD operator GraphicsPipeline() const
	{
		return GetPipeline();
	}

	NO_DISCARD const GraphicsPipeline& GetPipeline() const;
};

struct ComputePipelineHandle : PipelineHandle
{
	ComputePipelineHandle()
		: PipelineHandle{ .data = 0, .type = PipelineType::Compute }
	{

	}

	ComputePipelineHandle(size_t hash)
		: PipelineHandle{ .data = hash, .type = PipelineType::Compute }
	{

	}

	NO_DISCARD operator ComputePipeline() const
	{
		return GetPipeline();
	}

	NO_DISCARD const ComputePipeline& GetPipeline() const;
};

struct RayTracingPipelineHandle : PipelineHandle
{
	RayTracingPipelineHandle()
		: PipelineHandle{ .data = 0, .type = PipelineType::RayTracing }
	{

	}

	RayTracingPipelineHandle(size_t hash)
		: PipelineHandle{ .data = hash, .type = PipelineType::RayTracing }
	{

	}

	NO_DISCARD operator RayTracingPipeline() const
	{
		return GetPipeline();
	}

	NO_DISCARD const RayTracingPipeline& GetPipeline() const;
};

struct MeshPipelineHandle : PipelineHandle
{
	MeshPipelineHandle()
		: PipelineHandle{ .data = 0, .type = PipelineType::Mesh }
	{

	}

	MeshPipelineHandle(size_t hash)
		: PipelineHandle{ .data = hash, .type = PipelineType::Mesh }
	{

	}

	NO_DISCARD operator MeshPipeline() const
	{
		return GetPipeline();
	}

	NO_DISCARD const MeshPipeline& GetPipeline() const;
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
struct std::hash<Gleam::ComputePipelineHandle>
{
	size_t operator()(Gleam::ComputePipelineHandle handle) const
	{
		return handle.data;
	}
};

template <>
struct std::hash<Gleam::RayTracingPipelineHandle>
{
	size_t operator()(Gleam::RayTracingPipelineHandle handle) const
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

template <>
struct eastl::hash<Gleam::ComputePipelineHandle>
{
	size_t operator()(Gleam::ComputePipelineHandle handle) const
	{
		return std::hash<Gleam::ComputePipelineHandle>()(handle);
	}
};

template <>
struct eastl::hash<Gleam::RayTracingPipelineHandle>
{
	size_t operator()(Gleam::RayTracingPipelineHandle handle) const
	{
		return std::hash<Gleam::RayTracingPipelineHandle>()(handle);
	}
};

template <>
struct std::hash<Gleam::MeshPipelineHandle>
{
	size_t operator()(Gleam::MeshPipelineHandle handle) const
	{
		return handle.data;
	}
};

template <>
struct eastl::hash<Gleam::MeshPipelineHandle>
{
	size_t operator()(Gleam::MeshPipelineHandle handle) const
	{
		return std::hash<Gleam::MeshPipelineHandle>()(handle);
	}
};