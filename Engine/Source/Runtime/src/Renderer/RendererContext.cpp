#include "gpch.h"
#include "RendererContext.h"

using namespace Gleam;

const RefCounted<Shader>& RendererContext::CreateShader(const TString& entryPoint, ShaderStage stage)
{
    for (const auto& shader : mShaderCache)
    {
        if (shader->GetEntryPoint() == entryPoint)
        {
            return shader;
        }
    }
    
    return mShaderCache.emplace_back(CreateRef<Shader>(entryPoint, stage));
}

Buffer RendererContext::CreateBuffer(const BufferDescriptor& descriptor)
{

}
