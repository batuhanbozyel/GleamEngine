#pragma once
#include "Shader.h"
#include "RenderTarget.h"
#include "RendererConfig.h"

namespace Gleam {
    
struct Version;

class RendererContext final
{
public:
    
	void ConfigureBackend(const TString& appName, const Version& appVersion, const RendererConfig& config);

	void DestroyBackend();

	RefCounted<Shader> CreateShader(const TString& entryPoint, ShaderStage stage);

	// Color RT
	RenderTargetIdentifier CreateRenderTarget(TextureFormat format, bool useMipMap = false)
	
	RenderTargetIdentifier CreateRenderTarget(TextureFormat format, const Size& size, bool useMipMap = false)

	RenderTargetIdentifier CreateRenderTarget(TextureFormat format, const Size& size, uint32_t samples, bool useMipMap = false);

	// Color - Depth RT
	RenderTargetIdentifier CreateRenderTarget(TextureFormat colorFormat, TextureFormat depthFormat, bool useMipMap = false);

	RenderTargetIdentifier CreateRenderTarget(TextureFormat colorFormat, TextureFormat depthFormat, const Size& size, bool useMipMap = false);

	RenderTargetIdentifier CreateRenderTarget(TextureFormat colorFormat, TextureFormat depthFormat, const Size& size, uint32_t samples, bool useMipMap = false);

	// Multi RT
	RenderTargetIdentifier CreateRenderTarget(const TArray<TextureFormat>& formats, bool useMipMap = false);

	RenderTargetIdentifier CreateRenderTarget(const TArray<TextureFormat>& formats, const Size& size, bool useMipMap = false);
	
	RenderTargetIdentifier CreateRenderTarget(const TArray<TextureFormat>& formats, const Size& size, uint32_t samples, bool useMipMap = false);

	RenderPassDescriptor CreateRenderPassDescriptor();

	const RendererConfig& GetConfiguration() const;
    
private:

	RenderTextureIdentifier CreateRenderTexture(TextureFormat format, const Size& size, uint32_t sampleCount, bool useMipMap);

	RendererConfig mConfiguration;

	TArray<RefCounted<RenderTexture>> mRenderTextures;

	HashMap<TString, RefCounted<Shader>> mShaderCache;

};

} // namespace Gleam
