#pragma once
#include "Shader.h"
#include "CommandBuffer.h"
#include "RendererConfig.h"

namespace Gleam {

class Game;
struct Version;

class RendererContext final
{
	friend class Game;

public:
    
    void Exectute(PolyArray<IRenderer>& renderPipeline) const;
    
    RefCounted<RenderTexture> GetSwapchainTarget() const;
    
	const RefCounted<Shader>& CreateShader(const TString& entryPoint, ShaderStage stage);

	const RendererConfig& GetConfiguration() const;

private:

	void ConfigureBackend(const TString& appName, const Version& appVersion, const RendererConfig& config);

	void DestroyBackend();

	RendererConfig mConfiguration;

    CommandBuffer mCommandBuffer;
    
	TArray<RefCounted<Shader>> mShaderCache;

};

} // namespace Gleam
