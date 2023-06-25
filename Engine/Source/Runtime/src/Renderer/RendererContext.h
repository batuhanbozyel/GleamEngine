#pragma once
#include "Shader.h"
#include "RendererConfig.h"

namespace Gleam {

class RenderSystem;
struct Version;

class RendererContext final
{
	friend class RenderSystem;

public:
    
	const RefCounted<Shader>& CreateShader(const TString& entryPoint, ShaderStage stage);
    
    void Configure(const RendererConfig& config) const;
    
    const Size& GetDrawableSize() const;

private:
    
    void ConfigureBackend();
    
    void DestroyBackend();
    
	TArray<RefCounted<Shader>> mShaderCache;

};

} // namespace Gleam
