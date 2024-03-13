#include "gpch.h"
#include "RenderGraph.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"

#if defined(USE_METAL_RENDERER)
#import <Metal/Metal.h>
#elif defined(USE_DIRECTX_RENDERER)
#include "../DirectX/DirectXTransitionManager.h"
#endif

using namespace Gleam;

static AttachmentLoadAction GetLoadActionForRenderTexture(const RenderGraphTextureNode* node, RenderPassNode* pass)
{
    if (node->creator == pass || !node->transient)
    {
        return node->clearBuffer ? AttachmentLoadAction::Clear : AttachmentLoadAction::DontCare;
    }
    return AttachmentLoadAction::Load;
}

static AttachmentStoreAction GetStoreActionForRenderTexture(const RenderGraphTextureNode* node, RenderPassNode* pass)
{
    if (node->texture.GetDescriptor().sampleCount > 1)
    {
        return node->lastModifier == pass ? AttachmentStoreAction::Resolve : AttachmentStoreAction::StoreAndResolve;
    }
    return (node->lastReference == pass && !pass->hasSideEffect) ? AttachmentStoreAction::DontCare : AttachmentStoreAction::Store;
}

RenderGraph::RenderGraph(GraphicsDevice* device)
    : mDevice(device)
{
    
}

RenderGraph::~RenderGraph()
{
    for (auto pass : mPassNodes) { delete pass; }
    mPassNodes.clear();
    mRegistry.Clear();
}

void RenderGraph::Compile()
{
    // Setup resource dependency
    for (auto pass : mPassNodes)
    {
		// Buffer
        for (auto& resource : pass->bufferReads)
        {
            for (auto producer : resource.node->producers)
            {
                auto [_, success] = producer->dependents.insert(pass);
                pass->refCount += static_cast<uint32_t>(success);
            }
        }
        
        for (auto& resource : pass->bufferWrites)
        {
            if (resource.node->creator && resource.node->creator != pass)
            {
                auto [_, success] = resource.node->creator->dependents.insert(pass);
                pass->refCount += static_cast<uint32_t>(success);
            }
        }

		// Texture
		for (auto& resource : pass->textureReads)
		{
			for (auto producer : resource.node->producers)
			{
				auto [_, success] = producer->dependents.insert(pass);
				pass->refCount += static_cast<uint32_t>(success);
			}
		}
        
        for (auto& resource : pass->textureWrites)
        {
            if (resource.node->creator && resource.node->creator != pass)
            {
                auto [_, success] = resource.node->creator->dependents.insert(pass);
                pass->refCount += static_cast<uint32_t>(success);
            }
        }
    }
    
    // Perform topological sort
    Queue<RenderPassNode*> passQueue;
    TArray<RenderPassNode*> sortedPasses;
    for (auto pass : mPassNodes)
    {
        if (pass->refCount == 0)
        {
            passQueue.push(pass);
        }
    }
    
    while (!passQueue.empty())
    {
        auto pass = passQueue.front();
        passQueue.pop();
        
        sortedPasses.push_back(pass);
        for (auto dependent : pass->dependents)
        {
            if (--dependent->refCount == 0)
            {
                passQueue.push(dependent);
            }
        }
    }
    mPassNodes = sortedPasses;
    
    // Calculate resource lifetimes
    for (auto pass : mPassNodes)
    {
		// Buffer
        for (auto& resource : pass->bufferCreates)
        {
            auto node = static_cast<RenderGraphBufferNode*>(resource.node);
			auto memoryRequirements = mDevice->QueryMemoryRequirements({ .memoryType = MemoryType::GPU, .size = node->buffer.GetSize() });
            mHeapSize += Utils::AlignUp(memoryRequirements.size, memoryRequirements.alignment);
        }
        for (auto& resource : pass->bufferWrites)
        {
            resource.node->lastModifier = pass;
            resource.node->lastReference = pass;
        }
        for (auto& resource : pass->bufferReads)
        {
			resource.node->lastReference = pass;
        }

		// Texture
		for (auto& resource : pass->textureWrites)
		{
			resource.node->lastModifier = pass;
			resource.node->lastReference = pass;
		}
		for (auto& resource : pass->textureReads)
		{
			resource.node->lastReference = pass;
		}
    }
}

void RenderGraph::Execute(const CommandBuffer* cmd)
{
    Heap heap;
    if (mHeapSize > 0)
    {
        heap = mDevice->CreateHeap({ .memoryType = MemoryType::GPU, .size = mHeapSize });
    }

    for (auto pass : mPassNodes)
    {
        // Allocate buffers
        for (auto& resource : pass->bufferCreates)
        {
            if (HasResource(pass->bufferWrites, resource))
            {
                resource.node->buffer = heap.CreateBuffer(resource.node->buffer.GetSize());
                GLEAM_ASSERT(resource.node->buffer.IsValid());
            }
        }

        // Allocate textures
        for (auto& resource : pass->textureCreates)
        {
            if (HasResource(pass->textureWrites, resource))
            {
                resource.node->texture = mDevice->CreateTexture(resource.node->texture.GetDescriptor());
                GLEAM_ASSERT(resource.node->texture.IsValid());
            }
        }
        
        // execute render pass
        if (pass->isCustomPass())
        {
            std::invoke(pass->callback, cmd);
        }
        else
        {
            RenderPassDescriptor renderPassDesc{};
            renderPassDesc.colorAttachments.resize(pass->colorAttachments.size());
            for (uint32_t i = 0; i < pass->colorAttachments.size(); i++)
            {
                const auto node = static_cast<const RenderGraphTextureNode*>(pass->colorAttachments[i].node);
                renderPassDesc.colorAttachments[i].texture = node->texture;
                renderPassDesc.colorAttachments[i].loadAction = GetLoadActionForRenderTexture(node, pass);
                renderPassDesc.colorAttachments[i].storeAction = GetStoreActionForRenderTexture(node, pass);
                renderPassDesc.colorAttachments[i].clearColor = node->clearColor;
                
                const auto& descriptor = renderPassDesc.colorAttachments[i].texture.GetDescriptor();
                renderPassDesc.size = descriptor.size;
                renderPassDesc.samples = descriptor.sampleCount;
            }
            
            if (pass->depthAttachment.IsValid())
            {
                const auto node = static_cast<const RenderGraphTextureNode*>(pass->depthAttachment.node);
                renderPassDesc.depthAttachment.texture = node->texture;
                renderPassDesc.depthAttachment.loadAction = GetLoadActionForRenderTexture(node, pass);
                renderPassDesc.depthAttachment.storeAction = GetStoreActionForRenderTexture(node, pass);
                renderPassDesc.depthAttachment.clearDepth = node->clearDepth;
                renderPassDesc.depthAttachment.clearStencil = node->clearStencil;
                
                const auto& descriptor = renderPassDesc.depthAttachment.texture.GetDescriptor();
                renderPassDesc.size = descriptor.size;
                renderPassDesc.samples = descriptor.sampleCount;
            }
            
            cmd->BeginRenderPass(renderPassDesc, pass->name);
            cmd->SetViewport(renderPassDesc.size);
            
		#if defined(USE_METAL_RENDERER)
            [cmd->GetActiveRenderPass() useHeap:heap.GetHandle()];
            
            for (auto& resource : pass->textureReads)
            {
                [cmd->GetActiveRenderPass() useResource:resource.node->texture.GetView() usage:MTLResourceUsageRead stages:MTLRenderStageVertex | MTLRenderStageFragment];
            }
		#elif defined(USE_DIRECTX_RENDERER)
			for (auto& resource : pass->textureReads)
			{
				DirectXTransitionManager::TransitionLayout(
					static_cast<ID3D12GraphicsCommandList7*>(cmd->GetHandle()),
					static_cast<ID3D12Resource*>(resource.node->texture.GetHandle()),
					D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
				);
			}
		#endif
            std::invoke(pass->callback, cmd);
            cmd->EndRenderPass();
        }
    }

    // Release buffers & textures
    for (auto& pass : mPassNodes)
    {
        for (auto& resource : pass->bufferCreates)
        {
            mDevice->ReleaseBuffer(resource.node->buffer);
        }

        for (auto& resource : pass->textureCreates)
        {
            mDevice->ReleaseTexture(resource.node->texture);
        }
    }

    if (heap.IsValid())
    {
        mDevice->ReleaseHeap(heap);
    }
    
    for (auto pass : mPassNodes) { delete pass; }
    mPassNodes.clear();
    mRegistry.Clear();
}

TextureHandle RenderGraph::ImportBackbuffer(const Texture& backbuffer, const ImportResourceParams& params)
{
	RenderTextureDescriptor descriptor(backbuffer.GetDescriptor());
	descriptor.clearBuffer = params.clearOnFirstUse;
	descriptor.clearColor = params.clearColor;

	auto handle = mRegistry.CreateTexture(descriptor, false);
    handle.node->texture = backbuffer;
    return handle;
}

const TextureDescriptor& RenderGraph::GetDescriptor(TextureHandle handle) const
{
	auto node = static_cast<const RenderGraphTextureNode*>(handle.node);
	return node->texture.GetDescriptor();
}
