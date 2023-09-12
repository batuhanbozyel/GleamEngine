#pragma once
#include "RenderGraphBuilder.h"
#include "../Allocators/PoolAllocator.h"
#include "../Allocators/StackAllocator.h"

namespace Gleam {

class CommandBuffer;

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
    
    void Compile();

	void Execute(const CommandBuffer* cmd);

	template<typename PassData>
	const PassData& AddRenderPass(const TStringView name, SetupFunc<PassData>&& setup, RenderFunc<PassData>&& execute)
	{
        auto uniqueId = static_cast<uint32_t>(mPassNodes.size());
        auto node = mPassNodes.emplace_back(new RenderPassNode(uniqueId, name, std::forward<decltype(execute)>(execute)));
		auto& passData = std::any_cast<PassData&>(node->data);
		auto builder = RenderGraphBuilder(*node, mRegistry);
		setup(builder, passData);
		return passData;
	}
    
    TextureHandle ImportBackbuffer(const RefCounted<RenderTexture>& backbuffer, const ImportResourceParams& params = ImportResourceParams());
    
    const BufferDescriptor& GetDescriptor(BufferHandle handle) const;
    
    const TextureDescriptor& GetDescriptor(TextureHandle handle) const;

private:
    
    PoolAllocator mPoolAllocator;
    
    StackAllocator mStackAllocator;

    RenderGraphResourceRegistry mRegistry;

	TArray<RenderPassNode*> mPassNodes;

};

} // namespace Gleam
