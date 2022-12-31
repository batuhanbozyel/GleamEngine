#include "gpch.h"
#include "RendererContext.h"

using namespace Gleam;

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

RenderTargetIdentifier RendererContext::CreateRenderTarget(TextureFormat format, const Size& size, uint32_t samples = 1, bool useMipMap = false)
{
	auto renderBuffer = CreateRenderTexture(size, format, samples, useMipMap);
	renderBuffer->Lock();

	if (IsDepthStencilFormat(format))
		mDepthBuffer = renderBuffer;
	else
		mColorBuffers.push_back(renderBuffer);
}

RenderTargetIdentifier RendererContext::CreateRenderTarget(TextureFormat colorFormat, TextureFormat depthFormat, const Size& size, uint32_t samples = 1, bool useMipMap = false)
{
	auto renderBuffer = CreateRenderTexture(size, colorFormat, samples, useMipMap);
	renderBuffer->Lock();
	mColorBuffers.push_back(renderBuffer);

	mDepthBuffer = CreateRenderTexture(size, depthFormat, samples, useMipMap);
	mDepthBuffer->Lock();
}

RenderTargetIdentifier RendererContext::CreateRenderTarget(const TArray<TextureFormat>& formats, const Size& size, uint32_t samples = 1, bool useMipMap = false)
{
	for (auto format : formats)
	{
		auto renderBuffer = CreateRenderTexture(size, format, samples, useMipMap);
		renderBuffer->Lock();
		
		if (IsDepthStencilFormat(format))
			mDepthBuffer = renderBuffer;
		else
			mColorBuffers.push_back(renderBuffer);
	}
}

const RendererConfig& RendererContext::GetConfiguration() const
{
	return mConfiguration;
}