#pragma once
#include "Shader.h"
#include "CommandBuffer.h"
#include "RendererConfig.h"

namespace Gleam {

class Game;
struct Version;

struct RenderingData
{
    RenderTextureHandle colorTarget;
    RenderTextureHandle depthTarget;
};

class RendererContext final
{
	friend class Game;

public:
    
    void Execute(RenderPipeline& pipeline) const;
    
	const RefCounted<Shader>& CreateShader(const TString& entryPoint, ShaderStage stage);
    
    void SetConfiguration(const RendererConfig& config);
    
	const RendererConfig& GetConfiguration() const;
    
    const Size& GetDrawableSize() const;

private:
    
    void ConfigureBackend(const RendererConfig& config);
    
    void DestroyBackend();

    RendererConfig mConfiguration;
    
    Scope<CommandBuffer> mCommandBuffer;
    
	TArray<RefCounted<Shader>> mShaderCache;

};

} // namespace Gleam
