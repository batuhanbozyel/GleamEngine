#pragma once
#include "RenderGraphBuilder.h"

namespace Gleam {

class RenderSurface;
class CommandBuffer;
class GraphicsDevice;
class RenderResourcePool;

struct RenderContext;
struct SceneRenderingData;

struct ImportResourceParams
{
	Color clearColor = Color::clear;
	bool clearOnFirstUse = true;
};

class RenderGraph final
{
    template<typename PassData>
    using SetupFunc = std::function<void(RenderGraphBuilder& builder, PassData& passData)>;
    
public:
    
    RenderGraph(RenderContext& context);
    
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
    
    size_t mHeapSize = 0;
    
	RenderSurface* mSurface;

    GraphicsDevice* mDevice;

	RenderResourcePool* mResourcePool;

    RenderGraphResourceRegistry mRegistry;

	TArray<RenderGraphPassNode*> mPassNodes;

};

} // namespace Gleam
