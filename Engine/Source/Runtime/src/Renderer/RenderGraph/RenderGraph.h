#pragma once
#include "RenderGraphBuilder.h"

namespace Gleam {

class RenderSurface;
class CommandBuffer;
class GraphicsDevice;
class GPUAllocator;

struct RenderContext;
struct SceneRenderingData;

struct ImportResourceParams
{
	Color clearColor = Color::clear;
	bool clearOnFirstUse = true;
};

struct RenderGraphContext
{
	GPUAllocator* allocator = nullptr;
	GraphicsDevice* device = nullptr;
	RenderSurface* surface = nullptr;
};

class RenderGraph final
{
    template<typename PassData>
    using SetupFunc = std::function<void(RenderGraphBuilder& builder, PassData& passData)>;
    
public:
    
    RenderGraph(const RenderGraphContext& context);
    
    ~RenderGraph();
    
    void Compile();

	void Execute(const CommandBuffer* cmd, SceneRenderingData& sceneData);
	
	template<typename PassData>
	const PassData& AddRenderPass(const TStringView name, SetupFunc<PassData>&& setup, RenderFunc<PassData>&& execute)
	{
        auto uniqueId = static_cast<uint32_t>(mPassNodes.size());
        auto node = new RenderGraphRenderPassNode(uniqueId, name, std::forward<decltype(execute)>(execute));
		return AddPassNode<PassData>(node, std::forward<decltype(setup)>(setup));
	}
    
    TextureHandle ImportBackbuffer(const Texture& backbuffer, const ImportResourceParams& params = ImportResourceParams());
    
    const TextureDescriptor& GetDescriptor(TextureHandle handle) const;

private:
	
	template<typename PassData>
	const PassData& AddPassNode(RenderGraphPassNode* node, SetupFunc<PassData>&& setup)
	{
		mPassNodes.push_back(node);
		auto& passData = std::any_cast<PassData&>(node->data);
		auto builder = RenderGraphBuilder(*node, mRegistry);
		setup(builder, passData);
		return passData;
	}

	void AllocatePassResources(RenderGraphPassNode* pass, const CommandBuffer* cmd, SceneRenderingData& sceneData);
	void FreePassResources(RenderGraphPassNode* pass, const CommandBuffer* cmd);
	void SetupPassBarriers(RenderGraphPassNode* pass, const CommandBuffer* cmd);
	void ExecutePass(RenderGraphPassNode* pass, const CommandBuffer* cmd);
    
	RenderGraphContext mContext;

    RenderGraphResourceRegistry mRegistry;

	TArray<RenderGraphPassNode*> mPassNodes;

};

} // namespace Gleam
