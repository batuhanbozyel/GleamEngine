#include "gpch.h"
#include "RendererContext.h"
#include "Core/Application.h"

using namespace Gleam;

void RendererContext::Execute()
{

}

RefCounted<Shader> RendererContext::CreateShader(const TString& entryPoint, ShaderStage stage)
{
	const auto& shaderCacheIt = mShaderCache.find(entryPoint);
	if (shaderCacheIt != mShaderCache.end())
	{
		return shaderCacheIt->second;
	}
	else
	{
		auto shader = CreateRef<Shader>(entryPoint, stage);
		mShaderCache.insert(mShaderCache.end(), {entryPoint, shader});
		return shader;
	}
}

RefCounted<RenderTexture> RendererContext::CreateRenderTexture(const Size& size, TextureFormat format, uint32_t sampleCount, bool useMipMap)
{
	// TODO: 
}

RenderTargetIdentifier RendererContext::CreateRenderTarget(const Size& size, TextureFormat format, uint32_t samples, bool useMipMap)
{
	return CreateRenderTarget(size, { format }, samples, useMipMap);
}

RenderTargetIdentifier RendererContext::CreateRenderTarget(const Size& size, TextureFormat colorFormat, TextureFormat depthFormat, uint32_t samples, bool useMipMap)
{
	return CreateRenderTarget(size, { colorFormat, depthFormat }, samples, useMipMap);
}

RenderTargetIdentifier RendererContext::CreateRenderTarget(bool useMipMap)
{
	return CreateRenderTarget(ApplicationInstance.GetActiveWindow().GetResolution(), { mConfiguration.format }, mConfiguration.sampleCount, useMipMap);
}

RenderTargetIdentifier RendererContext::CreateRenderTarget(const Size& size, bool useMipMap)
{
	return CreateRenderTarget(size, { mConfiguration.format }, mConfiguration.sampleCount, useMipMap);
}

RenderTargetIdentifier RendererContext::CreateRenderTarget(TextureFormat format, bool useMipMap)
{
	return CreateRenderTarget(ApplicationInstance.GetActiveWindow().GetResolution(), { format }, mConfiguration.sampleCount, useMipMap);
}

RenderTargetIdentifier RendererContext::CreateRenderTarget(const Size& size, TextureFormat format, bool useMipMap)
{
	return CreateRenderTarget(size, { format }, mConfiguration.sampleCount, useMipMap);
}

RenderTargetIdentifier RendererContext::CreateRenderTarget(TextureFormat colorFormat, TextureFormat depthFormat, bool useMipMap)
{
	return CreateRenderTarget(ApplicationInstance.GetActiveWindow().GetResolution(), { colorFormat, depthFormat }, mConfiguration.sampleCount, useMipMap);
}

RenderTargetIdentifier RendererContext::CreateRenderTarget(const Size& size, TextureFormat colorFormat, TextureFormat depthFormat, bool useMipMap)
{
	return CreateRenderTarget(size, { colorFormat, depthFormat }, mConfiguration.sampleCount, useMipMap);
}

RenderTargetIdentifier RendererContext::CreateRenderTarget(const TArray<TextureFormat>& formats, bool useMipMap)
{
	return CreateRenderTarget(ApplicationInstance.GetActiveWindow().GetResolution(), formats, mConfiguration.sampleCount, useMipMap);
}

RenderTargetIdentifier RendererContext::CreateRenderTarget(const Size& size, const TArray<TextureFormat>& formats, bool useMipMap)
{
	return CreateRenderTarget(size, formats, mConfiguration.sampleCount, useMipMap);
}

RenderTargetIdentifier RendererContext::CreateRenderTarget(const Size& size, const TArray<TextureFormat>& formats, uint32_t samples, bool useMipMap)
{
	TArray<RefCounted<RenderTexture>> colorBuffers;
	RefCounted<RenderTexture> depthBuffer;

	for (auto format : formats)
	{
		auto renderBuffer = CreateRenderTexture(size, format, samples, useMipMap);

		if (Utils::IsDepthStencilFormat(format))
			depthBuffer = renderBuffer;
		else
			colorBuffers.push_back(renderBuffer);
	}

	RenderTargetIdentifier renderTargetId = mTemporaryRenderTargets.size();
	RenderTarget renderTarget(colorBuffers, depthBuffer);
	mTemporaryRenderTargets.push_back(renderTarget);
	return renderTargetId;
}

const RendererConfig& RendererContext::GetConfiguration() const
{
	return mConfiguration;
}
