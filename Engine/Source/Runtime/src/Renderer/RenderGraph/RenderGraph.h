#pragma once
#include "RenderGraphBuilder.h"

namespace Gleam {

class GPUAllocator;
class Swapchain;
class CommandBuffer;
class GraphicsDevice;

struct RenderContext;

struct ImportResourceParams
{
	Color clearColor = Color::clear;
	BarrierLayout initialLayout = BarrierLayout::Undefined;
	BarrierLayout finalLayout = BarrierLayout::Undefined;
	bool clearOnFirstUse = false;
	bool releaseAfterLastUse = false;
};

struct RenderGraphContext
{
	GPUAllocator* allocator = nullptr;
	GraphicsDevice* device = nullptr;
	Swapchain* surface = nullptr;
};

class RenderGraph final
{
    template<typename PassData>
    using SetupFunc = std::function<void(RenderGraphBuilder& builder, PassData& passData)>;
    
public:
    
    RenderGraph(const RenderGraphContext& context);
    
    ~RenderGraph();
    
    void Compile();

	void Execute(const CommandBuffer* cmd);
	
	template<typename PassData>
	PassData& AddRenderPass(const TStringView name, SetupFunc<PassData>&& setup, RenderFunc<PassData>&& execute)
	{
        auto uniqueId = static_cast<uint32_t>(mPassNodes.size());
        auto node = new RenderGraphRenderPassNode(uniqueId, name, std::forward<decltype(execute)>(execute));
		return AddPassNode<PassData>(node, std::forward<decltype(setup)>(setup));
	}

	template<typename PassData>
	PassData& AddComputePass(const TStringView name, SetupFunc<PassData>&& setup, RenderFunc<PassData>&& execute)
	{
		auto uniqueId = static_cast<uint32_t>(mPassNodes.size());
		auto node = new RenderGraphComputePassNode(uniqueId, name, std::forward<decltype(execute)>(execute));
		return AddPassNode<PassData>(node, std::forward<decltype(setup)>(setup));
	}
    
    TextureHandle ImportTexture(const Texture& texture, const ImportResourceParams& params = ImportResourceParams());

	TextureHandle ImportBackbuffer(const ImportResourceParams& params = {});

    const TextureDescriptor& GetDescriptor(TextureHandle handle) const;

	GPUAllocator* GetAllocator() const;

private:
	
	template<typename PassData>
	PassData& AddPassNode(RenderGraphPassNode* node, SetupFunc<PassData>&& setup)
	{
		mPassNodes.push_back(node);
		auto& passData = std::any_cast<PassData&>(node->data);
		auto builder = RenderGraphBuilder(*node, mRegistry);
		setup(builder, passData);
		return passData;
	}

	void AllocatePassResources(RenderGraphPassNode* pass, const CommandBuffer* cmd);
	void FreePassResources(RenderGraphPassNode* pass, const CommandBuffer* cmd);
	void SetupPassBarriers(RenderGraphPassNode* pass, const CommandBuffer* cmd);
	void ExecutePass(RenderGraphPassNode* pass, const CommandBuffer* cmd);
    
	RenderGraphContext mContext;

    RenderGraphResourceRegistry mRegistry;

	TArray<RenderGraphPassNode*> mPassNodes;

	TArray<TextureHandle> mImportedTextures;

	TextureHandle mBackbuffer;

};

} // namespace Gleam
