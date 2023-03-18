#pragma once
#include "Shader.h"
#include "CommandBuffer.h"
#include "RendererConfig.h"

namespace Gleam {

struct Version;

class View;
class Application;

class RendererContext final
{
	friend class Application;

public:
    
    void Exectute(const PolyArray<IRenderer>& renderPipeline) const;
    
	const RefCounted<Shader>& CreateShader(const TString& entryPoint, ShaderStage stage);
    
    const RefCounted<RenderTexture>& GetSwapchainTarget() const;

	const RendererConfig& GetConfiguration() const;

private:

	void ConfigureBackend(const TString& appName, const Version& appVersion, const RendererConfig& config);

	void DestroyBackend();

	RendererConfig mConfiguration;

    CommandBuffer mCommandBuffer;
    
	TArray<RefCounted<Shader>> mShaderCache;

};

} // namespace Gleam
