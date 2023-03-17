#include "gpch.h"
#include "RenderGraph.h"

using namespace Gleam;

void RenderGraph::Compile()
{
    for (auto& pass : mPassNodes)
    {
        pass.refCount += pass.bufferWrites.size();
        for (auto resource : pass.bufferReads)
        {
            auto& consumed = mRegistry.GetBufferNode(resource);
            consumed.refCount++;
        }
        for (auto resource : pass.bufferWrites)
        {
            auto& written = mRegistry.GetBufferNode(resource);
            written.producer = &pass;
        }
    }
}

void RenderGraph::Execute(const CommandBuffer& cmd)
{
    RenderGraphContext context;
    context.cmd = &cmd;
    context.registry = &mRegistry;
    
    for (auto& pass : mPassNodes)
    {
        if (pass.isCulled())
            continue;
        
        // TODO: allocate pass resources
        
        pass.callback(context);
        
        // TODO: destroy transient resources
    }
}

RenderTextureHandle RenderGraph::Import(const RefCounted<RenderTexture>& renderTexture)
{
    
}
