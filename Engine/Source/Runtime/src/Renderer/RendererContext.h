#pragma once
#include "Shader.h"
#include "RenderTarget.h"
#include "RendererConfig.h"
#include "PipelineStateDescriptor.h"

namespace Gleam {

struct Version;
class Application;

class RendererContext final
{
	friend class Application;

public:

	RefCounted<Shader> CreateShader(const TString& entryPoint, ShaderStage stage);

	RefCounted<RenderTexture> CreateRenderTexture(const Size& size, TextureFormat format, uint32_t sampleCount, bool useMipMap);

	// Color RT
	RenderTargetIdentifier CreateRenderTarget(bool useMipMap = false);

	RenderTargetIdentifier CreateRenderTarget(const Size& size, bool useMipMap = false);

	RenderTargetIdentifier CreateRenderTarget(TextureFormat format, bool useMipMap = false);
	
	RenderTargetIdentifier CreateRenderTarget(const Size& size, TextureFormat format, bool useMipMap = false);

	RenderTargetIdentifier CreateRenderTarget(const Size& size, TextureFormat format, uint32_t samples, bool useMipMap = false);

	// Color - Depth RT
	RenderTargetIdentifier CreateRenderTarget(TextureFormat colorFormat, TextureFormat depthFormat, bool useMipMap = false);

	RenderTargetIdentifier CreateRenderTarget(const Size& size, TextureFormat colorFormat, TextureFormat depthFormat, bool useMipMap = false);

	RenderTargetIdentifier CreateRenderTarget(const Size& size, TextureFormat colorFormat, TextureFormat depthFormat, uint32_t samples, bool useMipMap = false);

	// Multi RT
	RenderTargetIdentifier CreateRenderTarget(const TArray<TextureFormat>& formats, bool useMipMap = false);

	RenderTargetIdentifier CreateRenderTarget(const Size& size, const TArray<TextureFormat>& formats, bool useMipMap = false);
	
	RenderTargetIdentifier CreateRenderTarget(const Size& size, const TArray<TextureFormat>& formats, uint32_t samples, bool useMipMap = false);

	const RendererConfig& GetConfiguration() const;

private:

	void ConfigureBackend(const TString& appName, const Version& appVersion, const RendererConfig& config);

	void DestroyBackend();

	void Execute();

	RendererConfig mConfiguration;

	TArray<RenderTarget> mTemporaryRenderTargets;

	TArray<RefCounted<RenderTexture>> mRenderTextures;

	HashMap<TString, RefCounted<Shader>> mShaderCache;

};

} // namespace Gleam
