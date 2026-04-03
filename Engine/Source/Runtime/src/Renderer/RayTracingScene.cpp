#include "gpch.h"
#include "RayTracingScene.h"
#include "GraphicsDevice.h"
#include "Material/Material.h"

using namespace Gleam;

RayTracingScene::RayTracingScene(GraphicsDevice* device, GPUAllocator* allocator)
	: mDevice(device)
	, mAllocator(allocator)
	, mHitGroupRegistry()
{
}

RayTracingScene::~RayTracingScene()
{
	if (mTLAS.IsValid())
	{
		mDevice->Dispose(mTLAS);
	}
}

void RayTracingScene::ReleaseAccelerationStructure()
{
	if (mTLAS.IsValid())
	{
		mDevice->Dispose(mTLAS);
	}
}

void RayTracingScene::RegisterShadingPipeline(const Material* material)
{
	auto hash = material->GetSurfaceShaderHash();
	mHitGroupRegistry.Register(hash);
}

const HitGroupRegistry& RayTracingScene::GetRegistry() const
{
	return mHitGroupRegistry;
}

HitGroupRegistry::HitGroupRegistry()
{

}

uint32_t HitGroupRegistry::Register(uint32_t surfaceHash)
{
	auto it = mHashToIndex.find(surfaceHash);
	if (it != mHashToIndex.end())
	{
		return it->second;
	}

	uint32_t hitGroupIndex = (uint32_t)mHashToIndex.size();
	it = mHashToIndex.emplace_hint(mHashToIndex.end(), surfaceHash, hitGroupIndex);
	return hitGroupIndex;
}

uint32_t HitGroupRegistry::GetIndex(uint32_t surfaceHash) const
{
	auto it = mHashToIndex.find(surfaceHash);
	if (it != mHashToIndex.end())
	{
		return it->second;
	}
	return ~0u;
}

bool HitGroupRegistry::Contains(uint32_t surfaceHash) const
{
	return mHashToIndex.find(surfaceHash) != mHashToIndex.end();
}
